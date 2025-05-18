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
