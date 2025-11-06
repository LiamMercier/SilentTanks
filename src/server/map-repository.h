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

#include <random>
#include <shared_mutex>
#include <mutex>

class MapRepository
{
public:
    MapRepository();

    void load_map_file(std::string map_file_name);

    const GameMap & get_random_map(uint8_t index) const;

private:
    // Controls map access. Mostly to prevent reading before
    // initialization at server startup.
    mutable std::shared_mutex maps_mutex_;
    std::vector<std::vector<GameMap>> maps_;

    // For getting a random map, pseudorandom is fine.
    // We want this to be thread local for different match strategies.
    static thread_local std::mt19937 gen_;
};
