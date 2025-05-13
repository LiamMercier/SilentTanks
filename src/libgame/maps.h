#pragma once

#include <string>
#include <vector>

struct GameMap
{
public:
    GameMap(std::string file, uint8_t w, uint8_t h, uint8_t n_tanks);

public:
    std::string filename;
    uint8_t width;
    uint8_t height;
    // tanks per person
    uint8_t num_tanks;
};

struct MatchSettings
{
    MatchSettings(GameMap map, uint64_t init_time_ms, uint64_t inc);
    GameMap map;
    uint64_t initial_time_ms;
    uint64_t increment_ms;
};

class MapRepository
{
public:
    MapRepository() = delete;

    MapRepository(std::vector<GameMap> init_maps);

    const std::vector<GameMap> & get_available_maps() const;

    // in the future, perhaps allow for adding maps to the repository

private:
    std::vector<GameMap> maps_;
};
