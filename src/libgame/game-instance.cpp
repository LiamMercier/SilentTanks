#include "game-instance.h"
#include "constants.h"

#include <filesystem>

GameInstance::GameInstance()
:num_players_(0),
num_tanks_(0),
game_env_(),
placement_mask_(0),
tanks_(0)
{
}

// Constructor for match instances.
GameInstance::GameInstance(GameMap map)
:num_players_(map.map_settings.num_players),
num_tanks_(map.map_settings.num_tanks),
game_env_(std::move(map.env)),
placement_mask_(std::move(map.mask)),
tanks_(num_tanks_ * num_players_)
{
    players_.reserve(num_players_);

    // Create each tank in place, not currently alive.
    for (uint8_t i = 0; i < num_players_; ++i)
    {
        players_.emplace_back(num_tanks_, i);
    }
}

// Constructor when no map repository exists.
GameInstance::GameInstance(const MapSettings & map_settings)
:num_players_(map_settings.num_players),
num_tanks_(map_settings.num_tanks),
game_env_(map_settings.width, map_settings.height),
placement_mask_(map_settings.width * map_settings.height),
tanks_(num_tanks_ * num_players_)
{
    players_.reserve(num_players_);

    // Create each tank in place, not currently alive.
    for (uint8_t i = 0; i < num_players_; ++i)
    {
        players_.emplace_back(num_tanks_, i);
    }
}

// Rotate a tank of given ID
void GameInstance::rotate_tank(uint8_t ID, uint8_t dir)
{
    Tank & curr_tank = tanks_[ID];
    if (dir == 0)
    {
        curr_tank.turn_clockwise();
    }
    else
    {
        curr_tank.turn_counter_clockwise();
    }
}

// Rotate the barrel of a tank of given ID
void GameInstance::rotate_tank_barrel(uint8_t ID, uint8_t dir)
{
    Tank & curr_tank = tanks_[ID];
    if (dir == 0)
    {
        curr_tank.barrel_clockwise();
    }
    else
    {
        curr_tank.barrel_counter_clockwise();
    }
}

