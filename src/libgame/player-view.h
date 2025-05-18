#pragma once

#include <vector>

// Structure to hold a full view of the game state for a given player.
struct PlayerView
{
    PlayerView()
    :map_view(Environment::make_empty())
    {
    }

    PlayerView(uint8_t width, uint8_t height)
    :map_view(width, height)
    {

    }

    Environment map_view;
    std::vector<Tank> visible_tanks;
    uint8_t current_player;
};
