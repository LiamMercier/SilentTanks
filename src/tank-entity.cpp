#include "tank-entity.h"

Tank::Tank()
{

}

Tank::Tank(uint8_t x_start, uint8_t y_start, uint8_t start_dir, uint8_t owner)
:pos_(x_start, y_start), current_direction_(start_dir), barrel_direction_(start_dir), owner_(owner)
{}
