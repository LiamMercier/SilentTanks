#pragma once

#include "maps.h"

#include <random>
#include <shared_mutex>
#include <mutex>

class MapRepository
{
public:
    MapRepository();

    void load_map_file(std::string map_file_name);

    const GameMap & get_random_map(uint8_t index) const;

private:
    // Controls map access. Mostly to prevent reading before
    // initialization at server startup.
    mutable std::shared_mutex maps_mutex_;
    std::vector<std::vector<GameMap>> maps_;

    // For getting a random map, pseudorandom is fine.
    // We want this to be thread local for different match strategies.
    static thread_local std::mt19937 gen_;
};
