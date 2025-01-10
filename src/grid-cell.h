#pragma once

#include "cell-type.h"
#include <iostream>

struct GridCell
{
    CellType type_;
};

std::ostream& operator<<(std::ostream& os, const GridCell& cell);
