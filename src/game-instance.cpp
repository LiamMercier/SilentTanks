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
        vec2 tank_pos = curr_tank.get_pos();
        // for the index of the environment where the
        // tank is, set the occupant of the tile to the tank number.
        (game_env_[idx(tank_pos.x_, tank_pos.y_)]).occupant_ = i;
    }

}

GameInstance::~GameInstance()
{
    delete[] tanks;
}

void GameInstance::print_instance_console()
{
    std::cout << "WIDTH: " << +game_env_.get_width() << " LEN: " << +game_env_.get_height() << "\n";
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
                std::cout << 'A' << " ";
            }
        }
        std::cout << "\n";
    }

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
