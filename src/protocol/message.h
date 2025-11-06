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

#include "header.h"
#include "player-view.h"
#include "command.h"
#include "match-result-structs.h"
#include "message-structs.h"

#include <type_traits>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <bit>

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
    a[static_cast<uint8_t>(GameMode::ClassicThreePlayer)] = 3;
    a[static_cast<uint8_t>(GameMode::ClassicFivePlayer)] = 5;

    return a;
}();

constexpr uint8_t MAX_PLAYERS = players_for_gamemode[static_cast<uint8_t>(
                                    GameMode::ClassicFivePlayer)];

constexpr int LATEST_MATCHES_COUNT = 10;

struct Message
{
public:
    bool valid_matching_command() const;

    std::array<int, RANKED_MODES_COUNT> to_elos();

    LoginRequest to_login_request() const;

    Command to_command();

    StaticMatchData to_static_match_data(bool & op_status);

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

    MatchReplay to_match_replay(bool & op_status);

    template <typename mType>
    void create_serialized(const mType & req);

    void create_serialized(HeaderType h_type);

public:
    Header header;
    std::vector<uint8_t> payload;
};
