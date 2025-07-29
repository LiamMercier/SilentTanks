#pragma once
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

enum class HeaderType : uint8_t
{
    LoginRequest,
    RegistrationRequest,

    // Server messages to client for auth/register.
    Unauthorized,
    AlreadyAuthorized,
    GoodRegistration,
    BadRegistration,
    BadAuth,
    GoodAuth,

    // Fetch requests when the client connects.
    FetchFriends,
    FetchFriendRequests,
    FetchBlocks,

    // Server list callback headers.
    FriendList,
    FriendRequestList,
    BlockList,

    // User requests for friending/blocking.
    SendFriendRequest,
    RespondFriendRequest,
    RemoveFriend,
    BlockUser,
    UnblockUser,

    // Notifications for friending/blocking.
    NotifyFriendAdded,
    NotifyFriendRemoved,
    NotifyBlocked,
    NotifyUnblocked,

    // Queue related requests.
    QueueMatch,
    BadQueue,
    CancelMatch,
    ForfeitMatch,
    BadCancel,
    QueueDropped,

    // Game related headers
    MatchStarting,
    MatchCreationError,
    NoMatchFound,
    MatchInProgress,
    SendCommand,
    PlayerView,
    FailedMove,
    StaleMove,
    Eliminated,
    TimedOut,
    Victory,
    GameEnded,

    // Client server management headers.
    BadMessage,
    Ping,
    PingResponse,
    PingTimeout,
    RateLimited,
    Banned,

    // Communication.
    DirectTextMessage,
    MatchTextMessage,

    MAX_TYPE
};

static constexpr uint32_t MAX_PAYLOAD_LEN = 3000;

constexpr size_t MAX_USERNAME_LENGTH = 30;

struct Header
{
    bool valid();
    inline Header to_network();
    inline Header from_network();

    HeaderType type_;
    uint32_t payload_len;
};

inline Header Header::to_network()
{
    // We likely will not create a new header when this is optimized.
    Header return_header;
    return_header.type_ = type_;
    return_header.payload_len = htonl(payload_len);
    return return_header;
}

inline Header Header::from_network()
{
    // We likely will not create a new header when this is optimized.
    Header return_header;
    return_header.type_ = type_;
    return_header.payload_len = ntohl(payload_len);
    return return_header;
}
