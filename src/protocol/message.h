#pragma once

#include "header.h"
#include "player-view.h"
#include "command.h"
#include "external-user.h"
#include "cryptography-constants.h"
#include "match-result-structs.h"

#include <type_traits>
#include <vector>
#include <array>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <bit>
#include <boost/uuid/uuid.hpp>

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

constexpr std::array<uint8_t, NUMBER_OF_MODES> players_for_gamemode = []
{
    std::array<uint8_t, NUMBER_OF_MODES> a{};

    a[static_cast<uint8_t>(GameMode::ClassicTwoPlayer)] = 2;
    a[static_cast<uint8_t>(GameMode::RankedTwoPlayer)] = 2;

    return a;
}();

constexpr int LATEST_MATCHES_COUNT = 10;

struct GoodAuthNotification
{
    GoodAuthNotification(std::array<int, RANKED_MODES_COUNT> elo_array)
    :elos(elo_array)
    {
    }

    std::array<int, RANKED_MODES_COUNT> elos;
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

struct MatchHistoryRequest
{
    MatchHistoryRequest(GameMode input_mode)
    :mode(input_mode)
    {
    }

    GameMode mode;
};

struct ReplayRequest
{
    ReplayRequest(uint64_t id)
    :match_id(id)
    {
    }

    uint64_t match_id;
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

struct BadAuthNotification
{
    enum class Reason : uint8_t
    {
        BadHash,
        InvalidUsername,
        CurrentlyAuthenticated,
        ServerError
    };

    BadAuthNotification(Reason input_reason)
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

struct FriendRequest
{
    std::string username;
};

struct FriendDecision
{
    boost::uuids::uuid user_id;
    bool decision;
};

struct UnfriendRequest
{
    boost::uuids::uuid user_id;
};

struct BlockRequest
{
    std::string username;
};

struct UnblockRequest
{
    boost::uuids::uuid user_id;
};

struct NotifyRelationUpdate
{
    ExternalUser user;
};

struct TextMessage
{
    // Sender/receiver based on context.
    boost::uuids::uuid user_id;
    std::string text;
};

struct InternalMatchMessage
{
    boost::uuids::uuid user_id;
    std::string sender_username;
    std::string text;
};

struct ExternalMatchMessage
{
    boost::uuids::uuid user_id;
    uint8_t username_length;
    std::string sender_username;
    std::string text;
};

struct Message
{
public:
    bool valid_matching_command() const;

    std::array<int, RANKED_MODES_COUNT> to_elos();

    LoginRequest to_login_request() const;

    Command to_command();

    // Modify view with success return type.
    PlayerView to_player_view(bool & op_status);

    BanMessage to_ban_message();

    UserList to_user_list(bool & op_status);

    std::string to_username();

    boost::uuids::uuid to_uuid();

    FriendDecision to_friend_decision();

    ExternalUser to_user();

    TextMessage to_text_message();

    InternalMatchMessage to_match_message();

    GameMode to_gamemode();

    MatchResultList to_results_list();

    ReplayRequest to_replay_request();

    template <typename mType>
    void create_serialized(const mType & req);

    void create_serialized(HeaderType h_type);

public:
    Header header;
    std::vector<uint8_t> payload;
};
