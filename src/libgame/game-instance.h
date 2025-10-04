#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>

#include "flat-array.h"
#include "player.h"
#include "maps.h"
#include "player-view.h"

// The game instance class contains all
// relevant state data for a given game.
class GameInstance
{
public:
    GameInstance();

    // Create a predefined environment from map
    GameInstance(GameMap map);

    GameInstance(const MapSettings & map);

    ~GameInstance() = default;

    // Print the current instance state to the console
    void print_instance_console() const;

    void rotate_tank(uint8_t ID, uint8_t dir);

    void rotate_tank_barrel(uint8_t ID, uint8_t dir);

    bool move_tank(uint8_t ID, bool reverse);

    bool fire_tank(uint8_t ID);

    PlayerView compute_view(uint8_t player_ID, uint8_t & num_live_tanks);

    PlayerView dump_global_view();

    void cast_ray(PlayerView & player_view,
                  vec2 start,
                  vec2 slope,
                  float size,
                  float max_range,
                  uint8_t dir) const;

    void place_tank(vec2 pos, uint8_t player_ID, uint8_t placement_direction);

    void load_tank(uint8_t ID);

    const std::vector<uint8_t> get_mask();

    inline Player & get_player(uint8_t index);

    inline const Player & get_player(uint8_t index) const;

    inline Tank & get_tank(uint8_t index);

    inline void set_occupant(vec2 pos, uint8_t occupant);

    inline uint8_t get_width() const;

    inline uint8_t get_height() const;

    inline bool check_placement(vec2 pos, uint8_t player) const;

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
    FlatArray<GridCell> game_env_;
    std::vector<Player> players_;
    std::vector<uint8_t> placement_mask_;

public:
    std::vector<Tank> tanks_;
};

inline size_t GameInstance::idx(size_t x, size_t y) const
{
    return game_env_.idx(x,y);
}

inline size_t GameInstance::idx(const vec2 & pos) const
{
    return game_env_.idx(pos.x_, pos.y_);
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

// We want to ensure the player move is on a tile
// that is valid for placement and within
// their placement mask.
inline bool GameInstance::check_placement(vec2 pos, uint8_t player) const
{
    size_t i = idx(pos);
    // Check if the placement mask matches the player.
    if (placement_mask_[i] == player)
    {
        // Check that the tile is not terrain or occupied.
        if (game_env_[i].type_ != CellType::Terrain &&
            game_env_[i].occupant_ == NO_OCCUPANT)
        {
            return true;
        }
    }
    return false;
}

