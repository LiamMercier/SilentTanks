#include "tank-entity.h"
#include <iostream>
#include <string>

vec2 dir_to_vec(uint8_t dir)
{
    switch (dir)
    {
        case 0:
            return vec2(0, -1);
        case 1:
            return vec2(1, -1);
        case 2:
            return vec2(1, 0);
        case 3:
            return vec2(1, 1);
        case 4:
            return vec2(0, 1);
        case 5:
            return vec2(-1, 1);
        case 6:
            return vec2(-1, 0);
        case 7:
            return vec2(-1, -1);
    }
    // should never happen
    return vec2(0, 0);

}

const uint8_t INITIAL_HEALTH = 3;

Tank::Tank()
{

}

Tank::Tank(uint8_t x_start, uint8_t y_start, uint8_t start_dir, uint8_t owner)
:pos_(x_start, y_start), current_direction_(start_dir), barrel_direction_(start_dir), health_(INITIAL_HEALTH), aim_focused_(false), owner_(owner)
{}

void Tank::deal_damage(uint8_t damage)
{
    if (damage > health_){
        health_ = 0;
    }
    else
    {
        health_ = health_ - damage;
    }
}

// should be removed eventually, but useful for now while
// we work in the console
std::string direction_words[8]{"north", "north-east", "east", "south-east", "south", "south-west", "west", "north-west"};

void Tank::print_tank_state(uint8_t ID) const
{
    std::cout << "================\n";
    std::cout << "TANK ID: " << +ID << "\n";
    std::cout << "TANK OWNER: Player " << +owner_ << "\n";
    std::cout << "TANK DIRECTION: " << direction_words[current_direction_] << "\n";
    std::cout << "BARREL DIRECTION: " << direction_words[barrel_direction_] << "\n";
    std::cout << "HEALTH: " << +health_ << "\n";
    std::cout << "================\n";
}
