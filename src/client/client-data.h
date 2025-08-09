#pragma once

#include "external-user.h"

#include <unordered_map>
#include <boost/functional/hash.hpp>

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
};
