#include "replay-manager.h"
#include "client-state.h"

constexpr bool URGENT_POPUP = true;
constexpr bool STANDARD_POPUP = false;

// Helper to compare usernames.
constexpr bool same_username(const std::string & a, const std::string & b)
{
    if (a.size() != b.size())
    {
            return false;
    }

    for (size_t i = 0; i < a.size(); i++)
    {
        char ca = a[i];
        char cb = b[i];

        if (ca >= 'A' && ca <= 'Z')
        {
            ca += 'a' - 'A';
        }

        if (cb >= 'A' && cb <= 'Z')
        {
            cb += 'a' - 'A';
        }

        if (ca != cb)
        {
            return false;
        }
    }

    return true;
}

ReplayManager::ReplayManager(QObject * parent,
                             PlaySoundCallback sound_callback,
                             PopupCallback popup_callback)
:QAbstractListModel(parent),
sound_callback_(sound_callback),
popup_callback_(popup_callback)
{
}

int ReplayManager::rowCount(const QModelIndex & parent = QModelIndex()) const
{
    size_t row_count = 0;
    {
        std::lock_guard lock(replay_mutex_);
        row_count = replays_.size();
    }
    return static_cast<int>(row_count);
}

QVariant ReplayManager::data(const QModelIndex & index, int role) const
{

    // std::lock_guard lock(replay_mutex_);
    if (!index.isValid()
        || index.row() >= static_cast<int>(replays_.size())
        || index.row() < 0)
    {
        return {};
    }

    // TODO: replay data
    /*
    switch (role)
    {
        case TypeRole:
            return
        case OccupantRole:
            return
        case VisibleRole:
            return
        default:
            return {};
    }
    */
    return {};
}

QHash<int, QByteArray> ReplayManager::roleNames() const
{
    QHash<int, QByteArray> roles;
    // roles[TypeRole] = "type";
    // roles[OccupantRole] = "occupant";
    // roles[VisibleRole] = "visible";
    return roles;
}

// Fetch cell data for render.
Q_INVOKABLE QVariantMap ReplayManager::cell_at(int x, int y) const
{
    QVariantMap m;
    if (x < 0
        || y < 0
        || x >= current_view_.width()
        || y >= current_view_.height())
    {
        return m;
    }

    size_t index = current_view_.indx(x, y);
    const GridCell & cell = current_view_.map_view[index];

    m["type"] = static_cast<int>(cell.type_);
    m["occupant"] = static_cast<int>(cell.occupant_);
    m["visible"] = cell.visible_;
    return m;
}

Q_INVOKABLE QVariantMap ReplayManager::get_tank_data(int occupant) const
{
    QVariantMap m;
    if (occupant < 0 || occupant >= NO_OCCUPANT)
    {
        return m;
    }

    const Tank & tank = current_view_.find_tank(occupant);

    m["tank_direction"] = tank.current_direction_;
    m["barrel_direction"] = tank.barrel_direction_;
    m["health"] = tank.health_;
    m["loaded"] = tank.loaded_;
    m["owner"] = tank.owner_;

    return m;
}

UserListModel* ReplayManager::players_model()
{
    return & players_;
}

void ReplayManager::add_replay(MatchReplay replay)
{
    // Compute the size of the replay, in bytes.
    size_t replay_bytes = replay.get_size_in_bytes();

    // If we have too much data, drop the replay and show
    // a popup to the user.

    if (replay_bytes + total_replay_bytes_ > MAX_REPLAY_BYTES)
    {
        // TODO: consider writing replays to disk to keep allowing downloads.
        std::string oom_msg = "Replay "
                              + std::to_string(replay.match_id)
                              + " was dropped because the limit of"
                              + " bytes for replays was reached.";

        popup_callback_(
            Popup(PopupType::Info, "Replay Dropped", oom_msg),
                  URGENT_POPUP);

        return;
    }

    qint64 id = static_cast<qint64>(replay.match_id);

    int newRow = 0;
    {
        std::lock_guard lock(replay_mutex_);

        // Otherwise, we can add this replay.
        newRow = static_cast<int>(replays_.size());
    }

    beginInsertRows(QModelIndex(), newRow, newRow);
    {
        std::lock_guard lock(replay_mutex_);
        replays_.push_back(std::move(replay));
    }
    endInsertRows();

    total_replay_bytes_ += replay_bytes;

    // Enable the replay button.
    emit match_downloaded(id);
}

