// Copyright (c) 2025 Liam Mercier
//
// This file is part of SilentTanks.
//
// SilentTanks is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License Version 3.0
// as published by the Free Software Foundation.
//
// SilentTanks is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License v3.0
// for more details.
//
// You should have received a copy of the GNU Affero General Public License v3.0
// along with SilentTanks. If not, see <https://www.gnu.org/licenses/agpl-3.0.txt>

#pragma once
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "gamemodes.h"

enum class HeaderType : uint8_t
{
    LoginRequest,
    RegistrationRequest,

    // Server messages to client for auth/register.
    Unauthorized,
    AlreadyAuthorized,
    GoodRegistration,
    BadRegistration,
    GoodAuth,
    BadAuth,

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
    NotifyFriendRequest,
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
    StaticMatchData,
    PlayerView,//
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
    ServerFull,

    // Communication.
    DirectTextMessage,
    MatchTextMessage,
    FriendOffline,

    // Match data related headers.
    FetchMatchHistory,
    MatchHistory,
    NoNewMatches,
    MatchReplayRequest,
    MatchReplay,
    NoReplay,

    MAX_TYPE
};

static constexpr uint32_t MAX_PAYLOAD_LEN = 3000;

static constexpr uint32_t MAX_CLIENT_PAYLOAD_LEN = 8000;

constexpr size_t MAX_USERNAME_LENGTH = 24;

struct Header
{
    bool valid_server();

    bool valid_client();

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
