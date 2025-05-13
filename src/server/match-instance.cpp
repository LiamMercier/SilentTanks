#include "match-instance.h"

PlayerInfo::PlayerInfo(uint8_t p_id, uint64_t s_id)
:PlayerID(p_id), session_id(s_id), alive(true)
{

}

// Only valid constructor, refuse any other construction attempts.
MatchInstance::MatchInstance(asio::io_context & cntx,
                             const MatchSettings & settings,
                             std::vector<PlayerInfo> player_list,
                             uint8_t num_players,
                             SendCallback send_callback)
    : // initializer list
    current_player(0),
    remaining_players(num_players),
    current_fuel(TURN_PLAYER_FUEL),
    strand_(cntx.get_executor()),
    timer_(cntx),
    game_instance_(settings.map, num_players),
    players_(player_list),
    n_players_(num_players),
    tanks_placed(0),
    turn_ID_(0),
    increment_(std::chrono::milliseconds(settings.increment_ms)),
    current_state(GameState::Setup),
    command_queues_(num_players),
    time_left_(num_players, std::chrono::milliseconds(settings.initial_time_ms)),
    send_callback_(send_callback)
{
    // reserve and emplace_back into player_views_
    player_views_.reserve(num_players);
    for (int i = 0; i < num_players; i++)
    {
        player_views_.emplace_back(settings.map.width, settings.map.height);
    }
}

// Function to add a command to the queue (routed by the server)
void MatchInstance::receive_command(uint64_t session_id, Command cmd)
{
    asio::post(strand_,
        [self = shared_from_this(),
         m_cmd = std::move(cmd),
         t_id = turn_ID_,
         s_id = session_id]() mutable {

            // stale move due to strand ordering, cancel
            if (t_id != self->turn_ID_)
            {
                return;
            }

            // Otherwise, valid move, lets find the correct
            // player ID and then add this to the queue.
            //
            // We do this by scanning the vector to search for the ID.
            //
            // This should be fine since the vector is of minimal elements.
            uint8_t correct_id = UINT8_MAX;
            for (uint8_t p_id = 0; p_id < self->n_players_; p_id++)
            {
                if (self->players_[p_id].session_id == s_id)
                {
                    correct_id = p_id;
                    break;
                }
            }

            // If we couldn't find the player, return
            if (correct_id == UINT8_MAX)
            {
                return;
            }

            // Override the sender field
            m_cmd.sender = correct_id;

            // Enqueue the command
            self->command_queues_[m_cmd.sender].push(std::move(m_cmd));

            // notify if necessary
            if (m_cmd.sender == self->current_player)
            {
                self->on_player_move_arrived(t_id);
            }

        });
}

void MatchInstance::start()
{
    start_turn_strand();
}

// Handles one turn of the game. Should not be called directly.
void MatchInstance::start_turn()
{
    turn_ID_ += 1;
    turn_claimed_ = false;

    // setup end condition
    if (current_state == GameState::Setup &&
        tanks_placed >= remaining_players * game_instance_.num_tanks_)
    {
         current_state = GameState::Play;
         current_player = 0;
         current_fuel = TURN_PLAYER_FUEL;
    }

    // game end condition
    if (remaining_players <= 1)
    {
        conclude_game();
        return;
    }

    // skip eliminated players
    if (players_[current_player].alive == false)
    {
        current_player = ((current_player + 1) % n_players_);
        start_turn_strand();
        return;
    }

    // schedule move timeout and wait for turn
    //
    // if we cancel the timer, we will get an operation cancelled error_code
    // and if we have advanced the turn, the timer is likewise useless.
    //
    // thus, only perform timeout if we fail to cancel the timer and have not
    // moved to another player.
    expiry_time_ = steady_clock::now() + time_left_[current_player];

    timer_.expires_at(expiry_time_);
    timer_.async_wait(asio::bind_executor(strand_,
        [self = shared_from_this(),
         id=current_player,
         t_id = turn_ID_](asio::error_code ec)
        {
            // check for stale timer calls
            if (t_id != self->turn_ID_)
            {
                return;
            }

            // check that we didn't already move on this turn
            if (self->turn_claimed_ == true)
            {
                return;
            }

            // otherwise, timeout the player if expired
            if (!ec)
            {
                self->turn_claimed_ = true;
                self->handle_timeout();
            }
        })

    );

    // if we already have a move in the queue, use this now
    if (!command_queues_[current_player].empty())
    {
        on_player_move_arrived(turn_ID_);
    }

}

