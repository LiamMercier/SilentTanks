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

#include "external-user.h"
#include "gamemodes.h"

#include <unordered_map>
#include <boost/functional/hash.hpp>
#include <array>

using UserMap = std::unordered_map<boost::uuids::uuid,
                       ExternalUser,
                       boost::hash<boost::uuids::uuid>>;

struct ClientData
{
    void load_user_list(UserList user_list,
                        UserListType list_type);

    std::string client_username;

    // If iteration is bottlenecked, we could potentially store both a
    // vector and a map to speed both up, since client memory is cheap vs
    // computation power (only so many friends to store).
    UserMap friends;

    UserMap friend_requests;

    UserMap blocked_users;

    std::array<int, RANKED_MODES_COUNT> display_elos;
};
