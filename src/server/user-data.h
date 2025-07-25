#pragma once

#include "message.h"

#include <boost/uuid/uuid.hpp>
#include <string>
#include <array>

struct UserData
{
    UserData()
    :user_id()
    {

    }
    boost::uuids::uuid user_id;
    std::string username;
    std::array<int, RANKED_MODES_COUNT> matching_elos;
};
