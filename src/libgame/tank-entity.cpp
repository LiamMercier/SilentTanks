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

#include "tank-entity.h"
#include <iostream>
#include <string>

Tank::Tank()
:pos_(NO_POS_VEC),
current_direction_(0),
barrel_direction_(0),
health_(0),
aim_focused_(false),
loaded_(false),
owner_(NO_OWNER)
{

}

Tank::Tank(uint8_t x_start, uint8_t y_start, uint8_t start_dir, uint8_t owner)
:pos_(x_start, y_start), current_direction_(start_dir), barrel_direction_(start_dir), health_(INITIAL_HEALTH), aim_focused_(false), loaded_(true), owner_(owner)
{}

void Tank::deal_damage(uint8_t damage)
{
    if (damage > health_){
        health_ = 0;
    }
    else
    {
        health_ = health_ - damage;
    }
}

void Tank::repair(uint8_t repair_amount)
{
    if (health_ + repair_amount > health_){
        health_ = INITIAL_HEALTH;
    }
    else
    {
        health_ = health_ + repair_amount;
    }
}
