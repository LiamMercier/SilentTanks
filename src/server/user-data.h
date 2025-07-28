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

    inline int get_elo(GameMode mode)
    {
        return matching_elos[elo_ranked_index(mode)];
    }

    boost::uuids::uuid user_id;
    std::string username;
    std::array<int, RANKED_MODES_COUNT> matching_elos;
};
