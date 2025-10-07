#pragma once

struct vec2
{
public:
    vec2()
    :x_(0), y_(0)
    {}

    constexpr vec2(uint8_t x, uint8_t y)
    :x_(x), y_(y)
    {}

    inline vec2& operator+=(vec2 const & rhs);

    inline friend vec2 operator+(vec2 lhs, vec2 const & rhs);

    uint8_t x_;
    uint8_t y_;
};

inline vec2& vec2::operator+=(vec2 const & rhs)
{
        x_ += rhs.x_;
        y_ += rhs.y_;
        return *this;
}

inline vec2 operator+(vec2 lhs, vec2 const & rhs)
{
    lhs += rhs;
    return lhs;
}
