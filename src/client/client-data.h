#pragma once

#include "external-user.h"

#include <unordered_map>
#include <boost/functional/hash.hpp>

struct ClientData
{
    void load_user_list(UserList friends,
                        UserListType list_type);

    std::string client_username;

    // If iteration is bottlenecked, we could potentially store both a
    // vector and a map to speed both up, since client memory is cheap vs
    // computation power (only so many friends to store).
    std::unordered_map<boost::uuids::uuid,
                       ExternalUser,
                       boost::hash<boost::uuids::uuid>> friends;

    std::unordered_map<boost::uuids::uuid,
                       ExternalUser,
                       boost::hash<boost::uuids::uuid>> friend_requests;

    std::unordered_map<boost::uuids::uuid,
                       ExternalUser,
                       boost::hash<boost::uuids::uuid>> blocked_users;
};
