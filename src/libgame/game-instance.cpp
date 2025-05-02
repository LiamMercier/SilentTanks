#include "game-instance.h"
#include "constants.h"

GameInstance::GameInstance(const GameMap & map, uint8_t num_players)
:num_players_(num_players), num_tanks_(map.num_tanks), game_env_(map.width, map.height, map.width*map.height), tanks_(new (std::nothrow) Tank[map.num_tanks * num_players]{})
{
    // check that allocation didn't fail
    if (tanks_ == nullptr)
    {
        std::cerr << "game-instance: Memory allocation failed\n";
        return;
    }

    // Allocate players_ vector
    players_.reserve(num_players_);

    // Create each tank in place
    for (uint8_t i = 0; i < num_players_; ++i)
    {
        players_.emplace_back(num_tanks_, i);
    }

    // Get map from file
    read_env_by_name(map.filename, map.width*map.height);
}

GameInstance::~GameInstance()
{
    delete[] tanks_;
}

// Temporary array to hold colors for console print testing
const static std::string colors_array[4] = {"\033[48;5;196m", "\033[48;5;21m", "\033[48;5;46m", "\033[48;5;226m"};

// Print information to the console
void GameInstance::print_instance_console() const
{

    for (int tank_idx = 0; tank_idx < num_tanks_ * num_players_; tank_idx++)
    {
        Tank this_tank = tanks_[tank_idx];
        if (this_tank.health_ > 0)
        {
            this_tank.print_tank_state(tank_idx);
        }
    }

    for (int y = 0; y < game_env_.get_height(); y++)
    {
        for (int x = 0; x < game_env_.get_width(); x++)
        {
            GridCell curr = game_env_[idx(x,y)];

            if (curr.occupant_ == NO_OCCUPANT)
            {
                std::cout << curr << " ";
            }
            else
            {
                Tank this_tank = tanks_[curr.occupant_];
                uint8_t tank_owner = this_tank.get_owner();
                std::cout << colors_array[tank_owner] << +(curr.occupant_) << "\033[0m ";
            }
        }
        std::cout << "\n";
    }

}

// Rotate a tank of given ID
void GameInstance::rotate_tank(uint8_t ID, uint8_t dir)
{
    Tank& curr_tank = tanks_[ID];
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
    Tank& curr_tank = tanks_[ID];
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
bool GameInstance::move_tank(uint8_t ID, bool reverse))
{
    Tank& curr_tank = tanks_[ID];
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
    Tank& curr_tank = tanks_[ID];

    vec2 shell_dir = dir_to_vec[curr_tank.barrel_direction_];
    vec2 curr_loc = curr_tank.pos_;

    // Expend the shell
    curr_tank.loaded_ = false;

    for (int i = 1; i <= FIRING_DIST; i++)
    {
        // Take a step
        curr_loc = curr_loc + shell_dir;

        // Test if this tile is out of bounds
        if ((curr_loc.x_ > game_env_.get_height() - 1)
            || (curr_loc.y_ > game_env_.get_width() - 1))
        {
                return false;
        }

        GridCell curr_cell = game_env_[idx(curr_loc)];

        // test if the tile is terrain
        if (curr_cell.type_ == CellType::Terrain)
        {
            return false;
        }

        // test if the tile has a tank
        if (curr_cell.occupant_ != NO_OCCUPANT)
        {
            Tank& hit_tank = tanks_[curr_cell.occupant_];
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

// Below is probably the worst function in the entire code base.
//
// Should really be re-worked one day.

// Compute the view for this player
//
// We want to iterate over every tank owned by the
// player, check that the tank is still in play, then
// compute the vision that this tank generates
//
// We start with the current game environment and set all cells
// to having no vision
Environment GameInstance::compute_view(uint8_t player_ID, uint8_t & live_tanks)
{

    uint8_t width = game_env_.get_width();
    uint8_t height = game_env_.get_height();
    uint16_t total = width * height;

    // make a new environemnt
    Environment player_view(width, height);

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
    const int* player_tank_IDs = this_player.get_tanks_list();

    for (size_t i = 0; i < num_tanks_; i++)
    {
        uint8_t curr_tank_ID = player_tank_IDs[i];

        if (curr_tank_ID == NO_TANK)
        {
            continue;
        }

        Tank& curr_tank = tanks_[curr_tank_ID];

        // Check that the tank is still alive
        if (curr_tank.health_ == 0)
        {
            continue;
        }

        // otherwise increment the number of alive tanks
        live_tanks += 1;

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
        // TODO: Convert this mess into a compatible cast_ray version
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
                            if (primary_var_y == true)
                            {
                                curr_cell = game_env_[idx(sec_int, p)];
                            }
                            else
                            {
                                curr_cell = game_env_[idx(p, sec_int)];
                            }

                            if (curr_cell.type_ == CellType::Terrain)
                            {
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
                            if (primary_var_y == true)
                            {
                                curr_cell = game_env_[idx(sec_int, p)];
                            }
                            else {
                                curr_cell = game_env_[idx(p, sec_int)];
                            }

                            if (curr_cell.type_ == CellType::Terrain)
                            {
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
                                player_view[idx(sec_int, p)].occupant_ = game_env_[idx(sec_int, p)].occupant_;
                            }
                            else {
                                player_view[idx(p, sec_int)].visible_ = true;
                                player_view[idx(p, sec_int)].occupant_ = game_env_[idx(p, sec_int)].occupant_;
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

                    cast_ray(player_view, curr_tank.pos_, m, size, max_range, curr_tank.barrel_direction_);
                }


            }
        }
    }
    return player_view;
}

// Given <x_0, y_0> and a slope vector <x_s, y_x> we
// compute the visibility along:
//
// r(t) = <x_0, y_0> + t * <x_s, y_s>
//
// within a certain distance bound.
void GameInstance::cast_ray(Environment & view, vec2 start, vec2 slope, float size, float max_range, uint8_t dir) const
{
    // by default the slopes are setup for south east
    // since moving up in the game world is equivalent
    // to moving down on the y axis
    int x_sign = 1;
    int y_sign = 1;

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
            break;
        }
        // otherwise, mark the cell as visible
        view[idx(cx_t, cy_t)].visible_ = true;
        view[idx(cx_t, cy_t)].occupant_ = game_env_[idx(cx_t, cy_t)].occupant_;

        continue;
    }

    return;
}

void GameInstance::place_tank(vec2 pos, uint8_t player_ID)
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

    // set occupant
    this_cell.occupant_ = tank_ID;

    // update the tank list for the player
    int * tank_list = this_player.get_tanks_list();
    tank_list[this_player.tanks_placed_] = tank_ID;
    this_player.tanks_placed_ += 1;

    // destroy tank if placed on terrain
    if (this_cell.type_ == CellType::Terrain)
    {
        this_tank.health_ = 0;
    }

    return;

}

void GameInstance::load_tank(uint8_t ID)
{
    tanks_[ID].loaded_ = true;
    return;
}

// This should really be replaced in the future with a more reasonable solution.
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

        game_env_[i].occupant_ = NO_OCCUPANT;
        game_env_[i].visible_ = true;
    }

}
