#pragma once

#include "header.h"
#include "environment.h"
#include "player-view.h"

#include <type_traits>
#include <vector>
#include <cstring>

enum class GameMode : uint8_t
{
    ClassicTwoPlayer = 0,
    RankedTwoPlayer,
    NO_MODE
};

struct QueueMatchRequest
{
    QueueMatchRequest(GameMode input_mode)
    :mode(input_mode)
    {
    }

    GameMode mode;
};

struct CancelMatchRequest
{
    CancelMatchRequest(GameMode input_mode)
    :mode(input_mode)
    {
    }

    GameMode mode;
};

struct MatchStartNotification
{
    uint8_t player_id;
};

enum class CommandType : uint8_t
{
    Move,
    RotateTank,
    RotateBarrel,
    Fire,
    Place,
    Load,
    NO_OP
};

// Command structure
struct Command
{
    uint8_t sender;
    CommandType type;
    uint8_t tank_id;
    // Primary place to put a command parameter.
    uint8_t payload_first;
    // Mostly used to give a second x coordinate for placement.
    uint8_t payload_second;
    // Necessary since the client is asynchronous.
    uint16_t sequence_number;

    static constexpr std::size_t COMMAND_SIZE = 5 * sizeof(uint8_t)
                                                + sizeof(uint16_t);
};

// TODO: login request format when feature is to be added
struct LoginRequest
{
    uint64_t temp;
};

struct Message
{
public:
    Command to_command();

    // Modify view with success return type.
    PlayerView to_player_view(bool & op_status);

    bool valid_matching_command() const;

    template <typename mType>
    void create_serialized(const mType & req);

    void create_serialized(HeaderType h_type);

public:
    Header header;
    std::vector<uint8_t> payload;
};
