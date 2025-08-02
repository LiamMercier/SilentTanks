#include "maps.h"

MapSettings::MapSettings(std::string file,
                         uint8_t w,
                         uint8_t h,
                         uint8_t n_tanks,
                         uint8_t n_players,
                         uint8_t mode)
:filename(std::move(file)),
width(w),
height(h),
num_tanks(n_tanks),
num_players(n_players),
mode(mode)
{

}

GameMap::GameMap(MapSettings settings)
:map_settings(settings),
env(settings.width, settings.height),
mask(settings.width * settings.height)
{

}
