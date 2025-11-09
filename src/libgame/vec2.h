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

struct vec2
{
public:
    vec2()
    :x_(0), y_(0)
    {}

    constexpr vec2(uint8_t x, uint8_t y)
    :x_(x), y_(y)
    {}

    inline vec2& operator+=(vec2 const & rhs);

    inline friend vec2 operator+(vec2 lhs, vec2 const & rhs);

    uint8_t x_;
    uint8_t y_;
};

inline vec2& vec2::operator+=(vec2 const & rhs)
{
        x_ += rhs.x_;
        y_ += rhs.y_;
        return *this;
}

inline vec2 operator+(vec2 lhs, vec2 const & rhs)
{
    lhs += rhs;
    return lhs;
}
