#pragma once

#include <QAbstractListModel>
#include <QVariantMap>
#include <QObject>

#include "player-view.h"
#include "message-structs.h"

class GameManager : public QAbstractListModel
{
    using PlaySoundCallback = std::function<void()>;

    Q_OBJECT

    Q_PROPERTY(int map_width
               READ map_width
               NOTIFY map_width_changed)

    Q_PROPERTY(int map_height
               READ map_height
               NOTIFY map_height_changed)

    Q_PROPERTY(int state
               READ state
               NOTIFY state_changed)

    Q_PROPERTY(int fuel
               READ fuel
               NOTIFY fuel_changed)

    Q_PROPERTY(QString player
               READ player
               NOTIFY player_changed)
public:

    enum Roles {
        TypeRole = Qt::UserRole + 1,
        OccupantRole,
        VisibleRole
    };

    GameManager(QObject * parent = nullptr,
                PlaySoundCallback sound_callback = nullptr);

    int rowCount(const QModelIndex & parent) const override;

    QVariant data(const QModelIndex & index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    // Callable by GUI to fetch cell.
    Q_INVOKABLE QVariantMap cell_at(int x, int y) const;

    Q_INVOKABLE QVariantMap get_tank_data(int occupant) const;

    // Callable from C++ code for changing the view.
    void update_view(PlayerView new_view);

    void update_match_data(StaticMatchData data, std::string username);

    int map_width() const;

    int map_height() const;

    int state() const;

    int fuel() const;

    QString player() const;

    uint16_t sequence_number();

    uint8_t tank_at(int x, int y);

    Q_INVOKABLE bool is_turn();

    Q_INVOKABLE bool is_friendly_tank(uint8_t tank_id);

    Q_INVOKABLE bool valid_placement_tile(int x, int y);

    bool tank_has_ammo(uint8_t tank_id);

signals:
    void view_changed();

    void map_width_changed();

    void map_height_changed();

    void state_changed();

    void fuel_changed();

    void player_changed();

private:
    PlayerView current_view_;
    StaticMatchData current_data_;

    uint8_t player_id_{UINT8_MAX};

    // Increasing number across a game,
    uint16_t sequence_number_{0};

    PlaySoundCallback sound_callback_;
};
