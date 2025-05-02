#include "match-instance.h"

PlayerInfo::PlayerInfo(uint8_t id)
:PlayerID(id), alive(true)
{

}

// Only valid constructor, refuse any other construction attempts.
MatchInstance::MatchInstance(const GameMap & map,
                             std::vector<PlayerInfo> player_list,
                             uint8_t num_players,
                             uint64_t initial_time_ms,
                             uint64_t increment_ms)
    : // initializer list
    current_player(0),
    remaining_players(num_players),
    current_fuel(TURN_PLAYER_FUEL),
    game_instance_(map, num_players),
    players_(player_list),
    n_players_(num_players),
    increment_(std::chrono::milliseconds(increment_ms)),
    current_state(GameState::Setup),
    command_queues_(num_players), queue_mutex_(num_players),
    queue_cvs_(num_players),
    time_left_(num_players, std::chrono::milliseconds(initial_time_ms))
{
    // reserve and emplace_back into player_views_
    player_views_.reserve(num_players);
    for (int i = 0; i < num_players; i++)
    {
        player_views_.emplace_back(map.width, map.height);
    }
}

//
// [ CONCURRENCY ]
//
// This function will be called from other threads to allow
// writes to the player's command PQ.
void MatchInstance::receive_command(const Command& cmd)
{
    std::lock_guard<std::mutex> queue_lock(queue_mutex_[cmd.sender]);
    command_queues_[cmd.sender].push(cmd);
    queue_cvs_[cmd.sender].notify_one();
}
//
// [ /CONCURRENCY ]
//

void MatchInstance::setup_loop()
{
    using clock = std::chrono::steady_clock;
    using ms = std::chrono::milliseconds;

    tanks_placed = 0;
    current_player = 0;

    // Setup loop
    while(remaining_players > 1)
    {
        // skip eliminated players
        if (players_[current_player].alive != true)
        {
            current_player = ((current_player + 1) % n_players_);
            continue;
        }

        clock::time_point start_time = clock::now();

        // wait loop for this player to make a move
        Command next_move;
        bool got_move = wait_player_move(current_player, next_move);

        clock::time_point end_time = clock::now();
        ms elapsed = std::chrono::duration_cast<ms>(end_time - start_time);

        // handle timeout
        if (!got_move)
        {
            handle_timeout();
        }

        // if not timed out, subtract time from player clock
        time_left_[current_player] -= elapsed;

        ApplyResult res = apply_command(next_move);

        // Allow for a valid move to be issued if this one is invalid
        //
        // This will start up a new clock and wait for
        // the player
        if (res.valid_move == false)
        {
            std::cout << "invalid move\n";
            continue;
        }

        // Add increment to the timer of this player when a
        // valid move occurs
        time_left_[current_player] += increment_;

        current_player = ((current_player + 1) % n_players_);
        tanks_placed = tanks_placed + 1;

        // If all tanks are placed, exit the setup loop and update the game state
        if(tanks_placed >= remaining_players * game_instance_.num_tanks_)
        {
            current_state = GameState::Play;
            return;
        }
        compute_all_views();
        continue;
    }
}

uint8_t MatchInstance::game_loop()
{
    setup_loop();

    // After setup, use standard move options
    current_player = 0;

    // main game loop
    while (remaining_players > 1)
    {
        using clock = std::chrono::steady_clock;
        using ms = std::chrono::milliseconds;

        // skip eliminated players
        if (players_[current_player].alive != true)
        {
            current_player = ((current_player + 1) % n_players_);
            current_fuel = TURN_PLAYER_FUEL;
            continue;
        }

        clock::time_point start_time = clock::now();

        // wait for this player to make a move (blocks main game thread)
        Command next_move;
        bool got_move = wait_player_move(current_player, next_move);

        clock::time_point end_time = clock::now();
        ms elapsed = std::chrono::duration_cast<ms>(end_time - start_time);

        // handle player timeout
        if (!got_move)
        {
            handle_timeout();
        }

        // if not timed out, subtract time from player clock
        time_left_[current_player] -= elapsed;

        ApplyResult res = apply_command(next_move);

        // Allow for a valid move to be issued if this one is invalid
        //
        // This will start up a new clock and wait for
        // the player but will not consume fuel.
        if (res.valid_move == false)
        {
            std::cout << "invalid move\n";
            continue;
        }

        // Add increment to the timer of this player when a
        // valid move occurs
        time_left_[current_player] += increment_;

        // consume fuel
        current_fuel = current_fuel - 1;

        // update turn state
        if (current_fuel > 0)
        {
            compute_all_views();
            continue;
        }
        else
        {
            current_player = ((current_player + 1) % n_players_);
            current_fuel = TURN_PLAYER_FUEL;
            compute_all_views();
            continue;
        }

    }

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

    return winner;
}

void MatchInstance::handle_timeout()
{
    Player & this_player = game_instance_.get_player(current_player);
    remaining_players = remaining_players - 1;
    time_left_[current_player] = std::chrono::milliseconds(0);
    players_[current_player].alive = false;

    if (current_state == GameState::Setup)
    {
        tanks_placed = tanks_placed - this_player.tanks_placed_;
    }

    // set all tank health to zero
    int * t_IDs = this_player.get_tanks_list();
    for (int i = 0; i < this_player.tanks_placed_; i++)
    {
        int t_ID = t_IDs[i];
        Tank & this_tank = game_instance_.get_tank(t_ID);
        this_tank.health_ = 0;
        // set the tank tile to no occupant
        game_instance_.set_occupant(this_tank.pos_, NO_OCCUPANT);
    }
    // prepare for next player
    current_player = ((current_player + 1) % n_players_);
    compute_all_views();
    if (current_state == GameState::Play)
    {
        current_fuel = TURN_PLAYER_FUEL;
    }
    return;
}

bool MatchInstance::wait_player_move(uint8_t player_id, Command &out_cmd)
{
    // avoid long type names
    using namespace std::chrono;

    // grab mutex for this player's queue
    std::unique_lock<std::mutex> queue_lock(queue_mutex_[player_id]);

    // grab queue item if it exists
    if (!command_queues_[player_id].empty())
    {
        out_cmd = command_queues_[player_id].top();
        command_queues_[player_id].pop();
        return true;
    }

    // otherwise, we must be waiting on this player, start the clock
    milliseconds time_left = time_left_[player_id];
    steady_clock::time_point deadline = steady_clock::now() + time_left;

    // wait for an update to the player's commands
    bool awaken_on_data = queue_cvs_[player_id].wait_until(
        queue_lock, deadline,
        [&]{return !command_queues_[player_id].empty();});

    if (awaken_on_data)
    {
        out_cmd = command_queues_[player_id].top();
        command_queues_[player_id].pop();
        return true;
    }

    // otherwise the player timed out
    return false;

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
            res.op_status = game_instance_.move_tank(cmd.tank_id, cmd.payload);
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
            game_instance_.rotate_tank(cmd.tank_id, cmd.payload);
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
            game_instance_.rotate_tank_barrel(cmd.tank_id, cmd.payload);
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
            vec2 pos(cmd.payload, cmd.payload_optional);

            if (pos.x_ > game_instance_.get_width() - 1 || pos.y_ > game_instance_.get_height() - 1)
            {
                res.valid_move = false;
                break;
            }

            game_instance_.place_tank(pos, cmd.sender);
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
            remaining_players -= 1;

            players_[i].alive = false;
        }

        player_views_[i].print_view(game_instance_.tanks_);

    }

}

