#pragma once

#include "gamemodes.h"
#include "command.h"
#include "match-settings.h"
#include "external-user.h"

struct MatchResultRow
{
    int64_t match_id;
    std::chrono::system_clock::time_point finished_at;
    uint16_t placement;
    int32_t elo_change;

    static constexpr std::size_t DATA_SIZE = sizeof(match_id)
                                             // Time size.
                                             + sizeof(int64_t)
                                             + sizeof(placement)
                                             + sizeof(elo_change);
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
    UserList players;
    MapSettings settings;
    uint64_t initial_time_ms;
    uint64_t increment_ms;
    uint64_t match_id;
};