Q_INVOKABLE void ReplayManager::set_replay(qint64 match_id)
{
    bool environment_loaded = false;
    uint64_t turn_count = 0;

    {
        std::lock_guard lock(replay_mutex_);

        // Look for this replay ID, otherwise abort if we do not have the replay stored.
        bool found_replay = false;

        for (uint64_t i = 0; i < replays_.size(); i++)
        {
            const auto & replay = replays_[i];

            if (replay.match_id == static_cast<uint64_t>(match_id))
            {
                found_replay = true;
                current_replay_index_ = i;
                break;
            }
        }

        // Handle when replay is not found.
        if (!found_replay)
        {
            std::string not_found_msg = "Replay "
                                        + std::to_string(match_id)
                                        + " was unable to be found.";

            popup_callback_(
                Popup(PopupType::Info, "Replay Not Found", not_found_msg),
                    URGENT_POPUP);
            return;
        }

        // Load map file into game instance.
        const MatchReplay & current_replay = replays_[current_replay_index_];
        turn_count = current_replay.moves.size();

        current_instance_ = GameInstance(current_replay.settings);

        uint16_t total = current_replay.settings.width
                        * current_replay.settings.height;

        environment_loaded = current_instance_.read_env_by_name
                                (
                                    current_replay.settings.filename,
                                    total
                                );
    }

    if (!environment_loaded)
    {
        std::string no_env_msg = "The environment was unable to be loaded";

        popup_callback_(
            Popup(PopupType::Info, "Replay Failed", no_env_msg),
                URGENT_POPUP);
        return;
    }

    // Do setup.
    current_view_.current_player = 0;
    current_view_.current_fuel = TURN_PLAYER_FUEL;
    current_view_.current_state = GameState::Setup;

    applied_moves_ = 0;
    current_perspective_ = NO_PLAYER;
    move_status_ = std::vector<MoveStatus>(turn_count);

    // Compute the starting view.
    update_view();
}

