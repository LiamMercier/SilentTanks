// Copyright (c) 2025 Liam Mercier
//
// This file is part of SilentTanks.
//
// SilentTanks is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License Version 3.0
// as published by the Free Software Foundation.
//
// SilentTanks is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License v3.0
// for more details.
//
// You should have received a copy of the GNU Affero General Public License v3.0
// along with SilentTanks. If not, see <https://www.gnu.org/licenses/agpl-3.0.txt>

#include "match-instance.h"

PlayerInfo::PlayerInfo(uint8_t id,
                       uint64_t s_id,
                       boost::uuids::uuid u_id,
                       std::string user_string)
:PlayerID(id),
session_id(s_id),
user_id(u_id),
alive(true),
username(user_string)
{

}

// Only valid constructor, refuse any other construction attempts.
MatchInstance::MatchInstance(asio::io_context & cntx,
                             MatchSettings settings,
                             std::vector<PlayerInfo> player_list,
                             SendCallback send_callback,
                             GameMessageCallback game_message)
    :
    n_players_(settings.map.map_settings.num_players),
    elim_counter_(0),
    current_player(0),
    remaining_players(n_players_),
    current_fuel(TURN_PLAYER_FUEL),
    strand_(cntx.get_executor()),
    timer_(cntx),
    results_(settings.map.map_settings,
             settings.initial_time_ms,
             settings.increment_ms),
    players_(player_list),
    tanks_placed(0),
    game_instance_(std::move(settings.map)),
    turn_ID_(0),
    increment_(std::chrono::milliseconds(settings.increment_ms)),
    current_state(GameState::Setup),
    command_queues_(n_players_),
    time_left_(n_players_,
               std::chrono::milliseconds(settings.initial_time_ms)),
    send_callback_(send_callback),
    results_callback_(nullptr),
    game_message_(game_message)
{
    player_views_.reserve(n_players_);
    for (int i = 0; i < n_players_; i++)
    {
        player_views_.emplace_back(game_instance_.get_width(),
                                   game_instance_.get_height());
    }
}

void MatchInstance::set_results_callback(ResultsCallback cb)
{
    results_callback_ = std::move(cb);
}