// Given the ID of a tank, attempt to move it in its current direction.
//
// Return True if the move was successful, or false if there was terrain or other objects.
bool GameInstance::move_tank(uint8_t ID, bool reverse)
{
    Tank & curr_tank = tanks_[ID];
    uint8_t dir = curr_tank.current_direction_;

    if (reverse == true)
    {
        dir = (dir + 4) % 8;
    }

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
    //
    // Otherwise, we make sure the tank doesn't move into terrain or another player

    switch (dir)
    {
        case 0:
        {
            if (uint8_t(prev.y_ - 1) > game_env_.get_height() - 1)
            {
                return false;
            }
            curr_tank.pos_.y_ -= 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != NO_OCCUPANT)
            {
                curr_tank.pos_.y_ = prev.y_;
                return false;
            }
            if (moved_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.y_ = prev.y_;
                return false;
            }
        }
            break;
        case 1:
        {
            if ((uint8_t(prev.y_ - 1) > game_env_.get_height() - 1) || (uint8_t(prev.x_ + 1) > game_env_.get_width() - 1))
            {
                return false;
            }
            curr_tank.pos_.y_ -= 1;
            curr_tank.pos_.x_ += 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != NO_OCCUPANT)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
            if (moved_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }

            // When we move diagonally we need to check
            // that we aren't cutting through two mountains
            //
            // this is when we only do part of our movement, so
            // create two possible mountain positions and check.
            vec2 m1 = vec2(curr_tank.pos_.x_, prev.y_);
            vec2 m2 = vec2(prev.x_, curr_tank.pos_.y_);

            GridCell m1_cell = game_env_[idx(m1)];
            GridCell m2_cell = game_env_[idx(m2)];

            // Prevent passage diagonally when blocked in
            if (m1_cell.type_ == CellType::Terrain && m2_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
        }
            break;
        case 2:
        {
            if (uint8_t(prev.x_ + 1) > game_env_.get_width() - 1)
            {
                return false;
            }
            curr_tank.pos_.x_ += 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != NO_OCCUPANT)
            {
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
            if (moved_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
        }
            break;
        case 3:
        {
            if ((uint8_t(prev.y_ + 1) > game_env_.get_height() - 1) || (uint8_t(prev.x_ + 1) > game_env_.get_width() - 1))
            {
                return false;
            }
            curr_tank.pos_.y_ += 1;
            curr_tank.pos_.x_ += 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != NO_OCCUPANT)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
            if (moved_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }

            // When we move diagonally we need to check
            // that we aren't cutting through two mountains
            //
            // this is when we only do part of our movement, so
            // create two possible mountain positions and check.
            vec2 m1 = vec2(curr_tank.pos_.x_, prev.y_);
            vec2 m2 = vec2(prev.x_, curr_tank.pos_.y_);

            GridCell m1_cell = game_env_[idx(m1)];
            GridCell m2_cell = game_env_[idx(m2)];

            // Prevent passage diagonally when blocked in
            if (m1_cell.type_ == CellType::Terrain && m2_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
        }
            break;
        case 4:
        {
            if (uint8_t(prev.y_ + 1) > game_env_.get_height() - 1)
            {
                return false;
            }
            curr_tank.pos_.y_ += 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != NO_OCCUPANT)
            {
                curr_tank.pos_.y_ = prev.y_;
                return false;
            }
            if (moved_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.y_ = prev.y_;
                return false;
            }
        }
            break;
        case 5:
        {
            if ((uint8_t(prev.y_ + 1) > game_env_.get_height() - 1) || (uint8_t(prev.x_ - 1) > game_env_.get_width() - 1))
            {
                return false;
            }
            curr_tank.pos_.y_ += 1;
            curr_tank.pos_.x_ -= 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != NO_OCCUPANT)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
            if (moved_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }

            // When we move diagonally we need to check
            // that we aren't cutting through two mountains
            //
            // this is when we only do part of our movement, so
            // create two possible mountain positions and check.
            vec2 m1 = vec2(curr_tank.pos_.x_, prev.y_);
            vec2 m2 = vec2(prev.x_, curr_tank.pos_.y_);

            GridCell m1_cell = game_env_[idx(m1)];
            GridCell m2_cell = game_env_[idx(m2)];

            // Prevent passage diagonally when blocked in
            if (m1_cell.type_ == CellType::Terrain && m2_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
        }
            break;
        case 6:
        {
            if (uint8_t(prev.x_ - 1) > game_env_.get_width() - 1)
            {
                return false;
            }
            curr_tank.pos_.x_ -= 1;
            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != NO_OCCUPANT)
            {
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
            if (moved_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
        }
            break;
        case 7:
        {
            if ((uint8_t(prev.y_ - 1) > game_env_.get_height() - 1) || (uint8_t(prev.x_ - 1) > game_env_.get_width() - 1))
            {
                return false;
            }
            curr_tank.pos_.y_ -= 1;
            curr_tank.pos_.x_ -= 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != NO_OCCUPANT)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
            if (moved_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }

            // When we move diagonally we need to check
            // that we aren't cutting through two mountains
            //
            // this is when we only do part of our movement, so
            // create two possible mountain positions and check.
            vec2 m1 = vec2(curr_tank.pos_.x_, prev.y_);
            vec2 m2 = vec2(prev.x_, curr_tank.pos_.y_);

            GridCell m1_cell = game_env_[idx(m1)];
            GridCell m2_cell = game_env_[idx(m2)];

            // Prevent passage diagonally when blocked in
            if (m1_cell.type_ == CellType::Terrain && m2_cell.type_ == CellType::Terrain)
            {
                curr_tank.pos_.y_ = prev.y_;
                curr_tank.pos_.x_ = prev.x_;
                return false;
            }
        }
            break;
    }

    // After this point, we know the move must be valid.
    //
    // Proceed with updates.

    // Make old cell unoccupied
    game_env_[idx(prev.x_, prev.y_)].occupant_ = NO_OCCUPANT;

    // Make new cell occupied with our tank ID
    game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)].occupant_ = ID;

    return true;
}

