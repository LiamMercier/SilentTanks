#pragma once

#include <iostream>
#include <fstream>
#include <string>

#include "environment.h"
#include "player.h"

// The game instance class contains all
// relevant state data for a given game.
class GameInstance
{
public:
    GameInstance() = delete;

    // Create empty environment
    GameInstance(uint8_t input_width, uint8_t input_height, Tank* starting_locations);

    // Create a predefined environment from filename
    GameInstance(uint8_t input_width, uint8_t input_height, const std::string& filename, Tank* starting_locations, uint8_t num_tanks, Player* players, uint8_t num_players);

    ~GameInstance();

    // Print the current instance state to the console
    void print_instance_console() const;

    void rotate_tank(uint8_t ID, uint8_t dir);

    void rotate_tank_barrel(uint8_t ID, uint8_t dir);

    bool move_tank(uint8_t ID);

    bool fire_tank(uint8_t ID);

private:

    // Pass through the 2D -> 1D mapping for private use.
    inline size_t idx(size_t x, size_t y) const;

    inline size_t idx(const vec2 & pos) const;

    // Read env file
    //
    // Called when we create an instance from a file name
    void read_env_by_name(const std::string& filename, uint16_t total);

public:
    uint8_t num_players_;
    uint8_t num_tanks_;

private:
    Environment game_env_;
    Player* players_;
    Tank* tanks_;
};

inline size_t GameInstance::idx(size_t x, size_t y) const
{
    return game_env_.idx(x,y);
}

inline size_t GameInstance::idx(const vec2 & pos) const
{
    return game_env_.idx(pos);
}

