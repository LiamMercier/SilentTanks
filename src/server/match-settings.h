#pragma once

#include "maps.h"

// Exists in message.h
enum class GameMode : uint8_t;

// MatchSettings is used when creating a new game instance.
struct MatchSettings
{
    MatchSettings(GameMap arg_map,
                  uint64_t init_time_ms,
                  uint64_t inc,
                  GameMode queued_mode)
    :map(arg_map),
    initial_time_ms(init_time_ms),
    increment_ms(inc),
    mode(queued_mode)
    {}

    GameMap map;
    uint64_t initial_time_ms;
    uint64_t increment_ms;
    GameMode mode;

};
