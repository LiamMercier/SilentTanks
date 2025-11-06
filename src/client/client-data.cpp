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

#include "client-data.h"

void ClientData::load_user_list(UserList user_list,
                                UserListType list_type)
{
    switch (list_type)
    {
        case UserListType::Friends:
        {
            for (const auto & user : user_list.users)
            {
                friends[user.user_id] =  user;
            }
            break;
        }
        case UserListType::FriendRequests:
        {
            for (const auto & user : user_list.users)
            {
                friend_requests[user.user_id] =  user;
            }
            break;
        }
        case UserListType::Blocks:
        {
            for (const auto & user : user_list.users)
            {
                blocked_users[user.user_id] =  user;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}
