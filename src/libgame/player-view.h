#pragma once

#include <vector>
#include "grid-cell.h"
#include "tank-entity.h"
#include "flat-array.h"
#include "game-state.h"

// Structure to hold a full view of the game state for a given player.
struct PlayerView
{
    PlayerView()
    :map_view()
    {
    }

    PlayerView(uint8_t width, uint8_t height)
    :map_view(width, height)
    {

    }

    inline uint8_t width() const
    {
        return map_view.get_width();
    }

    inline uint8_t height() const
    {
        return map_view.get_height();
    }

    FlatArray<GridCell> map_view;
    std::vector<Tank> visible_tanks;
    uint8_t current_player;
    uint8_t current_fuel;
    GameState current_state;
};
