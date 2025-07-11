#pragma once

#include <vector>
#include "grid-cell.h"
#include "tank-entity.h"
#include "flat-array.h"

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

    FlatArray<GridCell> map_view;
    std::vector<Tank> visible_tanks;
    uint8_t player_id;
};
