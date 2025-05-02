#include "maps.h"

GameMap::GameMap(std::string file, uint8_t w, uint8_t h, uint8_t n_tanks)
:filename(std::move(file)), width(w), height(h), num_tanks(n_tanks)
{

}
