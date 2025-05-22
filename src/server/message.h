#pragma once

#include "header.h"
#include "environment.h"
#include "player-view.h"
#include "command.h"

#include <type_traits>
#include <vector>
#include <array>
#include <cstring>
#include <algorithm>

constexpr size_t HASH_LENGTH = 32;

constexpr size_t MAX_USERNAME_LENGTH = 20;

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

struct BadRegNotification
{
    enum class Reason : uint8_t
    {
        NotUnique = 0,
        InvalidUsername,
        CurrentlyAuthenticated,
        ServerError
    };

    BadRegNotification(Reason input_reason)
    :reason(input_reason)
    {
    }

    Reason reason;
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

struct RegisterRequest
{
    std::array<uint8_t, HASH_LENGTH> hash;
    std::string username;
};

struct Message
{
public:
    LoginRequest to_login_request() const;

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
