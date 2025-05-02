#pragma once

#include <new>
#include <iostream>

#include "tank-entity.h"

class Player
{
public:
    Player() = delete;

    Player(uint8_t num_tanks, uint8_t ID);

    Player(const Player& other);

    ~Player();

    int* get_tanks_list();

    const int* get_tanks_list() const;

public:
    uint8_t tanks_placed_;
private:
    // Array of pointers to tanks IDs
    int* owned_tank_IDs_;

    uint8_t num_tanks_;
    uint8_t player_ID_;

};
