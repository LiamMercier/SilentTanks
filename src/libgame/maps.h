#pragma once

#include <string>

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
