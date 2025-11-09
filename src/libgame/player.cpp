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

#include "player.h"

Player::Player(uint8_t num_tanks, uint8_t ID)
:tanks_placed_(0),
owned_tank_IDs_(num_tanks, NO_TANK),
player_ID_(ID)
{
}

std::vector<int> & Player::get_tanks_list()
{
    return owned_tank_IDs_;
}

const std::vector<int> & Player::get_tanks_list() const
{
    return owned_tank_IDs_;
}
