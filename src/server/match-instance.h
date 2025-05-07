#pragma once

#include <vector>
#include <queue>
#include <chrono>
#include <condition_variable>
#include "game-instance.h"
#include "maps.h"
#include "constants.h"

// Player information struct for usage by MatchInstance
struct PlayerInfo
{
public:
    PlayerInfo(uint8_t id);

public:
    uint8_t PlayerID;
    bool alive;
};

enum class GameState : uint8_t
{
    Setup,
    Play,
    Concluded
};

enum class CommandType : uint8_t
{
    Move,
    RotateTank,
    RotateBarrel,
    Fire,
    Place,
    Load
};

// Command structure
struct Command
{
    uint8_t sender;
    CommandType type;
    uint8_t tank_id;
    // optional field for direction of rotation or tank pos x
    uint8_t payload;
    // optional field for pos y
    uint8_t payload_optional;

    uint32_t sequence_number;
};

struct ApplyResult {
    bool valid_move;
    bool op_status;
};

class MatchInstance
{
public:

    MatchInstance() = delete;

    MatchInstance(const MatchSettings & settings,
                  std::vector<PlayerInfo> player_list,
                  uint8_t num_players);

    // function to be called by networking layer to enqueue commands
    void receive_command(const Command& cmd);

    void setup_loop();

    uint8_t game_loop();

    void handle_timeout();

private:
    struct seq_comp {
        bool operator()(Command const& a, Command const& b) const {
            // priority_queue is a max-heap, invert this
            return a.sequence_number > b.sequence_number;
        }
    };

    // Block game_loop until:
    // - a command appears from a network thread
    // - player clock hits zero (return false, failed to move)
    bool wait_player_move(uint8_t player_id, Command &out_cmd);

    ApplyResult apply_command(const Command & cmd);

    void compute_all_views();

public:
    uint8_t current_player;
    uint8_t remaining_players;
    uint8_t current_fuel;

private:
    GameInstance game_instance_;
    std::vector<PlayerInfo> players_;
    uint8_t n_players_;
    uint8_t tanks_placed;
    std::chrono::milliseconds increment_;
    GameState current_state;

    // We have a vector of prioritiy queues, one for each player, sorted by
    // the sequence number of the given command.
    //
    // This allows us to attempt to faithfully execute commands by each user in
    // the order they specified them.
    std::vector<std::priority_queue<Command, std::vector<Command>, seq_comp>> command_queues_;

    // We give each player a mutex for their priority queue
    std::vector<std::mutex> queue_mutex_;

    // condition variables for each player
    std::vector<std::condition_variable> queue_cvs_;

    // per player clocks
    std::vector<std::chrono::milliseconds> time_left_;

    //
    std::vector<Environment> player_views_;

};
