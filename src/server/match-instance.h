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

#pragma once

#include <vector>
#include <queue>
#include <chrono>
#include <utility>

#include <boost/asio.hpp>
#include <boost/asio/ts/timer.hpp>
#include <boost/uuid/uuid.hpp>

#include "game-instance.h"
#include "maps.h"
#include "match-settings.h"
#include "constants.h"
#include "message.h"
#include "match-result.h"
#include "game-state.h"

namespace asio = boost::asio;

// Player information struct for usage by MatchInstance
struct PlayerInfo
{
public:
    PlayerInfo(uint8_t id,
               uint64_t s_id,
               boost::uuids::uuid u_id,
               std::string user_string);

public:
    uint8_t PlayerID;
    uint64_t session_id;
    boost::uuids::uuid user_id;
    bool alive;
    std::string username;
};

struct ApplyResult
{
    bool valid_move;
    bool op_status;
};

static constexpr size_t MAX_QUEUE_SIZE = 8;

class MatchInstance : public std::enable_shared_from_this<MatchInstance>
{
using steady_timer = asio::steady_timer;
using steady_clock = std::chrono::steady_clock;
using SendCallback = std::function<void(uint64_t s_id, Message msg)>;
using ResultsCallback = std::function<void(MatchResult result)>;
using GameMessageCallback = std::function<void(boost::uuids::uuid sender,
                                               InternalMatchMessage msg)>;

public:
    MatchInstance() = delete;

    // Only valid constructor, refuse any other construction attempts.
    MatchInstance(asio::io_context & cntx,
                  MatchSettings settings,
                  std::vector<PlayerInfo> player_list,
                  SendCallback send_callback,
                  GameMessageCallback game_message);

    // To be called before the instance starts async operations.
    void set_results_callback(ResultsCallback cb);

    // Called by the networking layer to enqueue commands.
    void receive_command(boost::uuids::uuid user_id, Command cmd);

    void forfeit(boost::uuids::uuid user_id);

    // To be used when a reconnecting client needs to sync state.
    void sync_player(uint64_t session_id, boost::uuids::uuid user_id);

    void match_message(boost::uuids::uuid sender, InternalMatchMessage msg);

    void async_shutdown();

    // Initialization function to start a match.
    void start();

private:
    StaticMatchData compute_static_data();

    void update_view_timer(uint8_t player_ID);

    // Code for one turn of the match.
    void start_turn();

    // Concurrency safe function for start_turn using the match strand.
    void start_turn_strand();

    // Handles the arrival of a move in the context of a turn.
    void on_player_move_arrived(uint32_t t_id);

    // Handles timing out the player and preparing for the next game state.
    void handle_timeout();

    void handle_elimination(uint8_t p_id, HeaderType reason);

    // Applies a command, with a validation check returned.
    ApplyResult apply_command(const Command & cmd);

    // Compute the view of the game for every player.
    void compute_all_views();

    // Determines the winner and sends this information back to the server
    //
    // First, we should clean our instance up on its strand (cancel timers, etc)
    // and then we should let the match maker clean its memory up in its
    // callback.
    void conclude_game();

    // Internal function for the priority queue
    struct seq_comp {
        bool operator()(Command const& a, Command const& b) const {
            // priority_queue is a max-heap, invert this
            return a.sequence_number > b.sequence_number;
        }
    };

private:
    uint8_t n_players_;
    uint8_t elim_counter_;

public:
    uint8_t current_player;
    uint8_t remaining_players;
    uint8_t current_fuel;

private:
    asio::strand<asio::io_context::executor_type> strand_;

    asio::steady_timer timer_;
    steady_clock::time_point expiry_time_;

    // Move history for recording the game history.
    MatchResult results_;

    // Game related datastructures.
    std::vector<PlayerInfo> players_;
    uint8_t tanks_placed;
    GameInstance game_instance_;

    // The turn ID exists to ensure commands put into the queue
    // for a given turn are not used in later turns due to client or
    // server strand ordering differences.
    uint32_t turn_ID_;

    // Allows the winning async function (timer or move received) to
    // claim the turn and prevent the race condition caused by
    // having the strand order (move -> timeout -> start turn).
    bool turn_claimed_;

    std::chrono::milliseconds increment_;
    GameState current_state;

    // We have a vector of prioritiy queues, one for each player, sorted by
    // the sequence number of the given command.
    //
    // This allows us to attempt to faithfully execute commands by each user in
    // the order they specified them.
    //
    // The commands must be sorted because we might send out all three
    // fuel consumption moves in a row, which could be ordered incorrectly
    // on an async client. It is possible still that the moves
    // are sent incorrectly, but sorting will minimize this.
    //
    // Having the client wait for a "good move" callback would further
    // ensure the correct turn order is processed.
    std::vector<std::priority_queue<Command, std::vector<Command>, seq_comp>> command_queues_;

    // Per player counts of remaining time.
    std::vector<std::chrono::milliseconds> time_left_;

    // Per player views of the game.
    std::vector<PlayerView> player_views_;

    // Callback function to send message to a player's session.
    SendCallback send_callback_;

    // Callback to return the match result.
    ResultsCallback results_callback_;

    GameMessageCallback game_message_;

    bool shutdown{false};

};
