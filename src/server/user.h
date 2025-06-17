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
