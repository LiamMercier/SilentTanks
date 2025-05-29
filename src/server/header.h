#pragma once
#include <cstdint>

// TODO: setup winsock in the main compilation unit
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

enum class HeaderType : uint8_t
{
    LoginRequest,
    RegistrationRequest,
    Unauthorized,
    AlreadyAuthorized,
    GoodRegistration,
    BadRegistration,
    BadAuth,
    GoodAuth,
    QueueMatch,
    CancelMatch,
    ForfeitMatch,
    BadCancel,
    QueueDropped,
    MatchStarting,
    NoMatchFound,
    MatchInProgress,
    SendCommand,
    PlayerView,
    Text,
    FailedMove,
    StaleMove,
    Eliminated,
    TimedOut,
    Victory,
    MAX_TYPE
};

struct Header
{
    inline bool valid();
    inline Header to_network();
    inline Header from_network();

    HeaderType type_;
    uint32_t payload_len;
};

inline bool Header::valid()
{
    if (type_ >= HeaderType::MAX_TYPE)
    {
        return false;
    }
    // TODO: make this more defined/reasonable
    if (payload_len > 3000)
    {
        return false;
    }

    return true;
}

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
