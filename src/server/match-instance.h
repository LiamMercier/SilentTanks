#pragma once

#include <vector>
#include <queue>
#include <chrono>

#include <asio.hpp>
#include <asio/ts/timer.hpp>

#include "game-instance.h"
#include "maps.h"
#include "constants.h"
#include "message.h"

// Player information struct for usage by MatchInstance
struct PlayerInfo
{
public:
    PlayerInfo(uint8_t id, uint64_t s_id);

public:
    uint8_t PlayerID;
    uint64_t session_id;
    bool alive;
};

enum class GameState : uint8_t
{
    Setup,
    Play,
    Concluded
};

struct ApplyResult {
    bool valid_move;
    bool op_status;
};

class MatchInstance : public std::enable_shared_from_this<MatchInstance>
{
using steady_timer = asio::steady_timer;
using steady_clock = std::chrono::steady_clock;
using SendCallback = std::function<void(uint64_t s_id, Message msg)>;

public:
    MatchInstance() = delete;

    // Only valid constructor, refuse any other construction attempts.
    MatchInstance(asio::io_context & cntx,
                  const MatchSettings & settings,
                  std::vector<PlayerInfo> player_list,
                  uint8_t num_players,
                  SendCallback send_callback);

    // Called by the networking layer to enqueue commands.
    void receive_command(uint64_t session_id, Command cmd);

    // Initialization function to start a match.
    void start();

private:

    // Code for one turn of the match.
    void start_turn();

    // Concurrency safe function for start_turn using the match strand.
    void start_turn_strand();

    // Handles the arrival of a move in the context of a turn.
    void on_player_move_arrived(uint16_t t_id);

    // Handles timing out the player and preparing for the next game state.
    void handle_timeout();

    // Applies a command, with a validation check returned.
    ApplyResult apply_command(const Command & cmd);

    // Compute the view of the game for every player.
    void compute_all_views();

    // Determines the winner and sends this information back to the server
    //
    // TODO: tear down the instance
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

public:
    uint8_t current_player;
    uint8_t remaining_players;
    uint8_t current_fuel;

private:
    asio::strand<asio::io_context::executor_type> strand_;

    asio::steady_timer timer_;
    steady_clock::time_point expiry_time_;

    // Game related datastructures.
    GameInstance game_instance_;
    std::vector<PlayerInfo> players_;
    uint8_t n_players_;
    uint8_t tanks_placed;

    // important
    // TODO: document this
    uint16_t turn_ID_;

    // claims the turn to prevent race condition (move -> timeout -> start turn)
    // TODO: document this
    bool turn_claimed_;

    std::chrono::milliseconds increment_;
    GameState current_state;

    // TODO: limit the size of this data structure
    //
    // We have a vector of prioritiy queues, one for each player, sorted by
    // the sequence number of the given command.
    //
    // This allows us to attempt to faithfully execute commands by each user in
    // the order they specified them.
    std::vector<std::priority_queue<Command, std::vector<Command>, seq_comp>> command_queues_;

    // Per player counts of remaining time.
    std::vector<std::chrono::milliseconds> time_left_;

    // Per player views of the game.
    std::vector<Environment> player_views_;

    // Callback function to send message to a player's session
    SendCallback send_callback_;

};
