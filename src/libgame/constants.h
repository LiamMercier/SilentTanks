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
#include "vec2.h"

constexpr uint8_t INITIAL_HEALTH = 3;

constexpr uint8_t TURN_PLAYER_FUEL = 3;

constexpr uint8_t FIRING_DIST_DIAGONAL = 3;

constexpr uint8_t FIRING_DIST_HORIZONTAL = 4;

constexpr uint8_t SHELL_DAMAGE = 1;

constexpr uint8_t NO_OWNER = UINT8_MAX;
constexpr uint8_t NO_OCCUPANT = UINT8_MAX;
constexpr uint8_t NO_TANK = UINT8_MAX;
constexpr uint8_t NO_PLAYER = UINT8_MAX;

constexpr uint8_t NO_POS = UINT8_MAX;
constexpr vec2 NO_POS_VEC = vec2(UINT8_MAX, UINT8_MAX);

// lookup table for converting from directions to vectors
constexpr vec2 dir_to_vec[8]
{
    vec2(0, -1), vec2(1, -1), vec2(1, 0),
    vec2(1, 1), vec2(0, 1), vec2(-1, 1),
    vec2(-1, 0), vec2(-1, -1)
};

// Horizontal slopes for the east direction
//
// These are altered to create north, south, and west variants
constexpr float orthogonal_slopes[7]
{
    1.0f, 0.5f, 1.0f/3.0f, 0.0f, -1.0f/3.0f, -0.5f, -1.0f
};

// Precomputed ray distances for north, south, east, west
constexpr int orthogonal_ray_dists[7]
{
    2, 2, 3, 4, 3, 2, 2
};

// slopes for south-east direction
constexpr vec2 diagonal_slopes[9]
{
    vec2(0,1), vec2(1,3), vec2(1,2),
    vec2(2,3), vec2(1,1), vec2(3, 2),
    vec2(2, 1), vec2(3,1), vec2(1, 0)
};

// approximate L2 norm sizes for each vector
constexpr float diagonal_vec_size[9]
{
    1.0f, 3.16227766f, 2.2360679f,
    3.6055513f, 1.4142136f, 3.6055513f,
    2.2360679f, 3.16227766f, 1.0f
};

// predefined distances to advance the ray
constexpr float diagonal_ray_dists[9]
{
    2.0f, 1.0f,1.0f,1.0f,3.0f,1.0f,1.0f,1.0f,2.0f
};
