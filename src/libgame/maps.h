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

#include <string>
#include <vector>

#include "grid-cell.h"
#include "flat-array.h"

struct MapSettings
{
public:
    MapSettings(std::string file,
                uint8_t w,
                uint8_t h,
                uint8_t n_tanks,
                uint8_t n_players,
                uint8_t mode);

    MapSettings()
    :filename(""),
    width(0),
    height(0),
    num_tanks(0),
    num_players(0),
    mode(0)
    {
    }

    size_t get_size_in_bytes()
    {
        return filename.size()
               + sizeof(width)
               + sizeof(height)
               + sizeof(num_tanks)
               + sizeof(num_players)
               + sizeof(mode);
    }

public:
    std::string filename;
    uint8_t width;
    uint8_t height;
    // tanks per person
    uint8_t num_tanks;
    // number of players
    uint8_t num_players;

    // Type of map, linked to GameMode.
    uint8_t mode;

};

struct GameMap
{
public:
    GameMap(MapSettings settings);

    GameMap()
    :map_settings()
    {}

public:
    MapSettings map_settings;
    FlatArray<GridCell> env;
    std::vector<uint8_t> mask;

};