Q_INVOKABLE void ReplayManager::step_forward_turn()
{
    CommandHead cmd;

    {
        std::lock_guard lock(replay_mutex_);
        const MatchReplay & current_replay = replays_[current_replay_index_];

        // Handle no next move existing.
        if (applied_moves_ >= current_replay.moves.size())
        {
            return;
        }

        // Grab the next turn to apply. This is equal to the number
        // of applied turns, since moves are stored in a vector.
        cmd = current_replay.moves[applied_moves_];
    }

    std::cout << "Applying turn " << applied_moves_ << "\n";

    // Apply the command based on the type.
    bool valid_move = true;
    std::string popup_message;

    switch(cmd.type)
    {
        case CommandType::Move:
        {
            // If the tank does not exist, stop early.
            if (cmd.tank_id >= current_instance_.num_players_
                                * current_instance_.num_tanks_)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(tank does not exist)";
                valid_move = false;
                break;
            }

            Tank & this_tank = current_instance_.get_tank(cmd.tank_id);

            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(sender mismatch or tank is dead)";
                valid_move = false;
                break;
            }

            move_status_[applied_moves_] = current_instance_.move_tank(cmd.tank_id,
                                                                    cmd.payload_first);

            break;
        }
        case CommandType::RotateTank:
        {
            // If the tank does not exist, stop early.
            if (cmd.tank_id >= current_instance_.num_players_
                                * current_instance_.num_tanks_)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(tank does not exist)";
                valid_move = false;
                break;
            }

            Tank & this_tank = current_instance_.get_tank(cmd.tank_id);

            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(sender mismatch or tank is dead)";
                valid_move = false;
                break;
            }

            current_instance_.rotate_tank(cmd.tank_id, cmd.payload_first);
            move_status_[applied_moves_] = true;

            break;
        }
        case CommandType::RotateBarrel:
        {
            // If the tank does not exist, stop early.
            if (cmd.tank_id >= current_instance_.num_players_
                                * current_instance_.num_tanks_)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(tank does not exist)";
                valid_move = false;
                break;
            }

            Tank & this_tank = current_instance_.get_tank(cmd.tank_id);

            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(sender mismatch or tank is dead)";
                valid_move = false;
                break;
            }

            current_instance_.rotate_tank_barrel(cmd.tank_id, cmd.payload_first);
            move_status_[applied_moves_] = true;

            break;
        }
        case CommandType::Fire:
        {
            // If the tank does not exist, stop early.
            if (cmd.tank_id >= current_instance_.num_players_
                                * current_instance_.num_tanks_)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(tank does not exist)";
                valid_move = false;
                break;
            }

            Tank & this_tank = current_instance_.get_tank(cmd.tank_id);

            // check that the tank is alive
            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(sender mismatch or tank is dead)";
                valid_move = false;
                break;
            }

            // check that the tank is able to fire
            if (this_tank.loaded_ == false)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(tank was not loaded)";
                valid_move = false;
                break;
            }

            // fire
            MoveStatus status = current_instance_.replay_fire_tank(cmd.tank_id);
            move_status_[applied_moves_] = std::move(status);

            break;
        }
        case CommandType::Load:
        {
            // If the tank does not exist, stop early.
            if (cmd.tank_id >= current_instance_.num_players_
                                * current_instance_.num_tanks_)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(tank does not exist)";
                valid_move = false;
                break;
            }

            Tank & this_tank = current_instance_.get_tank(cmd.tank_id);

            if (this_tank.health_ == 0
                || this_tank.loaded_ == true
                || this_tank.owner_ != cmd.sender)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(sender mismatch or tank is dead)";
                valid_move = false;
                break;
            }

            // if unloaded, load the tank
            current_instance_.load_tank(cmd.tank_id);
            move_status_[applied_moves_] = true;

            break;
        }
        case CommandType::Place:
        {
            vec2 pos(cmd.payload_first, cmd.payload_second);

            if (pos.x_ > current_instance_.get_width() - 1
                || pos.y_ > current_instance_.get_height() - 1)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(placement out of bounds)";
                valid_move = false;
                break;
            }

            uint8_t placement_direction = cmd.tank_id;

            if (placement_direction >= 8)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(placement direction invalid)";
                valid_move = false;
                break;
            }

            current_instance_.place_tank(pos,
                                         cmd.sender,
                                         placement_direction);

            move_status_[applied_moves_] = true;

            break;
        }
        default:
        {
            popup_message = "Replay failed because move "
                                + std::to_string(applied_moves_)
                                + " was malformed \n(invalid command type)";
            valid_move = false;
            break;
        }
    }

    if (!valid_move)
    {
        popup_callback_(
            Popup(PopupType::Info, "Move Failed", popup_message),
                  URGENT_POPUP);
        return;
    }

    // If we got here, a command was applied successfully
    // so we can increment the counter.
    applied_moves_ += 1;

    // If we're in the setup phase, don't consume fuel.
    if (current_view_.current_state == GameState::Setup)
    {
        // If the next move is not a placement command, set state to playing.
        CommandHead next_cmd;
        {
            std::lock_guard lock(replay_mutex_);

            const MatchReplay & current_replay = replays_[current_replay_index_];

            // Handle no next move existing.
            if (applied_moves_ >= current_replay.moves.size())
            {
                update_view();
                return;
            }

            next_cmd = current_replay.moves[applied_moves_];
        }

        if (next_cmd.type != CommandType::Place)
        {
            current_view_.current_state = GameState::Play;
            current_view_.current_fuel = TURN_PLAYER_FUEL;
        }

        // Update player to the next person in play.
        current_view_.current_player = next_cmd.sender;

        update_view();
        return;
    }

    // Otherwise remove fuel.
    current_view_.current_fuel = current_view_.current_fuel - 1;

    if (current_view_.current_fuel > 0)
    {
        update_view();
        return;
    }
    // Update the player and reset fuel if previous player is out.
    else
    {
        CommandHead next_cmd;
        {
            std::lock_guard lock(replay_mutex_);

            const MatchReplay & current_replay = replays_[current_replay_index_];

            // Handle no next move existing.
            if (applied_moves_ >= current_replay.moves.size())
            {
                update_view();
                return;
            }

            next_cmd = current_replay.moves[applied_moves_];
        }

        current_view_.current_player = next_cmd.sender;
        current_view_.current_fuel = TURN_PLAYER_FUEL;

        update_view();
        return;
    }
}

