#pragma once

#include <new>
#include <iostream>
#include <vector>

#include "tank-entity.h"

class Player
{
public:
    Player() = delete;

    Player(uint8_t num_tanks, uint8_t ID);

    ~Player() = default;

    std::vector<int> & get_tanks_list();

    const std::vector<int> & get_tanks_list() const;

public:
    uint8_t tanks_placed_;
private:
    std::vector<int> owned_tank_IDs_;
    uint8_t player_ID_;

};
