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

#include "user-data.h"
#include "session.h"

#include <boost/uuid/uuid.hpp>
#include <boost/functional/hash.hpp>
#include <string>
#include <unordered_set>

class MatchInstance;

struct User
{
    using UUIDHashSet = std::unordered_set<boost::uuids::uuid,
                                           boost::hash<boost::uuids::uuid>>;

    User()
    :user_data()
    {
    }

    User(UserData data,
         UUIDHashSet friends_,
         UUIDHashSet blocks_)
    :user_data(std::move(data)),
    friends(std::move(friends_)),
    blocked_users(std::move(blocks_))
    {
    }

    UserData user_data;
    std::shared_ptr<Session> current_session;
    std::weak_ptr<MatchInstance> current_match;
    UUIDHashSet friends;
    UUIDHashSet blocked_users;
};
