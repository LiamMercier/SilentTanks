#pragma once

#include <QAbstractListModel>
#include <QObject>

#include "user-list-model.h"
#include "player-view.h"
#include "message-structs.h"
#include "game-instance.h"
#include "client-state.h"
#include "match-result-structs.h"

class ReplayManager : public QAbstractListModel
{
    using PlaySoundCallback = std::function<void(SoundType sound)>;

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
    // Upper bound on how many bytes the client is willing to store
    // as replays. This should generally never be hit under normal operation
    // because the replay structures are often small.
    static constexpr size_t MAX_REPLAY_BYTES = 16 * 1024 * 1024;

    enum Roles {
        TypeRole = Qt::UserRole + 1,
        OccupantRole,
        VisibleRole
    };

    ReplayManager(QObject * parent = nullptr,
                  PlaySoundCallback sound_callback = nullptr);

    int rowCount(const QModelIndex & parent) const override;

    QVariant data(const QModelIndex & index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    // Callable by GUI to fetch cell.
    Q_INVOKABLE QVariantMap cell_at(int x, int y) const;

    Q_INVOKABLE QVariantMap get_tank_data(int occupant) const;

    UserListModel* players_model();

    void add_replay(MatchReplay replay);

    Q_INVOKABLE void set_replay(qint64 replay_id);

    Q_INVOKABLE void step_forward_turn();

    Q_INVOKABLE void step_backward_turn();

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

private:
    // Callable from C++ code for changing the view.
    void update_view();

signals:
    void view_changed();

    void map_width_changed();

    void map_height_changed();

    void state_changed();

    void fuel_changed();

    void player_changed();

    void match_downloaded(qint64 id);

private:
    // Replay management data.
    std::vector<MatchReplay> replays_;
    size_t total_replay_bytes_{0};

    MatchReplay current_replay_;
    size_t current_replay_id_;

    // Match specific data.
    GameInstance current_instance_;

    // Current selected perspective.
    uint8_t current_perspective_{NO_PLAYER};
    PlayerView current_view_;

    // TODO: fetch data for match.
    StaticMatchData current_data_;
    UserListModel players_;

    PlaySoundCallback sound_callback_;
};
