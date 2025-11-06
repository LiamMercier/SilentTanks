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

#include <boost/uuid/uuid.hpp>
#include <string>
#include <vector>

constexpr bool ACCEPT_FRIEND_REQUEST = true;

enum class UserListType : uint8_t
{
    Friends,
    FriendRequests,
    Blocks
};

struct ExternalUser
{
    boost::uuids::uuid user_id;
    std::string username;
};

struct UserList
{
    std::vector<ExternalUser> users;

    size_t size() const
    {
        return users.size();
    }
};