bool GameInstance::fire_tank(uint8_t ID)
{
    Tank & curr_tank = tanks_[ID];

    vec2 shell_dir = dir_to_vec[curr_tank.barrel_direction_];
    vec2 curr_loc = curr_tank.pos_;

    // Expend the shell
    curr_tank.loaded_ = false;

    uint8_t shell_distance = (curr_tank.barrel_direction_ % 2) == 0
                             ? FIRING_DIST_HORIZONTAL : FIRING_DIST_DIAGONAL;

    for (int i = 1; i <= shell_distance; i++)
    {
        // Take a step
        curr_loc = curr_loc + shell_dir;

        // Test if this tile is out of bounds
        if ((curr_loc.x_ > game_env_.get_width() - 1)
            || (curr_loc.y_ > game_env_.get_height() - 1))
        {
                return false;
        }

        std::cout << game_env_.get_height() - 1
                  << " " << game_env_.get_width() - 1 << "\n";

        std::cout << idx(curr_loc) << " " << +curr_loc.x_ << " " << +curr_loc.y_ << "\n";
        GridCell curr_cell = game_env_[idx(curr_loc)];

        // test if the tile is terrain
        if (curr_cell.type_ == CellType::Terrain)
        {
            return false;
        }

        // test if the tile has a tank
        if (curr_cell.occupant_ != NO_OCCUPANT)
        {
            Tank & hit_tank = tanks_[curr_cell.occupant_];

            if (hit_tank.owner_ == curr_tank.owner_)
            {
                return false;
            }

            hit_tank.deal_damage(SHELL_DAMAGE);

            // if the health of the tank is zero, remove it from play
            if (hit_tank.health_ == 0)
            {
                vec2 hit_pos = hit_tank.pos_;
                game_env_[idx(hit_pos)].occupant_ = NO_OCCUPANT;
            }

            return true;
        }

    }
    return false;
}

MoveStatus GameInstance::replay_fire_tank(uint8_t ID)
{
    Tank & curr_tank = tanks_[ID];

    vec2 shell_dir = dir_to_vec[curr_tank.barrel_direction_];
    vec2 curr_loc = curr_tank.pos_;

    // Expend the shell
    curr_tank.loaded_ = false;

    uint8_t shell_distance = (curr_tank.barrel_direction_ % 2) == 0
                             ? FIRING_DIST_HORIZONTAL : FIRING_DIST_DIAGONAL;

    for (int i = 1; i <= shell_distance; i++)
    {
        // Take a step
        curr_loc = curr_loc + shell_dir;

        // Test if this tile is out of bounds
        if ((curr_loc.x_ > game_env_.get_height() - 1)
            || (curr_loc.y_ > game_env_.get_width() - 1))
        {
                return MoveStatus();
        }

        GridCell curr_cell = game_env_[idx(curr_loc)];

        // test if the tile is terrain
        if (curr_cell.type_ == CellType::Terrain)
        {
            return MoveStatus();
        }

        // test if the tile has a tank
        if (curr_cell.occupant_ != NO_OCCUPANT)
        {
            Tank & hit_tank = tanks_[curr_cell.occupant_];
            hit_tank.deal_damage(SHELL_DAMAGE);

            // if the health of the tank is zero, remove it from play
            if (hit_tank.health_ == 0)
            {
                vec2 hit_pos = hit_tank.pos_;
                game_env_[idx(hit_pos)].occupant_ = NO_OCCUPANT;
            }

            MoveStatus status;
            status.success = true;
            status.hits.push_back(hit_tank.pos_);

            return status;
        }

    }
    return MoveStatus();
}

void GameInstance::repair_tank(vec2 pos)
{
    GridCell target_cell = game_env_[idx(pos)];

    // Speed up lookup if tank is alive.
    if (target_cell.occupant_ != NO_OCCUPANT)
    {
        Tank & tank = tanks_[target_cell.occupant_];
        tank.repair(SHELL_DAMAGE);
    }
    // Otherwise, see if any of the dead tanks exist at this tile.
    else
    {
        for (Tank & tank : tanks_)
        {
            // If a tank exists at this position
            if (tank.pos_.x_ == pos.x_
                && tank.pos_.y_ == pos.y_)
            {
                // Heal the tank.
                tank.repair(SHELL_DAMAGE);

                // Ensure the tank is added back into play.
                vec2 this_pos = tank.pos_;
                game_env_[idx(this_pos)].occupant_ = tank.id_;
            }
        }
    }
}

