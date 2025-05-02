#include "grid-cell.h"

std::ostream& operator<<(std::ostream& os, const GridCell& cell) {
    os <<  +((uint8_t) cell.type_);
    return os;
}
