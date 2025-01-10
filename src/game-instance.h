#pragma once

#include "environment.h"
#include <iostream>
#include <fstream>
#include <string>

// The game instance class contains all
// relevant state data for a given game.
class GameInstance
{
public:
    GameInstance() = delete;

    // Create empty environment
    GameInstance(uint8_t input_width, uint8_t input_length);

    // Create a predefined environment from filename
    GameInstance(uint8_t input_width, uint8_t input_length, const std::string& filename);

    ~GameInstance();

    // Print the current instance state to the console
    void print_instance_console();

private:

    // Pass through the 2D -> 1D mapping for private use.
    inline size_t idx(size_t x, size_t y) const;

    // Read env file
    //
    // Called when we create an instance from a file name
    void read_env_by_name(const std::string& filename, uint16_t total);

private:
    Environment game_env_;
};

inline size_t GameInstance::idx(size_t x, size_t y) const
{
    return game_env_.idx(x,y);
}
