#pragma once

#include <new>
#include <iostream>

#include "tank-entity.h"

class Player
{
public:
    Player();

    Player(uint8_t num_tanks, uint8_t ID);

    Player(const Player& other);

    ~Player();

    int* get_tanks_list();

private:
    // Array of pointers to tanks IDs
    int* owned_tank_IDs_;
    uint8_t num_tanks_;
    uint8_t player_ID_;

};
