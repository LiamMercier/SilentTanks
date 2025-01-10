#include "game-instance.h"

// Not useful for now
GameInstance::GameInstance(uint8_t input_width, uint8_t input_height, Tank* starting_locations)
:game_env_(input_width, input_height), tanks(starting_locations)
{
    return;
}

GameInstance::GameInstance(uint8_t input_width, uint8_t input_height, const std::string& filename, Tank* starting_locations, uint8_t num_tanks)
:game_env_(input_width, input_height, input_width*input_height), tanks(starting_locations)
{
    read_env_by_name(filename, input_width*input_height);

    // set tank positions
    for (int i = 0; i < num_tanks; i++)
    {
        Tank curr_tank = tanks[i];
        vec2 tank_pos = curr_tank.pos_;
        // for the index of the environment where the
        // tank is, set the occupant of the tile to the tank number.
        (game_env_[idx(tank_pos.x_, tank_pos.y_)]).occupant_ = i;
    }

}

GameInstance::~GameInstance()
{
    delete[] tanks;
}

// Temporary array to hold colors for console print testing
const static std::string colors_array[4] = {"\033[48;5;196m", "\033[48;5;21m", "\033[48;5;46m", "\033[48;5;226m"};

// Print information to the console
void GameInstance::print_instance_console()
{
    for (int y = 0; y < game_env_.get_height(); y++)
    {
        for (int x = 0; x < game_env_.get_width(); x++)
        {
            GridCell curr = game_env_[idx(x,y)];

            if (curr.occupant_ == UINT8_MAX)
            {
                std::cout << curr << " ";
            }
            else
            {
                Tank this_tank = tanks[curr.occupant_];
                uint8_t tank_owner = this_tank.get_owner();
                std::cout << colors_array[tank_owner] << (char)('A' + curr.occupant_) << "\033[0m ";
            }
        }
        std::cout << "\n";
    }

}

bool GameInstance::move_tank(uint8_t ID)
{
    Tank curr_tank = tanks[ID];
    uint8_t dir = curr_tank.current_direction_;
    vec2 prev = curr_tank.pos_;

    // Since we are using uint8_t we know that
    // all out of bounds checks will have the respective
    // variable be larger than the max width/length
    //
    // 0 - 1 = 255 > bound_max
    // bound_max + 1 > bound_max
    //
    // Only exception is if we allow the max bound to be reached by
    // the environment and then increase, since we would get
    //
    // 255 + 1 = 0
    //
    // So, don't allow length/width to be 255. Shouldn't be that
    // large anyways.

    switch (dir)
    {
        case 0:
            if (prev.y_ - 1 > game_env_.get_height())
            {
                return false;
            }
            curr_tank.pos_.y_ -= 1;
            break;
        case 1:
            if ((prev.y_ - 1 > game_env_.get_height()) || (prev.x_ + 1 > game_env_.get_width()))
            {
                return false;
            }
            curr_tank.pos_.y_ -= 1;
            curr_tank.pos_.x_ += 1;
            break;
        case 2:
            if (prev.x_ + 1 > game_env_.get_height())
            {
                return false;
            }
            curr_tank.pos_.x_ += 1;
            break;
        case 3:
            if ((prev.y_ + 1 > game_env_.get_height()) || (prev.x_ + 1 > game_env_.get_width()))
            {
                return false;
            }
            curr_tank.pos_.y_ += 1;
            curr_tank.pos_.x_ += 1;
            break;
        case 4:
            if (prev.y_ + 1 > game_env_.get_height())
            {
                return false;
            }
            curr_tank.pos_.y_ += 1;
            break;
        case 5:
            if ((prev.y_ + 1 > game_env_.get_height()) || (prev.x_ - 1 > game_env_.get_width()))
            {
                return false;
            }
            curr_tank.pos_.y_ += 1;
            curr_tank.pos_.x_ -= 1;
            break;
        case 6:
            if (prev.x_ - 1 > game_env_.get_height())
            {
                return false;
            }
            curr_tank.pos_.x_ -= 1;
            break;
        case 7:
            if ((prev.y_ - 1 > game_env_.get_height()) || (prev.x_ - 1 > game_env_.get_width()))
            {
                return false;
            }
            curr_tank.pos_.y_ -= 1;
            curr_tank.pos_.x_ -= 1;
            break;
    }

    // After this point, we know the move must be valid.
    //
    // Proceed with updates.

    GridCell prev_cell = game_env_[idx(prev.x_,prev.y_)];
    prev_cell.occupant_ = UINT8_MAX;

    GridCell curr_cell = game_env_[idx(curr_tank.pos_.x_,curr_tank.pos_.y_)];
    curr_cell.occupant_ = ID;

    return true;
}

void GameInstance::read_env_by_name(const std::string& filename, uint16_t total)
{
    /*
     Somewhere down the line we will probably need to do
     #if defined(_WIN32) || defined(_WIN64)
        filename = "C:\\path\\to\\file.txt";
    #else
        filename = "/home/user/file.txt";
    #endif

    but this could possibly be pushed off to the precomputed file list or something
    that will be made later
     */

    std::ifstream file(filename, std::ios::in);

    if (!file.is_open()){
        std::cerr << "ERROR IN FILE OPENING\n";
        return;
    }

    // read the file all at once into the buffer
    char buffer[total];
    file.read(buffer, total);

    // turn the ASCII data (48, 49, 50..) into the correct integers (0,1,2)
    // and assign the GridCell elements the correct type accordingly.
    for (int i = 0; i < total; i++)
    {
        // CellType is an enum derived from uint8_t, hence the two casts.
        //
        // Normally you shouldn't do this, but we're reading a chunk of data.
        game_env_[i].type_ = static_cast<CellType>(static_cast<uint8_t>(buffer[i] - '0'));
        game_env_[i].occupant_ = UINT8_MAX;
    }

}
