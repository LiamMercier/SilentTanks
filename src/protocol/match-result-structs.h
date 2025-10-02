#pragma once

#include "gamemodes.h"
#include "command.h"
#include "match-settings.h"

struct MatchResultRow
{
    int64_t match_id;
    std::chrono::system_clock::time_point finished_at;
    uint16_t placement;
    int32_t elo_change;

    static constexpr std::size_t DATA_SIZE = sizeof(int64_t)
                                             // Time size.
                                             + sizeof(int64_t)
                                             + sizeof(uint16_t)
                                             + sizeof(int32_t);
};

struct MatchResultList
{
    GameMode mode;
    std::vector<MatchResultRow> match_results;
};

struct MatchReplay
{
    size_t get_size_in_bytes()
    {
        return moves.size()
               + settings.get_size_in_bytes()
               + sizeof(initial_time_ms)
               + sizeof(increment_ms)
               + sizeof(match_id);
    }

    std::vector<CommandHead> moves;
    MapSettings settings;
    uint64_t initial_time_ms;
    uint64_t increment_ms;
    uint64_t match_id;
};
