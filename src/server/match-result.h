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

#include <vector>
#include <boost/uuid/uuid.hpp>
#include <glaze/glaze.hpp>

#include "command.h"
#include "maps.h"

// Exists in message.h
enum class GameMode : uint8_t;

struct MatchResult
{
    MatchResult(MapSettings settings,
                uint64_t init_ms,
                uint64_t inc_ms)
    :move_history(),
    user_ids(settings.num_players),
    elimination_order(settings.num_players),
    settings(settings),
    initial_time_ms(init_ms),
    increment_ms(inc_ms)
    {

    }

    MatchResult()
    :move_history(),
    user_ids(),
    elimination_order(),
    settings(),
    initial_time_ms(0),
    increment_ms(0)
    {

    }

    std::vector<CommandHead> move_history;
    // user_id's and elimination_order are both indexed by player_id
    std::vector<boost::uuids::uuid> user_ids;
    std::vector<uint8_t> elimination_order;
    MapSettings settings;
    uint64_t initial_time_ms;
    uint64_t increment_ms;

};

namespace glz {

template <>
struct meta<MapSettings> {
    using T = MapSettings;
    static constexpr auto value = object(
        &T::filename,
        &T::width,
        &T::height,
        &T::num_tanks,
        &T::num_players,
        &T::mode
    );
};

};

namespace glz {

template <>
struct meta<MatchResult> {
    using T = MatchResult;
    static constexpr auto value = object(
        "map_settings", &T::settings,
        "initial_time", &T::initial_time_ms,
        "increment", &T::increment_ms
    );
};

};