// Function to add a command to the queue (routed by the server)
void MatchInstance::receive_command(boost::uuids::uuid user_id, Command cmd)
{
    asio::post(strand_,
        [self = shared_from_this(),
         m_cmd = std::move(cmd),
         t_id = turn_ID_,
         user_id = user_id]() mutable {

            // Lets find the correct player ID and then add this to the queue.
            //
            // We do this by scanning the vector to search for the ID.
            //
            // This should be fine since the vector is of minimal elements.
            uint8_t correct_id = UINT8_MAX;
            for (uint8_t p_id = 0; p_id < self->n_players_; p_id++)
            {
                if (self->players_[p_id].user_id == user_id)
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

            // Check if game is over.
            if (self->current_state == GameState::Concluded)
            {
                Message match_over;
                match_over.create_serialized(HeaderType::GameEnded);

                self->send_callback_(self->players_[correct_id].session_id,
                                     std::move(match_over));
                return;
            }

            // stale move due to strand ordering, cancel
            if (t_id != self->turn_ID_)
            {
                Message failed_move_msg;
                failed_move_msg.create_serialized(HeaderType::StaleMove);

                self->send_callback_(self->players_[correct_id].session_id,
                                     std::move(failed_move_msg));

                return;
            }

            // Override the sender field.
            m_cmd.sender = correct_id;

            // Enqueue the command if there is room. Otherwise, drop.
            if (self->command_queues_[m_cmd.sender].size() < MAX_QUEUE_SIZE)
            {
                self->command_queues_[m_cmd.sender].push(std::move(m_cmd));
            }
            else
            {
                return;
            }

            // notify if necessary
            if (m_cmd.sender == self->current_player)
            {
                self->on_player_move_arrived(t_id);
            }

        });
}

void MatchInstance::forfeit(boost::uuids::uuid user_id)
{
    asio::post(strand_,
        [self = shared_from_this(), u_id = user_id]{

        // Find the player given the user ID.
        uint8_t correct_id = UINT8_MAX;
        for (uint8_t p_id = 0; p_id < self->n_players_; p_id++)
        {
            if (self->players_[p_id].user_id == u_id)
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

        // Check if game is over.
        if (self->current_state == GameState::Concluded)
        {
            Message match_over;
            match_over.create_serialized(HeaderType::GameEnded);

            self->send_callback_(self->players_[correct_id].session_id,
                                 std::move(match_over));
            return;
        }

        // Now, if we got a player, we do regular elimination logic.
        //
        // We can just do the usual elimination logic.
        self->handle_elimination(correct_id, HeaderType::ForfeitMatch);

    });
}

void MatchInstance::sync_player(uint64_t session_id,
                                boost::uuids::uuid user_id)
{
    asio::post(strand_,
               [self = shared_from_this(),
               s_id = session_id,
               u_id = user_id]{

            // Check if game is over.
            if (self->current_state == GameState::Concluded)
            {
                Message match_over;
                match_over.create_serialized(HeaderType::GameEnded);

                self->send_callback_(s_id, std::move(match_over));
                return;
            }

            // Get the player ID for this user.
            uint8_t correct_id = UINT8_MAX;
            for (uint8_t p_id = 0; p_id < self->n_players_; p_id++)
            {
                if (self->players_[p_id].user_id == u_id)
                {
                    correct_id = p_id;
                    break;
                }
            }

            // If we couldn't find the player, return
            if (correct_id == UINT8_MAX)
            {
                Message match_over;
                match_over.create_serialized(HeaderType::GameEnded);

                self->send_callback_(s_id, std::move(match_over));
                return;
            }

            self->players_[correct_id].session_id = s_id;

            // We should dump the old session's commands
            // to make the client experience more consistent.
            std::priority_queue<Command, std::vector<Command>, seq_comp> none;
            std::swap(self->command_queues_[correct_id], none);

            // Compute static data and send.
            StaticMatchData match_data = self->compute_static_data();

            Message static_data_msg;
            static_data_msg.create_serialized(match_data);
            self->send_callback_(self->players_[correct_id].session_id,
                                 static_data_msg);

            // Views were already computed in previous strand call
            // because we always compute everyone's view at
            // the end of an operation. Just grab this and send
            // the view to the client with updated timer.
            self->update_view_timer(correct_id);

            Message view_message;
            view_message.create_serialized(self->player_views_[correct_id]);
            self->send_callback_(s_id, std::move(view_message));

    });
}

void MatchInstance::match_message(boost::uuids::uuid sender,
                                 InternalMatchMessage msg)
{
    asio::post(strand_,
               [wp = weak_from_this(),
               sender = std::move(sender),
               msg = std::move(msg)]{

        // Only do work if game is still in progress.
        if (auto self = wp.lock())
        {
            // Deliver messages to all users, using the user manager.
            for (uint8_t i = 0; i < self->n_players_; i++)
            {
                if (self->players_[i].user_id != sender)
                {
                    InternalMatchMessage dm;
                    dm.user_id = self->players_[i].user_id;
                    dm.sender_username = msg.sender_username;
                    dm.text = msg.text;

                    self->game_message_(sender, dm);
                }
            }
        }

        return;
    });
}

// Force close the instance.
void MatchInstance::async_shutdown()
{
    asio::dispatch(strand_,
            [self = shared_from_this()]{

    if (self->shutdown)
    {
        return;
    }

    self->shutdown = true;

    self->timer_.cancel();
    self->send_callback_ = nullptr;
    self->results_callback_ = nullptr;

    });
}

void MatchInstance::start()
{
    // Create a list of static data to send to all users.
    StaticMatchData match_data = compute_static_data();

    for (uint8_t id = 0; id < players_.size(); id++)
    {
        Message static_data_msg;
        static_data_msg.create_serialized(match_data);

        send_callback_(players_[id].session_id, static_data_msg);
    }

    // Send an initial view of the environment to each player
    // and then start the game asynchronously
    compute_all_views();

    start_turn_strand();
}

StaticMatchData MatchInstance::compute_static_data()
{
    StaticMatchData match_data;
    match_data.player_list.users.reserve(players_.size());

    for (uint8_t id = 0; id < players_.size(); id++)
    {
        ExternalUser curr_player;

        curr_player.user_id = players_[id].user_id;
        curr_player.username = players_[id].username;

        match_data.player_list.users.emplace_back(curr_player);
    }

    match_data.placement_mask = game_instance_.get_mask();

    return match_data;
}

void MatchInstance::update_view_timer(uint8_t player_ID)
{
    steady_clock::time_point now = steady_clock::now();
    std::chrono::milliseconds time = std::chrono::milliseconds(0);

    // Compute the elapsed time and update this timer.
    steady_clock::duration remaining = expiry_time_ - now;
    time = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);

    player_views_[player_ID].timers[current_player] = time;
    return;
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

        // Send this updated view so players know the state is updated.
        compute_all_views();
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
        compute_all_views();
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
         t_id = turn_ID_](boost::system::error_code ec)
        {
            // check for stale timer calls
            if (t_id != self->turn_ID_)
            {
                return;
            }

            // check that we didn't already move on this turn
            if (self->turn_claimed_)
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

void MatchInstance::on_player_move_arrived(uint32_t t_id)
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
        // Callback failed move to player
        Message failed_move_msg;
        failed_move_msg.create_serialized(HeaderType::FailedMove);

        send_callback_(players_[current_player].session_id,
                       std::move(failed_move_msg));

        start_turn_strand();
        return;
    }

    time_left_[current_player] += increment_;

    // Add this command to the move history of the match
    results_.move_history.push_back(std::move(CommandHead(next_move)));

    // If we're in the setup phase, don't consume fuel.
    // Just show the tank placement.
    if (current_state == GameState::Setup)
    {
        current_player = ((current_player + 1) % n_players_);

        // show the new tank placement
        compute_all_views();

        start_turn_strand();
        return;
    }

    // Remove 1 fuel for this turn.
    current_fuel = current_fuel - 1;

    // Turn update logic

    // If we're not out of fuel (and implicitly, setup is over)
    // then we simply start another turn for this player.
    if (current_fuel > 0)
    {
        // Compute views for all players
        compute_all_views();

        start_turn_strand();
        return;
    }
    // Only happens if we used all fuel (thus GameState is Play)
    else
    {
        current_player = ((current_player + 1) % n_players_);
        current_fuel = TURN_PLAYER_FUEL;

        // Compute views for all players
        compute_all_views();

        start_turn_strand();
        return;
    }

}

