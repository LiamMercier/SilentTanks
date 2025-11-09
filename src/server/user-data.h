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

#include "message.h"

#include <boost/uuid/uuid.hpp>
#include <string>
#include <array>

struct UserData
{
    UserData()
    :user_id()
    {

    }

    inline int get_elo(GameMode mode)
    {
        return matching_elos[elo_ranked_index(mode)];
    }

    boost::uuids::uuid user_id;
    std::string username;
    std::array<int, RANKED_MODES_COUNT> matching_elos;
};
