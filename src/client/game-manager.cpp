#include "game-manager.h"

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

GameManager::GameManager(QObject * parent, PlaySoundCallback sound_callback)
:QAbstractListModel(parent),
sound_callback_(sound_callback)
{
}

int GameManager::rowCount(const QModelIndex & parent = QModelIndex()) const
{
    Q_UNUSED(parent)
    return current_view_.width() * current_view_.height();
}

QVariant GameManager::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()
        || index.row() >= static_cast<int>(current_view_.width()
                                           * current_view_.height())
        || index.row() < 0)
    {
        return {};
    }

    const GridCell & cell = current_view_.map_view[index.row()];

    switch (role)
    {
        case TypeRole:
            return static_cast<int>(cell.type_);
        case OccupantRole:
            return static_cast<int>(cell.occupant_);
        case VisibleRole:
            return cell.visible_;
        default:
            return {};
    }
}

QHash<int, QByteArray> GameManager::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TypeRole] = "type";
    roles[OccupantRole] = "occupant";
    roles[VisibleRole] = "visible";
    return roles;
}

Q_INVOKABLE QVariantMap GameManager::cell_at(int x, int y) const
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

Q_INVOKABLE QVariantMap GameManager::get_tank_data(int occupant) const
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

UserListModel* GameManager::players_model()
{
    return & players_;
}

void GameManager::update_view(PlayerView new_view)
{
    bool width_changed = (new_view.width() != current_view_.width());
    bool height_changed = (new_view.height() != current_view_.height());

    beginResetModel();
    current_view_ = new_view;
    players_.set_timers(current_view_.timers);
    endResetModel();

    if (sound_callback_)
    {
        // Determine if it is now our turn, with 3 fuel.
        if (current_view_.current_player == player_id_
            && current_view_.current_fuel == TURN_PLAYER_FUEL)
        {
            sound_callback_();
        }
    }

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
    emit timers_changed();
}

void GameManager::update_match_data(StaticMatchData data, std::string username)
{
    current_data_ = data;
    players_.set_users(data.player_list);

    // Game server will dump old commands, so just reset our sequence number.
    sequence_number_ = 0;

    // find our player ID.
    for (unsigned int i = 0; i < current_data_.player_list.users.size(); i++)
    {
        if (same_username(current_data_.player_list.users[i].username, username))
        {
            player_id_ = static_cast<uint8_t>(i);
            break;
        }
    }

    emit player_changed();
}

int GameManager::map_width() const
{
    return static_cast<int>(current_view_.width());
}

int GameManager::map_height() const
{
    return static_cast<int>(current_view_.height());
}

int GameManager::state() const
{
    return static_cast<int>(current_view_.current_state);
}

int GameManager::fuel() const
{
    return static_cast<int>(current_view_.current_fuel);
}

QString GameManager::player() const
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

uint16_t GameManager::sequence_number()
{
    return sequence_number_;
}

uint8_t GameManager::tank_at(int x, int y)
{
    return current_view_.map_view[current_view_.indx(x, y)].occupant_;
}

Q_INVOKABLE bool GameManager::is_turn()
{
    return current_view_.current_player == player_id_;
}

Q_INVOKABLE bool GameManager::is_friendly_tank(uint8_t tank_id)
{
    for (const auto tank : current_view_.visible_tanks)
    {
        if (tank.id_ == tank_id)
        {
            return tank.owner_ == player_id_;
        }
    }

    // Default to false if we can't find the tank.
    return false;
}

Q_INVOKABLE bool GameManager::valid_placement_tile(int x, int y)
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
    return (player_id_ == current_data_.placement_mask[index]);
}

bool GameManager::tank_has_ammo(uint8_t tank_id)
{
    for (const auto tank : current_view_.visible_tanks)
    {
        if (tank.id_ == tank_id)
        {
            return tank.loaded_;
        }
    }
    // If we can't find the tank return false.
    return false;
}
