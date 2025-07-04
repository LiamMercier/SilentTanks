#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>

#include "environment.h"
#include "player.h"
#include "maps.h"
#include "player-view.h"

// The game instance class contains all
// relevant state data for a given game.
class GameInstance
{
public:
    GameInstance() = delete;

    // Create a predefined environment from map
    GameInstance(const GameMap & map, uint8_t num_players);

    ~GameInstance();

    // Print the current instance state to the console
    void print_instance_console() const;

    void rotate_tank(uint8_t ID, uint8_t dir);

    void rotate_tank_barrel(uint8_t ID, uint8_t dir);

    bool move_tank(uint8_t ID, bool reverse);

    bool fire_tank(uint8_t ID);

    PlayerView compute_view(uint8_t player_ID, uint8_t & live_tanks);

    void cast_ray(PlayerView & player_view, vec2 start, vec2 slope, float size, float max_range ,uint8_t dir) const;

    void place_tank(vec2 pos, uint8_t player_ID);

    void load_tank(uint8_t ID);

    inline Player& get_player(uint8_t index);

    inline const Player& get_player(uint8_t index) const;

    inline Tank& get_tank(uint8_t index);

    inline void set_occupant(vec2 pos, uint8_t occupant);

    inline uint8_t get_width() const;

    inline uint8_t get_height() const;

    inline bool check_placement_mask(vec2 pos, uint8_t player) const;

    // Read env file
    //
    // Called when we create an instance from a file name
    bool read_env_by_name(const std::string& filename, uint16_t total);

private:

    // Pass through the 2D -> 1D mapping for private use.
    inline size_t idx(size_t x, size_t y) const;

    inline size_t idx(const vec2 & pos) const;

public:
    uint8_t num_players_;
    // Number of tanks per player.
    uint8_t num_tanks_;

private:
    Environment game_env_;
    std::vector<Player> players_;
    std::vector<uint8_t> placement_mask_;

public:
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

inline Player& GameInstance::get_player(uint8_t index)
{
    return players_[index];
}

inline const Player& GameInstance::get_player(uint8_t index) const
{
    return players_[index];
}

inline Tank& GameInstance::get_tank(uint8_t index)
{
    return tanks_[index];
}

inline void GameInstance::set_occupant(vec2 pos, uint8_t occupant)
{
    game_env_[idx(pos)].occupant_ = occupant;
    return;
}

inline uint8_t GameInstance::get_width() const
{
    return game_env_.get_width();
}

inline uint8_t GameInstance::get_height() const
{
    return game_env_.get_height();
}

inline bool GameInstance::check_placement_mask(vec2 pos, uint8_t player) const
{
    return (placement_mask_[idx(pos)] == player);
}