// Below is probably the worst function in the entire code base.
//
// Should really be re-worked one day.

// TODO <feature>: vision on foliage tiles

// Compute the view for this player
//
// We want to iterate over every tank owned by the
// player, check that the tank is still in play, then
// compute the vision that this tank generates
//
// We start with the current game environment and set all cells
// to having no vision
PlayerView GameInstance::compute_view(uint8_t player_ID, uint8_t & num_live_tanks)
{
    uint8_t width = game_env_.get_width();
    uint8_t height = game_env_.get_height();
    uint16_t total = width * height;

    PlayerView view(width, height);

    FlatArray<GridCell> & player_view = view.map_view;

    // copy the cell types
    //
    // visibility was already set to zero during initialization
    for (int i = 0; i < total; i++)
    {
        player_view[i].type_ = game_env_[i].type_;
        player_view[i].occupant_ = NO_OCCUPANT;
    }

    // now, go through every tank and compute visible tiles
    //
    // the visibility depends on the aim_state_ of the tank

    const Player& this_player =  get_player(player_ID);
    const std::vector<int> & player_tank_IDs = this_player.get_tanks_list();

    for (size_t i = 0; i < num_tanks_; i++)
    {
        uint8_t curr_tank_ID = player_tank_IDs[i];

        if (curr_tank_ID == NO_TANK)
        {
            continue;
        }

        Tank& curr_tank = tanks_[curr_tank_ID];

        view.visible_tanks.push_back(curr_tank);

        // Check that the tank is still alive
        if (curr_tank.health_ == 0)
        {
            continue;
        }

        // otherwise increment the number of alive tanks
        num_live_tanks += 1;

        // Add the current tank to the visibility map
        vec2 tank_pos = curr_tank.pos_;
        player_view[idx(tank_pos)].occupant_ = curr_tank_ID;
        player_view[idx(tank_pos)].visible_ = true;

        // Use the aim state to determine what vision to compute
        //
        // When aim_state_ is false, we are not focusing the tank, use a spread out
        // cone. Example (with tank X):

        //   East Aim             South East Aim
        //
        // ? ? ? ? ? ? ?          ? ? ? ? ? ? ?
        // ? ? ? _ ? ? ?          ? X _ _ ? ? ?
        // ? ? _ _ _ ? ?          ? _ _ _ _ ? ?
        // ? X _ _ _ _ ?          ? _ _ _ _ ? ?
        // ? ? _ _ _ ? ?          ? ? _ _ _ ? ?
        // ? ? ? _ ? ? ?          ? ? ? ? ? ? ?
        // ? ? ? ? ? ? ?          ? ? ? ? ? ? ?

        // We can compute horizontal/vertical options by using 7 rays.
        //
        // TODO <refactoring>: Convert this mess into a compatible cast_ray version
        if (curr_tank.aim_focused_ == false)
        {
            // We can compute horizontal/vertical options by using 7 rays.
            if (curr_tank.barrel_direction_ % 2 == 0)
            {
                // for each ray
                for (int r = 0; r < 7; r++)
                {
                    float m = orthogonal_slopes[r];
                    int max_range = orthogonal_ray_dists[r];

                    unsigned int primary_var = tank_pos.x_;
                    unsigned int secondary_var = tank_pos.y_;

                    bool primary_var_y = false;
                    unsigned int p_early_bound = game_env_.get_width() - 1;
                    unsigned int s_early_bound = game_env_.get_height() - 1;
                    int sign = 1;

                    // west
                    if (curr_tank.barrel_direction_ == 6)
                    {
                        sign = -1;
                    }
                    // north
                    if (curr_tank.barrel_direction_ == 0)
                    {
                        sign = -1;
                        // set y as the primary variable, swap items
                        primary_var_y = true;

                        primary_var = tank_pos.y_;
                        secondary_var = tank_pos.x_;

                        p_early_bound = game_env_.get_height() - 1;
                        s_early_bound = game_env_.get_width() - 1;

                        if (r != 3)
                        {
                            m = -1*m;
                        }
                    }
                    if (curr_tank.barrel_direction_ == 4)
                    {
                        sign = 1;
                        // set y as the primary variable, swap items
                        primary_var_y = true;

                        primary_var = tank_pos.y_;
                        secondary_var = tank_pos.x_;

                        p_early_bound = game_env_.get_height() - 1;
                        s_early_bound = game_env_.get_width() - 1;

                        if (r != 3)
                        {
                            m = -1*m;
                        }
                    }

                    // Do raycast steps
                    for (int dx = 1; dx <= max_range; dx++)
                    {
                        // this step x value
                        unsigned int p = primary_var + (sign * dx);

                        // early skip for anything outside of x bounds
                        if (p > p_early_bound)
                        {
                            break;
                        }

                        // this step y value
                        float sec = secondary_var + m * dx;

                        unsigned int sec_int = std::floor(sec);
                        float frac_sec = sec - sec_int;

                        // we want to be permissive with our ray casting (design choice)
                        // so we will treat 0.5 as not touching a mountain.

                        // check upper square for terrain, if it exists
                        if (frac_sec > 0.5f + 0.001f)
                        {
                            sec_int = sec_int + 1;

                            // if out of bounds, break
                            if ((sec_int > s_early_bound))
                            {
                                break;
                            }
                            // if location is a mountain, break
                            GridCell curr_cell;
                            size_t this_idx = 0;

                            if (primary_var_y == true)
                            {
                                this_idx = idx(sec_int, p);
                                curr_cell = game_env_[this_idx];
                            }
                            else
                            {
                                this_idx = idx(p, sec_int);
                                curr_cell = game_env_[this_idx];
                            }

                            if (curr_cell.type_ == CellType::Terrain)
                            {
                                player_view[this_idx].visible_ = true;
                                break;
                            }
                        }
                        // check lower square for terrain
                        else if (frac_sec < 0.5f - 0.001f)
                        {
                            // if out of bounds, break
                            if (sec_int > s_early_bound)
                            {
                                break;
                            }
                            // if location is a mountain, break
                            GridCell curr_cell;
                            size_t this_idx = 0;

                            if (primary_var_y == true)
                            {
                                this_idx = idx(sec_int, p);
                                curr_cell = game_env_[this_idx];
                            }
                            else {
                                this_idx = idx(p, sec_int);
                                curr_cell = game_env_[this_idx];
                            }

                            if (curr_cell.type_ == CellType::Terrain)
                            {
                                player_view[this_idx].visible_ = true;
                                break;
                            }
                        }
                        // if the value is ~0.5 then continue, don't check either side as being a mountain.
                        else
                        {
                            continue;
                        }

                        // Otherwise, add this to the view if valid and continue on
                        if (std::fabs(frac_sec) < 0.001f)
                        {
                            if (primary_var_y == true)
                            {
                                player_view[idx(sec_int, p)].visible_ = true;

                                uint8_t occ = game_env_[idx(sec_int, p)].occupant_;
                                player_view[idx(sec_int, p)].occupant_ = occ;

                                // Add occupant to list of tanks.
                                if (occ != NO_OCCUPANT)
                                {
                                    Tank & this_tank = tanks_[occ];
                                    view.visible_tanks.push_back(this_tank);
                                }

                            }
                            else {
                                player_view[idx(p, sec_int)].visible_ = true;

                                uint8_t occ = game_env_[idx(p, sec_int)].occupant_;
                                player_view[idx(p, sec_int)].occupant_ = occ;

                                // Add occupant to list of tanks.
                                if (occ != NO_OCCUPANT)
                                {
                                    Tank & this_tank = tanks_[occ];
                                    view.visible_tanks.push_back(this_tank);
                                }
                            }
                        }

                        continue;
                    }

                }
            }

            // Otherwise we need to deal with diagonal rays
            //
            // This can be done with 9 rays total
            if (curr_tank.barrel_direction_ % 2 == 1)
            {
                // Since we need to move in both y and x directions, we should
                // use a parametric ray cast.

                for (int r = 0; r < 9; r++)
                {
                    vec2 m = diagonal_slopes[r];
                    float size = diagonal_vec_size[r];
                    float max_range = diagonal_ray_dists[r];

                    cast_ray(view, curr_tank.pos_, m, size, max_range, curr_tank.barrel_direction_);
                }


            }
        }
    }

    return view;
}

