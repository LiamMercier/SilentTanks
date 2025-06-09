#pragma once

#include "header.h"
#include "environment.h"
#include "player-view.h"
#include "command.h"
#include "external-user.h"

#include <type_traits>
#include <vector>
#include <array>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <bit>
#include <boost/uuid/uuid.hpp>

constexpr size_t HASH_LENGTH = 32;

// Construct a lookup table of valid username characters.
constexpr std::array<bool, 256> allowed_username_characters = []
{
    std::array<bool, 256> a{};
    for (char c = '0'; c <= '9'; c++)
    {
        a[static_cast<unsigned char>(c)] = true;
    }
    for (char c = 'A'; c <= 'Z'; c++)
    {
        a[static_cast<unsigned char>(c)] = true;
    }
    for (char c = 'a'; c <= 'z'; c++)
    {
        a[static_cast<unsigned char>(c)] = true;
    }

    a['_'] = true;
    a['-'] = true;

    return a;
}();

// This must be ordered so that all ranked modes are together.
enum class GameMode : uint8_t
{
    ClassicTwoPlayer = 0,
    RankedTwoPlayer,
    NO_MODE
};

// First GameMode which is ranked, for fast checks of a game mode.
constexpr uint8_t RANKED_MODES_START = static_cast<uint8_t>(GameMode::RankedTwoPlayer);

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

struct BanMessage
{
    std::chrono::system_clock::time_point time_till_unban;
    std::string reason;
};

struct UserList
{
    std::vector<ExternalUser> users;
};

struct FriendDecision
{
    boost::uuids::uuid user_id;
    bool decision;
};

struct Message
{
public:
    bool valid_matching_command() const;

    LoginRequest to_login_request() const;

    Command to_command();

    // Modify view with success return type.
    PlayerView to_player_view(bool & op_status);

    BanMessage to_ban_message();

    std::string to_username();

    boost::uuids::uuid to_uuid();

    template <typename mType>
    void create_serialized(const mType & req);

    void create_serialized(HeaderType h_type);

public:
    Header header;
    std::vector<uint8_t> payload;
};
