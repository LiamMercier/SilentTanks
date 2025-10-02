#include "replay-manager.h"

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

ReplayManager::ReplayManager(QObject * parent, PlaySoundCallback sound_callback)
:QAbstractListModel(parent),
sound_callback_(sound_callback)
{
}

int ReplayManager::rowCount(const QModelIndex & parent = QModelIndex()) const
{
    return replays_.size();
}

QVariant ReplayManager::data(const QModelIndex & index, int role) const
{
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

        // TODO: popup
        return;
    }

    qint64 id = static_cast<qint64>(replay.match_id);

    // Otherwise, we can add this replay.
    const int newRow = static_cast<int>(replays_.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    replays_.push_back(std::move(replay));
    endInsertRows();

    total_replay_bytes_ += replay_bytes;

    // Enable the replay button.
    emit match_downloaded(id);
}

Q_INVOKABLE void ReplayManager::set_replay(qint64 replay_id)
{
    // TODO: evaluate if necessary.
    current_replay_id_ = static_cast<size_t>(replay_id);

    // Look for this replay ID, otherwise abort if we do not have the replay stored.
    bool found_replay = false;

    for (const MatchReplay & replay : replays_)
    {
        if (replay.match_id == static_cast<uint64_t>(replay_id))
        {
            found_replay = true;
            current_replay_ = replay;
            break;
        }
    }

    // Handle when replay is not found.
    if (!found_replay)
    {
        // TODO: popup
        return;
    }

    // TODO: check MapSettings is valid.

    // Load map file into game instance.
    current_instance_ = GameInstance(current_replay_.settings);

    // Do setup.
    current_view_.current_player = 0;
    current_view_.current_fuel = TURN_PLAYER_FUEL;
    current_view_.current_state = GameState::Setup;

    // Compute the starting view.
    update_view();
}

Q_INVOKABLE void ReplayManager::step_forward_turn()
{

}

Q_INVOKABLE void ReplayManager::step_backward_turn()
{

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
        return QString();
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

    if (index > current_data_.placement_mask.size())
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

// TODO: compute view, global if necessary
void ReplayManager::update_view()
{
    PlayerView new_view;

    if (current_perspective_ == NO_PLAYER)
    {
        // TODO: global dump of environment
    }
    else
    {
        if (current_perspective_ >= current_instance_.num_players_)
        {
            // TODO: popup, error occured.
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
