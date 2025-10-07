#pragma once

#include "vec2.h"

struct MoveStatus
{
    MoveStatus()
    :success{false},
    hits()
    {
    }

    MoveStatus(bool result)
    :success{result},
    hits()
    {
    }

    bool success;
    std::vector<vec2> hits;
};
