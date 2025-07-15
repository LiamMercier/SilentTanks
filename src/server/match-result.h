#pragma once

#include <vector>
#include <boost/uuid/uuid.hpp>
#include <glaze/glaze.hpp>

#include "command.h"
#include "maps.h"

// Exists in message.h
enum class GameMode : uint8_t;

struct MatchResult
{
    MatchResult(MapSettings settings,
                uint64_t init_ms,
                uint64_t inc_ms)
    :move_history(),
    user_ids(settings.num_players),
    elimination_order(settings.num_players),
    settings(settings),
    initial_time_ms(init_ms),
    increment_ms(inc_ms)
    {

    }

    std::vector<CommandHead> move_history;
    std::vector<boost::uuids::uuid> user_ids;
    std::vector<uint8_t> elimination_order;
    MapSettings settings;
    uint64_t initial_time_ms;
    uint64_t increment_ms;

};

namespace glz {

template <>
struct meta<MapSettings> {
    using T = MapSettings;
    static constexpr auto value = object(
        &T::filename,
        &T::width,
        &T::height,
        &T::num_tanks,
        &T::num_players,
        &T::mode
    );
};

};

namespace glz {

template <>
struct meta<MatchResult> {
    using T = MatchResult;
    static constexpr auto value = object(
        "map_settings", &T::settings,
        "initial_time", &T::initial_time_ms,
        "increment", &T::increment_ms
    );
};

};

