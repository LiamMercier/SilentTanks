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

    MapSettings()
    :filename(""),
    width(0),
    height(0),
    num_tanks(0),
    num_players(0),
    mode(0)
    {
    }

public:
    std::string filename;
    uint8_t width;
    uint8_t height;
    // tanks per person
    uint8_t num_tanks;
    // number of players
    uint8_t num_players;

    // Type of map, linked to GameMode.
    uint8_t mode;

};

struct GameMap
{
public:
    GameMap(MapSettings settings);

    GameMap()
    :map_settings()
    {}

public:
    MapSettings map_settings;
    FlatArray<GridCell> env;
    std::vector<uint8_t> mask;

};
