#include "tank-entity.h"

Tank::Tank(uint8_t x_start, uint8_t y_start, Direction start_dir)
:current_position_(x_start, y_start), current_direction_(start_dir), barrel_direction_(start_dir)
{}