PlayerView GameInstance::dump_global_view()
{
    uint8_t width = game_env_.get_width();
    uint8_t height = game_env_.get_height();
    uint16_t total = width * height;

    PlayerView view(width, height);

    // Copy the entire board.
    view.map_view = game_env_;

    // Set each cell to visible.
    for (int i = 0; i < total; i++)
    {
        view.map_view[i].visible_ = true;
    }

    // Copy each living tank, they should all be visible.
    for (const auto & tank : tanks_)
    {
        if (tank.health_ > 0)
        {
            view.visible_tanks.push_back(tank);
        }
    }

    return view;
}

// Given <x_0, y_0> and a slope vector <x_s, y_x> we
// compute the visibility along:
//
// r(t) = <x_0, y_0> + t * <x_s, y_s>
//
// within a certain distance bound.
void GameInstance::cast_ray(PlayerView & player_view, vec2 start, vec2 slope, float size, float max_range, uint8_t dir) const
{
    // by default the slopes are setup for south east
    // since moving up in the game world is equivalent
    // to moving down on the y axis
    int x_sign = 1;
    int y_sign = 1;

    FlatArray<GridCell> & view = player_view.map_view;

    // prepare slopes for north east
    if (dir % 8 == 1)
    {
        y_sign = -1;
    }
    // prepare slopes for south west
    else if (dir % 8 == 5)
    {
        x_sign = -1;
    }
    // prepare slopes for north west
    else if (dir % 8 == 7)
    {
        x_sign = -1;
        y_sign = -1;
    }

    // make step size based on the vector size
    float dt = 0.5f / size;
    unsigned int x_0 = start.x_;
    unsigned int y_0 = start.y_;

    // step through the ray
    for (float t = dt; t <= max_range; t += dt)
    {
        float x_t = x_0 + t * (x_sign * slope.x_);
        float y_t = y_0 + t * (y_sign * slope.y_);

        // coordinates for x, y
        unsigned int cx_t = std::floor(x_t);
        unsigned int cy_t = std::floor(y_t);

        float frac_x = x_t - cx_t;
        float frac_y = y_t - cy_t;


        // Find the closest cell to the ray
        if (frac_x > 0.5f)
        {
            cx_t = cx_t + 1;
        }
        if (frac_y > 0.5f)
        {
            cy_t = cy_t + 1;
        }

        // check if this is inside the game environemnt
        if (cx_t > uint8_t(game_env_.get_width() - 1) || cy_t > uint8_t(game_env_.get_height() - 1))
        {
            break;
        }
        // check if this is terrain
        GridCell curr_cell = game_env_[idx(cx_t, cy_t)];
        if (curr_cell.type_ == CellType::Terrain)
        {
            // Set the mountain as visible and break.
            view[idx(cx_t, cy_t)].visible_ = true;
            break;
        }

        // otherwise, mark the cell as visible
        view[idx(cx_t, cy_t)].visible_ = true;

        uint8_t occ = game_env_[idx(cx_t, cy_t)].occupant_;
        view[idx(cx_t, cy_t)].occupant_ = occ;

        // Add occupant to list of tanks.
        if (occ != NO_OCCUPANT)
        {
            const Tank & this_tank = tanks_[occ];
            player_view.visible_tanks.push_back(this_tank);
        }

        continue;
    }

    return;
}