// Post work for the next turn to the strand.
void MatchInstance::start_turn_strand()
{
    asio::post(strand_, [self = shared_from_this()]{
            self->start_turn();
        });

}

void MatchInstance::on_player_move_arrived(uint16_t t_id)
{
    // Measure time of completion before timer cancel.
    steady_clock::time_point now = steady_clock::now();

    // Prevent stale turns.
    //
    // Likely not necessary but good to keep
    if (t_id != turn_ID_)
    {
        return;
    }

    // Prevent moving since timeout was already called.
    if (turn_claimed_ == true)
    {
        return;
    }

    // Claim the turn, timer may not run its handler anymore.
    turn_claimed_ = true;

    // Prevent operations after the game concluded before this
    // async operation was called.
    //
    // Should not be an issue, but good to leave for now.
    if (current_state == GameState::Concluded)
    {
        return;
    }

    // prevent calling this when nothing has actually arived
    //
    // this should never happen anyways unless someone calls
    // on_player_move_arrived incorrectly or logic error
    if (command_queues_[current_player].empty())
    {
        return;
    }

    // cancel the timer so we don't get timed out
    timer_.cancel();

    // compute the time difference
    steady_clock::duration remaining = std::chrono::milliseconds(0);
    if (expiry_time_ > now)
    {
        remaining = expiry_time_ - now;
    }

    // grab the command
    Command next_move = std::move(command_queues_[current_player].top());
    command_queues_[current_player].pop();

    // remove time
    time_left_[current_player] = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);

    // apply the command to the game state
    ApplyResult res = apply_command(next_move);

    // if the command is not valid, wait for the player to
    // try again (restart the turn)
    if (res.valid_move == false)
    {
        // TODO: callback failed move to player
        start_turn_strand();
        return;
    }

    time_left_[current_player] += increment_;

    // If we're in the setup phase, don't consume fuel.
    // Just show the tank placement.
    if (current_state == GameState::Setup)
    {
        // show the new tank placement
        compute_all_views();

        current_player = ((current_player + 1) % n_players_);
        start_turn_strand();
        return;
    }

    // Remove 1 fuel for this turn.
    current_fuel = current_fuel - 1;

    // Compute views for all players
    compute_all_views();

    // Turn update logic

    // If we're not out of fuel (and implicitly, setup is over)
    // then we simply start another turn for this player.
    if (current_fuel > 0)
    {
        start_turn_strand();
        return;
    }
    // Only happens if we used all fuel (thus GameState is Play)
    else
    {
        current_player = ((current_player + 1) % n_players_);
        current_fuel = TURN_PLAYER_FUEL;
        start_turn_strand();
        return;
    }

}

void MatchInstance::handle_timeout()
{
    // Handle accidental calls to handle_timeout twice.
    if (players_[current_player].alive == false)
    {
        return;
    }

    // Reclaim priority queue space.
    std::priority_queue<Command, std::vector<Command>, seq_comp> none;
    std::swap(command_queues_[current_player], none);

    // Remove the player.
    Player & this_player = game_instance_.get_player(current_player);
    remaining_players = remaining_players - 1;
    time_left_[current_player] = std::chrono::milliseconds(0);
    players_[current_player].alive = false;

    // Handle updating the setup criterion.
    if (current_state == GameState::Setup)
    {
        tanks_placed = tanks_placed - this_player.tanks_placed_;
    }

    // Set all tank health to zero.
    int * t_IDs = this_player.get_tanks_list();
    for (int i = 0; i < this_player.tanks_placed_; i++)
    {
        int t_ID = t_IDs[i];
        Tank & this_tank = game_instance_.get_tank(t_ID);
        this_tank.health_ = 0;
        // Set the tank tile to no occupant.
        game_instance_.set_occupant(this_tank.pos_, NO_OCCUPANT);
    }

    compute_all_views();

    // Prepare for the next player.
    current_player = ((current_player + 1) % n_players_);
    current_fuel = TURN_PLAYER_FUEL;

    // Go back to main game loop by posting a new turn.
    start_turn_strand();
}