void MatchInstance::handle_timeout()
{
    handle_elimination(current_player, HeaderType::TimedOut);
}

void MatchInstance::handle_elimination(uint8_t p_id, HeaderType reason)
{
    // Handle accidental calls to handle_timeout twice.
    if (players_[p_id].alive == false)
    {
        return;
    }

    // Reclaim priority queue space.
    std::priority_queue<Command, std::vector<Command>, seq_comp> none;
    std::swap(command_queues_[p_id], none);

    // Remove the player.
    Player & this_player = game_instance_.get_player(p_id);
    remaining_players = remaining_players - 1;
    time_left_[p_id] = std::chrono::milliseconds(0);
    players_[p_id].alive = false;
    results_.elimination_order[p_id] = elim_counter_;
    elim_counter_++;

    // Inform the player that they are eliminated
    Message inform_elimination;
    inform_elimination.create_serialized(reason);
    send_callback_(players_[p_id].session_id, inform_elimination);

    // Handle updating the setup criterion.
    if (current_state == GameState::Setup)
    {
        tanks_placed = tanks_placed - this_player.tanks_placed_;
    }

    // Set all tank health to zero.
    std::vector<int> & t_IDs = this_player.get_tanks_list();
    for (int i = 0; i < this_player.tanks_placed_; i++)
    {
        int t_ID = t_IDs[i];
        Tank & this_tank = game_instance_.get_tank(t_ID);
        this_tank.health_ = 0;
        // Set the tank tile to no occupant.
        game_instance_.set_occupant(this_tank.pos_, NO_OCCUPANT);
    }

    compute_all_views();

    // Prepare for the next player if necessary.
    if (p_id == current_player)
    {
        current_player = ((current_player + 1) % n_players_);
        current_fuel = TURN_PLAYER_FUEL;
        // Cancel timers.
        timer_.cancel();
    }

    // Go back to main game loop by posting a new turn.
    start_turn_strand();
}