void GameInstance::place_tank(vec2 pos,
                              uint8_t player_ID,
                              uint8_t placement_direction)
{

    GridCell & this_cell = game_env_[idx(pos)];
    Player & this_player = players_[player_ID];

    uint8_t tank_ID = this_player.tanks_placed_ + (player_ID * num_tanks_);

    // place a tank in the array
    Tank & this_tank = tanks_[tank_ID];
    this_tank.pos_ = pos;
    this_tank.owner_ = player_ID;
    this_tank.health_ = 3;
    this_tank.loaded_ = true;
    this_tank.id_ = tank_ID;
    this_tank.current_direction_ = placement_direction;
    this_tank.barrel_direction_ = placement_direction;

    // set occupant
    this_cell.occupant_ = tank_ID;

    // update the tank list for the player
    std::vector<int> & tank_list = this_player.get_tanks_list();
    tank_list[this_player.tanks_placed_] = tank_ID;
    this_player.tanks_placed_ += 1;

    // destroy tank if placed on terrain
    if (this_cell.type_ == CellType::Terrain)
    {
        this_tank.health_ = 0;
    }

    return;

}

void GameInstance::remove_tank(vec2 pos, uint8_t player_ID)
{
    GridCell & this_cell = game_env_[idx(pos)];
    Player & this_player = players_[player_ID];

    uint8_t tank_ID = (this_player.tanks_placed_ - 1) + (player_ID * num_tanks_);

    // Set the tank to destroyed with no owner.
    Tank & this_tank = tanks_[tank_ID];
    this_tank.pos_ = NO_POS_VEC;
    this_tank.owner_ = NO_OWNER;
    this_tank.health_ = 0;
    this_tank.loaded_ = false;
    this_tank.id_ = tank_ID;
    this_tank.current_direction_ = 0;
    this_tank.barrel_direction_ = 0;

    // Remove the occupant
    this_cell.occupant_ = NO_TANK;

    // Reduce the number of tanks.
    this_player.tanks_placed_ -= 1;

    // Remove the tank from the list for this player.
    std::vector<int> & tank_list = this_player.get_tanks_list();
    tank_list[this_player.tanks_placed_] = NO_TANK;

    return;
}

