#pragma once

#include <boost/uuid/uuid.hpp>

struct ExternalUser
{
    boost::uuids::uuid user_id;
    std::string username;
};
