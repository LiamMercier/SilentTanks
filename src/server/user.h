#pragma once

#include <boost/uuid/uuid.hpp>
#include <string>

struct UserData
{
    UserData()
    :user_id()
    {

    }
    boost::uuids::uuid user_id;
    std::string username;
};

struct User
{
    User()
    :user_id()
    {
    }

    User(UserData data)
    :user_data(std::move(data))
    {
    }

    UserData user_data;
    std::shared_ptr<Session> current_session;
    std::weak_ptr<MatchInstance> current_match;
};
