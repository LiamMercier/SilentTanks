#pragma once

#include "cell-type.h"
#include "constants.h"
#include <iostream>

struct GridCell
{
    CellType type_;

    // Which tank is on this Grid.
    //
    // NO_OCCUPANT if not occupied.
    uint8_t occupant_{NO_OCCUPANT};

    // used when computing views
    bool visible_;
};

std::ostream& operator<<(std::ostream& os, const GridCell& cell);