void GameInstance::load_tank(uint8_t ID)
{
    tanks_[ID].loaded_ = true;
    return;
}

void GameInstance::unload_tank(uint8_t ID)
{
    tanks_[ID].loaded_ = false;
    return;
}

const std::vector<uint8_t> GameInstance::get_mask()
{
    return placement_mask_;
}

bool GameInstance::read_env_by_name(const std::string& filename,
                                    uint16_t total)
{
    std::error_code ec;

    std::filesystem::path map_path = std::filesystem::path("envs") / filename;

    if (!std::filesystem::is_regular_file(map_path, ec))
    {
        std::cerr << "Environment file " << filename << " not found or not regular\n";
        return false;
    }

    auto size = std::filesystem::file_size(map_path, ec);

    if (ec)
    {
        std::cerr << "Unable to query file size \n";
        return false;
    }

    if (size != static_cast<std::size_t>(total) * 2)
    {
        std::cerr << "File size mismatch in environment setup.\n";
        std::cerr << size << "\n";
        return false;
    }

    std::ifstream file(map_path, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "ERROR IN FILE OPENING\n";
        return false;
    }

    // read the file all at once into the buffer
    std::vector<char> env_buffer(total);
    file.read(env_buffer.data(), total);

    if (file.gcount() != total)
    {
        std::cerr << "Too few bytes for environment\n";
        return false;
    }

    if (!file)
    {
        std::cerr << "IO error reading environment\n";
        return false;
    }

    // Read bitmask for tank placements.
    std::vector<char> mask_buffer(total);
    file.read(mask_buffer.data(), total);

    if (file.gcount() != total)
    {
        std::cerr << "Too few bytes for mask\n";
        return false;
    }

    if (!file)
    {
        std::cerr << "IO error reading mask\n";
        return false;
    }

    // turn the ASCII data (48, 49, 50..) into the correct integers (0,1,2)
    // and assign the GridCell elements the correct type accordingly.
    for (uint16_t i = 0; i < total; i++)
    {
        char c = env_buffer[i];
        if (c < '0' || c > '2')
        {
            std::cerr << "Invalid environment character \n";
            return false;
        }
        uint8_t value = static_cast<uint8_t>(c - '0');

        // CellType is an enum derived from uint8_t, hence the two casts.
        //
        // Normally you shouldn't do this, but we're reading a chunk of data.
        game_env_[i].type_ = static_cast<CellType>(value);
        game_env_[i].occupant_ = NO_OCCUPANT;
        game_env_[i].visible_ = true;

        // Convert the bitmask for each player.
        //
        // Places where a player may place their tank are identified
        // by their player ID.
        placement_mask_[i] = static_cast<uint8_t>(mask_buffer[i] - '0');
    }

    return true;

}