ApplyResult MatchInstance::apply_command(const Command & cmd)
{
    ApplyResult res;
    res.valid_move = true;
    res.op_status = false;

    // Enforce setup moves.
    if (current_state == GameState::Setup
        && cmd.type != CommandType::Place)
    {
        res.valid_move = false;
        return res;
    }

    switch(cmd.type)
    {
        case CommandType::Move:
        {
            // If the tank does not exist, stop early.
            if (cmd.tank_id >= game_instance_.num_players_
                                * game_instance_.num_tanks_)
            {
                res.valid_move = false;
                break;
            }

            Tank & this_tank = game_instance_.get_tank(cmd.tank_id);

            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
            {
                res.valid_move = false;
                break;
            }
            res.op_status = game_instance_.move_tank(cmd.tank_id,
                                                     cmd.payload_first);
            break;
        }
        case CommandType::RotateTank:
        {
            // If the tank does not exist, stop early.
            if (cmd.tank_id >= game_instance_.num_players_
                                * game_instance_.num_tanks_)
            {
                res.valid_move = false;
                break;
            }

            Tank & this_tank = game_instance_.get_tank(cmd.tank_id);

            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
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
            // If the tank does not exist, stop early.
            if (cmd.tank_id >= game_instance_.num_players_
                                * game_instance_.num_tanks_)
            {
                res.valid_move = false;
                break;
            }

            Tank & this_tank = game_instance_.get_tank(cmd.tank_id);

            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
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
            // If the tank does not exist, stop early.
            if (cmd.tank_id >= game_instance_.num_players_
                                * game_instance_.num_tanks_)
            {
                res.valid_move = false;
                break;
            }

            Tank & this_tank = game_instance_.get_tank(cmd.tank_id);

            // check that the tank is alive
            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
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
            // If the tank does not exist, stop early.
            if (cmd.tank_id >= game_instance_.num_players_
                                * game_instance_.num_tanks_)
            {
                res.valid_move = false;
                break;
            }

            Tank & this_tank = game_instance_.get_tank(cmd.tank_id);

            if (this_tank.loaded_ == true
                || this_tank.health_ == 0
                || this_tank.owner_ != cmd.sender)
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

            if (pos.x_ > game_instance_.get_width() - 1
                || pos.y_ > game_instance_.get_height() - 1)
            {
                res.valid_move = false;
                break;
            }

            // Check if placement is within the player's
            // permitted placement area.
            bool valid = game_instance_.check_placement(pos, cmd.sender);

            if (!valid)
            {
                res.valid_move = false;
                break;
            }

            uint8_t placement_direction = cmd.tank_id;

            if (placement_direction >= 8)
            {
                res.valid_move = false;
                break;
            }

            game_instance_.place_tank(pos,
                                      cmd.sender,
                                      placement_direction);

            if (res.valid_move == true)
            {
                tanks_placed += 1;
            }

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
            results_.elimination_order[i] = elim_counter_;
            elim_counter_++;

            // Inform the player that they are eliminated
            Message inform_elimination;
            inform_elimination.create_serialized(HeaderType::Eliminated);
            send_callback_(players_[i].session_id, inform_elimination);
        }

        // Append state information.
        player_views_[i].current_player = current_player;
        player_views_[i].current_fuel = current_fuel;
        player_views_[i].current_state = current_state;

        // Append time for each player.
        player_views_[i].timers = time_left_;

        // Send view to the player
        Message view_message;
        view_message.create_serialized(player_views_[i]);
        send_callback_(players_[i].session_id,
                       std::move(view_message));


    }

}

void MatchInstance::conclude_game()
{
    // Prevent more database calls on server close.
    if (shutdown)
    {
        return;
    }

    // At this point, there is only one player. Determine the winner.
    current_state = GameState::Concluded;

    uint8_t winner = UINT8_MAX;
    for (int i = 0; i < n_players_; i++)
    {
        if (players_[i].alive == true)
        {
            winner = i;
            results_.elimination_order[i] = elim_counter_;
        }

        results_.user_ids[i] = players_[i].user_id;
    }

    // Tell winner that they won, everyone else has already
    // been told that they lost.
    if (winner < n_players_)
    {
        Message inform_winner;
        inform_winner.create_serialized(HeaderType::Victory);
        send_callback_(players_[winner].session_id, inform_winner);
    }

    // Call back to the server to write results to the database
    // and handle any updates
    results_callback_(results_);

    // Break down the game instance. We do not need it anymore.
    timer_.cancel();

    send_callback_ = nullptr;
    results_callback_ = nullptr;

    return;
}