ApplyResult MatchInstance::apply_command(const Command & cmd)
{
    ApplyResult res;
    res.valid_move = true;
    res.op_status = false;
    switch(cmd.type)
    {
        case CommandType::Move:
        {
            Tank & this_tank = game_instance_.get_tank(cmd.tank_id);
            if (this_tank.health_ == 0)
            {
                res.valid_move = false;
                break;
            }
            res.op_status = game_instance_.move_tank(cmd.tank_id, cmd.payload_first);
            break;
        }
        case CommandType::RotateTank:
        {
            Tank & this_tank = game_instance_.get_tank(cmd.tank_id);
            if (this_tank.health_ == 0)
            {
                res.valid_move = false;
                break;
            }
            game_instance_.rotate_tank(cmd.tank_id, cmd.payload_first);
            res.op_status = true;
            break;
        }
        case CommandType::RotateBarrel:
        {
            Tank & this_tank = game_instance_.get_tank(cmd.tank_id);
            if (this_tank.health_ == 0)
            {
                res.valid_move = false;
                break;
            }
            game_instance_.rotate_tank_barrel(cmd.tank_id, cmd.payload_first);
            res.op_status = true;
            break;
        }
        case CommandType::Fire:
        {
            Tank & this_tank = game_instance_.get_tank(cmd.tank_id);

            // check that the tank is alive
            if (this_tank.health_ == 0)
            {
                res.valid_move = false;
                break;
            }

            // check that the tank is able to fire
            if (this_tank.loaded_ == false)
            {
                res.valid_move = false;
                break;
            }

            // fire
            res.op_status = game_instance_.fire_tank(cmd.tank_id);
            break;
        }
        case CommandType::Load:
        {
            Tank & this_tank = game_instance_.get_tank(cmd.tank_id);
            if (this_tank.loaded_ == true)
            {
                res.valid_move = false;
                break;
            }
            // if unloaded, load the tank
            game_instance_.load_tank(cmd.tank_id);
            break;
        }
        case CommandType::Place:
        {
            if (current_state != GameState::Setup)
            {
                res.valid_move = false;
                break;
            }
            vec2 pos(cmd.payload_first, cmd.payload_second);

            if (pos.x_ > game_instance_.get_width() - 1 || pos.y_ > game_instance_.get_height() - 1)
            {
                res.valid_move = false;
                break;
            }

            game_instance_.place_tank(pos, cmd.sender);
            break;
        }
        default:
        {
            res.valid_move = false;
            break;
        }
    }
    return res;
}

void MatchInstance::compute_all_views()
{
    for (int i = 0; i < n_players_; i++)
    {
        std::cout << "View : " << i << "\n";

        // avoid unnecessary computation
        if (players_[i].alive == false)
        {
            continue;
        }

        uint8_t live_tanks = 0;

        player_views_[i] = game_instance_.compute_view(i, live_tanks);

        if (live_tanks == 0 && current_state != GameState::Setup)
        {
            // first time we have observed this player to have no tanks
            //
            // timeout would have set this player as not alive
            remaining_players -= 1;

            players_[i].alive = false;
        }

        player_views_[i].print_view(game_instance_.tanks_);

    }

}

void MatchInstance::conclude_game()
{
    // At this point, there is only one player. Determine the winner.
    current_state = GameState::Concluded;

    uint8_t winner = UINT8_MAX;
    for (int i = 0; i < n_players_; i++)
    {
        if (players_[i].alive == true)
        {
            winner = i;
            break;
        }
    }

    std::cout << winner << "\n";

    // TODO: callbacks at end of game

    return;
}

