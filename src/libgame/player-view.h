#pragma once

#include <vector>
#include <chrono>

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

    inline size_t indx(size_t x, size_t y) const
    {
        return map_view.idx(x, y);
    }

    // Linear scan to find the tank for this occupant.
    inline Tank find_tank(uint8_t tank_id) const
    {
        for (const auto & tank : visible_tanks)
        {
            if (tank_id == tank.id_)
            {
                return tank;
            }
        }

        return {};
    }

    FlatArray<GridCell> map_view;
    std::vector<Tank> visible_tanks;
    std::vector<std::chrono::milliseconds> timers;
    uint8_t current_player;
    uint8_t current_fuel;
    GameState current_state;
};
