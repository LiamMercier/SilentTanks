#include "environment.h"

Environment::Environment(uint8_t input_width, uint8_t input_length)
:environment_layout_(input_width, input_length)
{}

GridCell& Environment::operator[](size_t index)
{
    return environment_layout_[index];
}

const GridCell& Environment::operator[](size_t index) const
{
    return environment_layout_[index];
}
