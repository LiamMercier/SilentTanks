#pragma once

#include <vector>
#include <boost/uuid/uuid.hpp>

#include "command.h"

struct MatchResult
{
    std::vector<CommandHead> move_history;
    std::vector<boost::uuids::uuid> player_ids;
    uint8_t winner_idx;
};
