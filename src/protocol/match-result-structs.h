#pragma once

#include "gamemodes.h"

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
