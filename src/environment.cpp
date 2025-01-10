#include "environment.h"

Environment::Environment(uint8_t input_width, uint8_t input_height)
:environment_layout_(input_width, input_height)
{
}

Environment::Environment(uint8_t input_width, uint8_t input_height, uint16_t total_entries)
:environment_layout_(input_width, input_height, total_entries)
{
}


Environment::~Environment()
{
}

GridCell& Environment::operator[](size_t index)
{
    return environment_layout_[index];
}

const GridCell& Environment::operator[](size_t index) const
{
    return environment_layout_[index];
}
