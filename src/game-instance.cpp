#include "game-instance.h"

GameInstance::GameInstance(uint8_t input_width, uint8_t input_length)
:game_env_(input_width, input_length)
{
}

GameInstance::GameInstance(uint8_t input_width, uint8_t input_length, const std::string& filename)
:game_env_(input_width, input_length, input_width*input_length)
{
    read_env_by_name(filename, input_width*input_length);
}

GameInstance::~GameInstance()
{
}

void GameInstance::print_instance_console()
{
    for (int x = 0; x < game_env_.get_width(); x++)
    {
        for (int y = 0; y < game_env_.get_length(); y++)
        {
            std::cout << game_env_[idx(x,y)] << " ";
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
        (game_env_[i]).type_ = static_cast<CellType>(static_cast<uint8_t>(buffer[i] - '0'));
    }

}