Q_INVOKABLE void ReplayManager::step_backward_turn()
{
    // If no moves have been applied, do nothing.
    if (applied_moves_ == 0)
    {
        return;
    }

    uint64_t last_turn = applied_moves_ - 1;

    CommandHead cmd;
    {
        std::lock_guard lock(replay_mutex_);
        const MatchReplay & current_replay = replays_[current_replay_index_];

        // Grab the next turn to apply. This is equal to the number
        // of applied turns, since moves are stored in a vector.
        cmd = current_replay.moves[last_turn];
    }

    std::cout << "Reverting turn " << last_turn << "\n";

    // Apply the command based on the type.
    bool valid_move = true;
    std::string popup_message;

    switch(cmd.type)
    {
        case CommandType::Move:
        {
            // If the tank did not exist, stop early.
            if (cmd.tank_id >= current_instance_.num_players_
                                * current_instance_.num_tanks_)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(tank does not exist)";
                valid_move = false;
                break;
            }

            Tank & this_tank = current_instance_.get_tank(cmd.tank_id);

            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(sender mismatch or tank is dead)";
                valid_move = false;
                break;
            }

            // We need to revert the movement action, but it's possible that
            // the move did nothing, so only reverse the tank if we moved.
            if (move_status_[last_turn].success)
            {
                bool reversed_dir = (cmd.payload_first == 0);
                current_instance_.move_tank(cmd.tank_id,
                                            reversed_dir);
            }

            break;
        }
        case CommandType::RotateTank:
        {
            // If the tank did not exist, stop early.
            if (cmd.tank_id >= current_instance_.num_players_
                                * current_instance_.num_tanks_)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(tank does not exist)";
                valid_move = false;
                break;
            }

            Tank & this_tank = current_instance_.get_tank(cmd.tank_id);

            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(sender mismatch or tank is dead)";
                valid_move = false;
                break;
            }
            bool reversed_dir = (cmd.payload_first == 0);
            current_instance_.rotate_tank(cmd.tank_id, reversed_dir);
            break;
        }
        case CommandType::RotateBarrel:
        {
            // If the tank did not exist, stop early.
            if (cmd.tank_id >= current_instance_.num_players_
                                * current_instance_.num_tanks_)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(tank does not exist)";
                valid_move = false;
                break;
            }

            Tank & this_tank = current_instance_.get_tank(cmd.tank_id);

            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(sender mismatch or tank is dead)";
                valid_move = false;
                break;
            }

            bool reversed_dir = (cmd.payload_first == 0);
            current_instance_.rotate_tank_barrel(cmd.tank_id, reversed_dir);
            break;
        }
        case CommandType::Fire:
        {
            // If the tank did not exist, stop early.
            if (cmd.tank_id >= current_instance_.num_players_
                                * current_instance_.num_tanks_)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(tank does not exist)";
                valid_move = false;
                break;
            }

            Tank & this_tank = current_instance_.get_tank(cmd.tank_id);

            // check that the tank was alive
            if (this_tank.health_ == 0 || this_tank.owner_ != cmd.sender)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(sender mismatch or tank is dead)";
                valid_move = false;
                break;
            }

            // check that the tank fired
            if (this_tank.loaded_ == true)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(tank was still loaded)";
                valid_move = false;
                break;
            }

            // Undo the damage if any was taken and reload the tank.
            if (move_status_[last_turn].success)
            {
                for (const auto & hit_pos : move_status_[last_turn].hits)
                {
                    current_instance_.repair_tank(hit_pos);
                }
            }

            current_instance_.load_tank(cmd.tank_id);
            break;
        }
        case CommandType::Load:
        {
            // If the tank did not exist, stop early.
            if (cmd.tank_id >= current_instance_.num_players_
                                * current_instance_.num_tanks_)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(tank does not exist)";
                valid_move = false;
                break;
            }

            Tank & this_tank = current_instance_.get_tank(cmd.tank_id);

            if (this_tank.health_ == 0
                || this_tank.loaded_ == false
                || this_tank.owner_ != cmd.sender)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(sender mismatch or tank is dead)";
                valid_move = false;
                break;
            }

            // Unload the tank
            current_instance_.unload_tank(cmd.tank_id);
            break;
        }
        case CommandType::Place:
        {
            vec2 pos(cmd.payload_first, cmd.payload_second);

            if (pos.x_ > current_instance_.get_width() - 1
                || pos.y_ > current_instance_.get_height() - 1)
            {
                popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(placement out of bounds)";
                valid_move = false;
                break;
            }

            current_instance_.remove_tank(pos,
                                          cmd.sender);

            break;
        }
        default:
        {
            popup_message = "Replay failed because move "
                                + std::to_string(last_turn)
                                + " was malformed \n(invalid command type)";
            valid_move = false;
            break;
        }
    }

    if (!valid_move)
    {
        popup_callback_(
            Popup(PopupType::Info, "Move Failed", popup_message),
                  URGENT_POPUP);
        return;
    }

    // Reduce the move count.
    applied_moves_ -= 1;

    // If we're in the playing phase, check if we need to reverse.
    if (current_view_.current_state == GameState::Play)
    {
        // If the previous move is a placement command, set state to setup.
        CommandHead previous_cmd;
        {
            std::lock_guard lock(replay_mutex_);

            const MatchReplay & current_replay = replays_[current_replay_index_];

            // Handle no previous move existing. This should never occur.
            if (applied_moves_ == 0)
            {
                current_view_.current_player = 0;
                current_view_.current_state = GameState::Setup;
                current_view_.current_fuel = TURN_PLAYER_FUEL;

                update_view();
                return;
            }

            previous_cmd = current_replay.moves[applied_moves_ - 1];
        }

        if (previous_cmd.type == CommandType::Place)
        {
            current_view_.current_state = GameState::Setup;
            current_view_.current_fuel = TURN_PLAYER_FUEL;
            // Update player to the previous person for their setup.
            current_view_.current_player = previous_cmd.sender;

            update_view();
            return;
        }

        // Otherwise add fuel.
        current_view_.current_fuel = current_view_.current_fuel + 1;

        // If we went from 3 fuel to 4 fuel, we should actually roll back to
        // the previous player's last turn.
        if (current_view_.current_fuel > 3)
        {
            current_view_.current_player = previous_cmd.sender;
            current_view_.current_fuel = 1;

            update_view();
            return;
        }

        update_view();
        return;
    }
    // Handle setup moves, simple.
    else
    {
        CommandHead previous_cmd;
        {
            std::lock_guard lock(replay_mutex_);

            const MatchReplay & current_replay = replays_[current_replay_index_];

            // Handle no previous move existing.
            if (applied_moves_ == 0)
            {
                current_view_.current_player = 0;
                current_view_.current_state = GameState::Setup;
                current_view_.current_fuel = TURN_PLAYER_FUEL;

                update_view();
                return;
            }

            previous_cmd = current_replay.moves[applied_moves_ - 1];
        }

        // Update player to the previous person for their setup.
        current_view_.current_player = previous_cmd.sender;

        update_view();
        return;
    }
}

