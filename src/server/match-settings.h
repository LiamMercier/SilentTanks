#pragma once

#include "maps.h"
#include <glaze/glaze.hpp>

// Exists in message.h
enum class GameMode : uint8_t;

struct MatchSettings
{
    MatchSettings(GameMap arg_map,
                  uint64_t init_time_ms,
                  uint64_t inc,
                  GameMode queued_mode)
    :map(arg_map),
    initial_time_ms(init_time_ms),
    increment_ms(inc),
    mode(queued_mode){}

    GameMap map;
    uint64_t initial_time_ms;
    uint64_t increment_ms;
    GameMode mode;

};

namespace glz {

template <>
struct meta<GameMap> {
    using T = GameMap;
    static constexpr auto value = object(
        &T::filename,
        &T::width,
        &T::height,
        &T::num_tanks
    );
};

};

namespace glz {

template <>
struct meta<MatchSettings> {
    using T = MatchSettings;
    static constexpr auto value = object(
        &T::map,
        &T::initial_time_ms,
        &T::increment_ms
    );
};

};
