#pragma once

#include <cstdint>

struct vec2
{
public:
    vec2(uint8_t x, uint8_t y)
    :x_(x), y_(y)
    {}

    uint8_t x_;
    uint8_t y_;
};

class Tank
{
public:

    Tank(uint8_t x_start, uint8_t y_start, uint8_t start_dir, uint8_t owner);

    inline uint8_t get_owner () const;

    inline void turn_clockwise();

    inline void barrel_clockwise();

    inline void turn_counter_clockwise();

    inline void barrel_counter_clockwise();

    vec2 pos_;

    // Directions start at 0 (North) and end at 7 (North west)
    //
    // Compass with numbers:
    //
    //    7 0 1
    //    6 X 2
    //    5 4 3
    //
    uint8_t current_direction_;
    uint8_t barrel_direction_;

private:

    // player who owns this tank
    uint8_t owner_;
};

inline uint8_t Tank::get_owner () const
{
    return owner_;
}

inline void Tank::turn_clockwise()
{
    current_direction_ = (current_direction_ + 1) % 8;
}

inline void Tank::barrel_clockwise()
{
    barrel_direction_ = (barrel_direction_ + 1) % 8;
}

inline void Tank::turn_counter_clockwise()
{
    current_direction_ = (current_direction_ - 1) % 8;
}

inline void Tank::barrel_counter_clockwise()
{
    barrel_direction_ = (barrel_direction_ - 1) % 8;
}
