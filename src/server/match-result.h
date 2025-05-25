#pragma once

#include <vector>
#include <boost/uuid/uuid.hpp>

#include "command.h"
#include "match-settings.h"

// Exists in message.h
enum class GameMode : uint8_t;

struct MatchResult
{
    MatchResult(uint8_t num_players, MatchSettings game_settings)
    :move_history{},
    user_ids(num_players),
    elimination_order(num_players),
    settings(game_settings)
    {

    }

    std::vector<CommandHead> move_history;
    std::vector<boost::uuids::uuid> user_ids;
    std::vector<uint8_t> elimination_order;
    MatchSettings settings;

};
