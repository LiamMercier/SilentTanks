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
};
