#include "maps.h"

GameMap::GameMap(std::string file,
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
