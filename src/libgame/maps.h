#pragma once

#include <string>
#include <vector>

#include "grid-cell.h"
#include "flat-array.h"

struct MapSettings
{
public:
    MapSettings(std::string file,
                uint8_t w,
                uint8_t h,
                uint8_t n_tanks,
                uint8_t n_players,
                uint8_t mode);

public:
    std::string filename;
    uint8_t width;
    uint8_t height;
    // tanks per person
    uint8_t num_tanks;
    // number of players
    uint8_t num_players;

    // Type of map. 0 is for 2 player for now.
    uint8_t mode;

};

struct GameMap
{
public:
    GameMap(MapSettings settings);

public:
    MapSettings map_settings;
    FlatArray<GridCell> env;
    std::vector<uint8_t> mask;

};
