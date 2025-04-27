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

void Environment::print_view(Tank* tanks_list) const
{
    const static std::string colors_array[4] = {"\033[48;5;196m", "\033[48;5;21m", "\033[48;5;46m", "\033[48;5;226m"};

    for (int y = 0; y < get_height(); y++)
    {
        for (int x = 0; x < get_width(); x++)
        {
            GridCell curr = environment_layout_[idx(x,y)];

            if (curr.occupant_ == UINT8_MAX)
            {
                if (curr.visible_ == true)
                {
                    std::cout << "\033[48;5;184m" << "_" << "\033[0m ";
                }
                else
                {
                    if (curr.type_ == CellType::Terrain)
                    {
                        std::cout << "\033[48;5;130m" << curr << "\033[0m ";
                    }
                    else
                    {
                        std::cout << "?" << " ";
                    }
                }
            }
            else
            {
                Tank this_tank = tanks_list[curr.occupant_];
                uint8_t tank_owner = this_tank.get_owner();
                std::cout << colors_array[tank_owner] << +(curr.occupant_) << "\033[0m ";
            }
        }
        std::cout << "\n";
    }

}
