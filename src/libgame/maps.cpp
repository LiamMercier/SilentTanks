#include "maps.h"

GameMap::GameMap(std::string file, uint8_t w, uint8_t h, uint8_t n_tanks)
:filename(std::move(file)), width(w), height(h), num_tanks(n_tanks)
{

}

MatchSettings::MatchSettings(GameMap arg_map, uint64_t init_time_ms, uint64_t inc)
:map(arg_map), initial_time_ms(init_time_ms), increment_ms(inc)
{
}

MapRepository::MapRepository(std::vector<GameMap> init_maps)
: maps_(std::move(init_maps))
{
}

const std::vector<GameMap> & MapRepository::get_available_maps() const
{
    return maps_;
}
