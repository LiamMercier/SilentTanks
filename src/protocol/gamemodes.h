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

// This must be ordered so that all ranked modes are together.
enum class GameMode : uint8_t
{
    ClassicTwoPlayer = 0,
    ClassicThreePlayer,
    ClassicFivePlayer,
    RankedTwoPlayer,
    NO_MODE
};

// First GameMode which is ranked, for fast checks of a game mode.
constexpr uint8_t RANKED_MODES_START = static_cast<uint8_t>(GameMode::RankedTwoPlayer);

constexpr uint8_t NUMBER_OF_MODES = static_cast<uint8_t>(GameMode::NO_MODE);

constexpr uint8_t RANKED_MODES_COUNT = static_cast<uint8_t>(GameMode::NO_MODE)
                                       - RANKED_MODES_START;

// Helpers to get the index for elo vectors.
inline uint8_t elo_ranked_index(GameMode m)
{
    uint8_t idx = static_cast<uint8_t>(m);
    return idx - RANKED_MODES_START;
}

inline uint8_t elo_ranked_index(uint8_t m)
{
    return m - RANKED_MODES_START;
}
