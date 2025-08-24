#pragma once

#include "cryptography-constants.h"
#include "gamemodes.h"
#include "external-user.h"

#include <boost/uuid/uuid.hpp>
#include <array>
#include <vector>

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

// Information sent at the start of a match that will not change.
struct StaticMatchData
{
    // List of users indexed by their player IDs.
    UserList player_list;
};
