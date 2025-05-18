#pragma once

#include "header.h"
#include "environment.h"
#include "player-view.h"
#include "command.h"

#include <type_traits>
#include <vector>
#include <cstring>
#include <algorithm>

constexpr HASH_LENGTH = 32;

constexpr MAX_USERNAME_LENGTH = 20;

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

struct LoginRequest
{
    std::array<uint8_t, HASH_LENGTH> hash;
    std::string username;
};

struct Message
{
public:
    LoginRequest to_login_request();

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
