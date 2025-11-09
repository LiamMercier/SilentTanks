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

#include "maps.h"

// Exists in message.h
enum class GameMode : uint8_t;

// MatchSettings is used when creating a new game instance.
struct MatchSettings
{
    MatchSettings(GameMap arg_map,
                  uint64_t init_time_ms,
                  uint64_t inc,
                  GameMode queued_mode)
    :map(arg_map),
    initial_time_ms(init_time_ms),
    increment_ms(inc),
    mode(queued_mode)
    {}

    GameMap map;
    uint64_t initial_time_ms;
    uint64_t increment_ms;
    GameMode mode;

};