void ReplayManager::update_match_data(StaticMatchData data, std::string username)
{
    // current_data_ = data;
    // players_.set_users(data.player_list);
    //
    // // Game server will dump old commands, so just reset our sequence number.
    // sequence_number_ = 0;
    //
    // // find our player ID.
    // for (unsigned int i = 0; i < current_data_.player_list.users.size(); i++)
    // {
    //     if (same_username(current_data_.player_list.users[i].username, username))
    //     {
    //         player_id_ = static_cast<uint8_t>(i);
    //         break;
    //     }
    // }
    //
    // emit player_changed();
}

int ReplayManager::map_width() const
{
    return static_cast<int>(current_view_.width());
}

int ReplayManager::map_height() const
{
    return static_cast<int>(current_view_.height());
}

int ReplayManager::state() const
{
    return static_cast<int>(current_view_.current_state);
}

int ReplayManager::fuel() const
{
    return static_cast<int>(current_view_.current_fuel);
}

QString ReplayManager::player() const
{
    uint8_t current_player = current_view_.current_player;

    if (current_player >= current_data_.player_list.users.size())
    {
        // Fallback to player number.
        std::string player_str = "Player " + std::to_string(current_player);
        return QString::fromStdString(player_str);
    }

    const auto & user = current_data_.player_list.users[current_player];
    std::string username = user.username;
    return QString::fromStdString(username);
}

