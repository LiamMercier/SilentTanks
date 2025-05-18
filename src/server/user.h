#pragma once

#include "user-data.h"
#include "session.h"

#include <boost/uuid/uuid.hpp>
#include <string>


struct User
{
    User()
    :user_data()
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
