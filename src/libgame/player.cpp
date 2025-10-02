#include "player.h"

Player::Player(uint8_t num_tanks, uint8_t ID)
:tanks_placed_(0),
owned_tank_IDs_(num_tanks, NO_TANK),
player_ID_(ID)
{
}

std::vector<int> & Player::get_tanks_list()
{
    return owned_tank_IDs_;
}

const std::vector<int> & Player::get_tanks_list() const
{
    return owned_tank_IDs_;
}
