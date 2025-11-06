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
#include <cstdint>

// Default elo, must match create_tables.sql default if changed.
constexpr int DEFAULT_ELO = 1500;

constexpr int K_TWO_PLAYERS = 32;

constexpr int ELO_FLOOR = 500;

constexpr double ELO_DIFF_UPPER = 7.0;

constexpr double ELO_DIFF_LOWER = -7.0;

std::vector<int> elo_updates(const std::vector<int> & initial_elos,
                             const std::vector<uint8_t> & placement);