uint8_t ReplayManager::tank_at(int x, int y)
{
    return current_view_.map_view[current_view_.indx(x, y)].occupant_;
}

Q_INVOKABLE bool ReplayManager::is_turn()
{
    return current_view_.current_player == current_perspective_;
}

Q_INVOKABLE bool ReplayManager::is_friendly_tank(uint8_t tank_id)
{
    for (const auto tank : current_view_.visible_tanks)
    {
        if (tank.id_ == tank_id)
        {
            return tank.owner_ == current_perspective_;
        }
    }

    // Default to false if we can't find the tank.
    return false;
}

Q_INVOKABLE bool ReplayManager::valid_placement_tile(int x, int y)
{
    size_t index = current_view_.indx(x, y);

    if (index >= current_data_.placement_mask.size())
    {
        return false;
    }

    if (current_view_.map_view[index].occupant_ != NO_OCCUPANT)
    {
        return false;
    }

    // See if this is our tile or not.
    return (current_perspective_ == current_data_.placement_mask[index]);
}

void ReplayManager::update_view()
{
    PlayerView new_view;

    if (current_perspective_ == NO_PLAYER)
    {
        // Dump the entire environment with full visibility.
        new_view = current_instance_.dump_global_view();
    }
    else
    {
        if (current_perspective_ >= current_instance_.num_players_)
        {
            std::string invalid_player_msg = "The perspective selected was out of bounds.";

            popup_callback_(
                Popup(PopupType::Info, "Replay Failed", invalid_player_msg),
                    URGENT_POPUP);
            return;
        }
        uint8_t live_tanks = 0;
        new_view = current_instance_.compute_view(current_perspective_, live_tanks);
    }

    // Keep state information.
    new_view.current_player = current_view_.current_player;
    new_view.current_fuel = current_view_.current_fuel;
    new_view.current_state = current_view_.current_state;

    // Decide if we need to emit that the dimensions changed.
    bool width_changed = false;
    bool height_changed = false;

    if (new_view.width() != current_view_.width())
    {
        width_changed = true;
    }

    if (new_view.height() != current_view_.height())
    {
        height_changed = true;
    }

    // Replace the view.
    current_view_ = new_view;

    if (width_changed)
    {
        emit map_width_changed();
    }
    if (height_changed)
    {
        emit map_height_changed();
    }

    emit view_changed();
    emit state_changed();
    emit fuel_changed();
    emit player_changed();
}
