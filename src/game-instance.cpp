#include "game-instance.h"

// Not useful for now
GameInstance::GameInstance(uint8_t input_width, uint8_t input_height, Tank* starting_locations)
:game_env_(input_width, input_height), tanks_(starting_locations)
{
    return;
}

GameInstance::GameInstance(uint8_t input_width, uint8_t input_height, const std::string& filename, Tank* starting_locations, uint8_t num_tanks, Player* players, uint8_t num_players)
:num_players_(num_players), num_tanks_(num_tanks), game_env_(input_width, input_height, input_width*input_height), tanks_(starting_locations), players_(players)
{
    read_env_by_name(filename, input_width*input_height);

    // set tank positions
    for (int i = 0; i < num_tanks; i++)
    {
        Tank curr_tank = tanks_[i];
        vec2 tank_pos = curr_tank.pos_;
        // for the index of the environment where the
        // tank is, set the occupant of the tile to the tank number.
        (game_env_[idx(tank_pos.x_, tank_pos.y_)]).occupant_ = i;
    }

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

    for (int tank_idx = 0; tank_idx < num_tanks_; tank_idx++)
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

            if (curr.occupant_ == UINT8_MAX)
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
bool GameInstance::move_tank(uint8_t ID)
{
    Tank& curr_tank = tanks_[ID];
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
    //
    // Otherwise, we make sure the tank doesn't move into terrain or another player

    switch (dir)
    {
        case 0:
        {
            if (prev.y_ - 1 > game_env_.get_height() - 1)
            {
                return false;
            }
            curr_tank.pos_.y_ -= 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != UINT8_MAX)
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
            if ((prev.y_ - 1 > game_env_.get_height() - 1) || (prev.x_ + 1 > game_env_.get_width() - 1))
            {
                return false;
            }
            curr_tank.pos_.y_ -= 1;
            curr_tank.pos_.x_ += 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != UINT8_MAX)
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
            if (prev.x_ + 1 > game_env_.get_width() - 1)
            {
                return false;
            }
            curr_tank.pos_.x_ += 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != UINT8_MAX)
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
            if ((prev.y_ + 1 > game_env_.get_height() - 1) || (prev.x_ + 1 > game_env_.get_width() - 1))
            {
                return false;
            }
            curr_tank.pos_.y_ += 1;
            curr_tank.pos_.x_ += 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != UINT8_MAX)
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
            if (prev.y_ + 1 > game_env_.get_height() - 1)
            {
                return false;
            }
            curr_tank.pos_.y_ += 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != UINT8_MAX)
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
            if ((prev.y_ + 1 > game_env_.get_height() - 1) || (prev.x_ - 1 > game_env_.get_width() - 1))
            {
                return false;
            }
            curr_tank.pos_.y_ += 1;
            curr_tank.pos_.x_ -= 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != UINT8_MAX)
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
            if (prev.x_ - 1 > game_env_.get_height() - 1)
            {
                return false;
            }
            curr_tank.pos_.x_ -= 1;
            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != UINT8_MAX)
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
            if ((prev.y_ - 1 > game_env_.get_height() - 1) || (prev.x_ - 1 > game_env_.get_width() - 1))
            {
                return false;
            }
            curr_tank.pos_.y_ -= 1;
            curr_tank.pos_.x_ -= 1;

            // CHECK IF TERRAIN/OCCUPIED, IF SO, REVERT
            GridCell moved_cell = game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)];
            if (moved_cell.occupant_ != UINT8_MAX)
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
    game_env_[idx(prev.x_, prev.y_)].occupant_ = UINT8_MAX;

    // Make new cell occupied with our tank ID
    game_env_[idx(curr_tank.pos_.x_, curr_tank.pos_.y_)].occupant_ = ID;

    return true;
}

bool GameInstance::fire_tank(uint8_t ID)
{
    Tank& curr_tank = tanks_[ID];

    vec2 shell_dir = dir_to_vec(curr_tank.barrel_direction_);
    vec2 curr_loc = curr_tank.pos_;

    uint8_t firing_dist = 3;
    uint8_t damage = 1;

    for (int i = 1; i <= firing_dist; i++)
    {
        // take a step
        curr_loc = curr_loc + shell_dir;

        // test if this tile is out of bounds
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
        if (curr_cell.occupant_ != UINT8_MAX)
        {
            Tank& hit_tank = tanks_[curr_cell.occupant_];
            hit_tank.deal_damage(damage);

            // if the health of the tank is zero, remove it from play
            if (hit_tank.health_ == 0)
            {
                vec2 hit_pos = hit_tank.pos_;
                game_env_[idx(hit_pos)].occupant_ = UINT8_MAX;
            }

            return true;
        }

    }
    return false;
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

        game_env_[i].occupant_ = UINT8_MAX;
    }

}
