#include "tank-entity.h"
#include <iostream>
#include <string>

Tank::Tank()
:pos_(NO_POS_VEC),
current_direction_(0),
barrel_direction_(0),
health_(0),
aim_focused_(false),
loaded_(false),
owner_(NO_OWNER)
{

}

Tank::Tank(uint8_t x_start, uint8_t y_start, uint8_t start_dir, uint8_t owner)
:pos_(x_start, y_start), current_direction_(start_dir), barrel_direction_(start_dir), health_(INITIAL_HEALTH), aim_focused_(false), loaded_(true), owner_(owner)
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

void Tank::repair(uint8_t repair_amount)
{
    if (health_ + repair_amount > health_){
        health_ = INITIAL_HEALTH;
    }
    else
    {
        health_ = health_ + repair_amount;
    }
}

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
