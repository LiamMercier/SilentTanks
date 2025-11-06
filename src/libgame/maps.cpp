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

#include "maps.h"

MapSettings::MapSettings(std::string file,
                         uint8_t w,
                         uint8_t h,
                         uint8_t n_tanks,
                         uint8_t n_players,
                         uint8_t mode)
:filename(std::move(file)),
width(w),
height(h),
num_tanks(n_tanks),
num_players(n_players),
mode(mode)
{

}

GameMap::GameMap(MapSettings settings)
:map_settings(settings),
env(settings.width, settings.height),
mask(settings.width * settings.height)
{

}
