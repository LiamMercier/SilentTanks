#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <unordered_set>

#include "user-list-model.h"
#include "player-view.h"
#include "message-structs.h"
#include "game-instance.h"
#include "client-state.h"
#include "popup.h"
#include "match-result-structs.h"
#include "move-status.h"

class ReplayManager : public QAbstractListModel
{
    using PopupCallback = std::function<void(Popup p, bool urgent)>;

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

    Q_PROPERTY(int replay_id
               READ replay_id
               NOTIFY replay_id_changed)

    Q_PROPERTY(int player_count
               READ player_count
               NOTIFY player_count_changed)

    Q_PROPERTY(int perspective
               READ perspective
               NOTIFY perspective_changed)
public:
    // Upper bound on how many bytes the client is willing to store
    // as replays. This should generally never be hit under normal operation
    // because the replay structures are often small.
    static constexpr size_t MAX_REPLAY_BYTES = 16 * 1024 * 1024;

    enum Roles {
        MatchIDRole = Qt::UserRole + 1
    };

    ReplayManager(QObject * parent = nullptr,
                  PlaySoundCallback sound_callback = nullptr,
                  PopupCallback popup_callback = nullptr);

    int rowCount(const QModelIndex & parent) const override;

    QVariant data(const QModelIndex & index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    // Callable by GUI to fetch cell.
    Q_INVOKABLE QVariantMap cell_at(int x, int y) const;

    Q_INVOKABLE QVariantMap get_tank_data(int occupant) const;

    UserListModel* players_model();

    void add_replay(MatchReplay replay);

    Q_INVOKABLE void set_replay(qint64 match_id);

    Q_INVOKABLE void set_perspective(qint64 player_id);

    Q_INVOKABLE void step_forward_turn();

    Q_INVOKABLE void step_backward_turn();

    Q_INVOKABLE bool match_exists(qint64 match_id);

    void update_match_data(StaticMatchData data, std::string username);

    int map_width() const;

    int map_height() const;

    int state() const;

    int fuel() const;

    int replay_id() const;

    int player_count() const;

    int perspective () const;

    QString player() const;

    uint16_t sequence_number();

    uint8_t tank_at(int x, int y);

    Q_INVOKABLE bool is_turn();

    Q_INVOKABLE bool is_friendly_tank(uint8_t tank_id);

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

    void replay_id_changed();

    void player_count_changed();

    void perspective_changed();

    void match_downloaded(qint64 id);

private:
    //
    // Replay management data.
    //
    size_t total_replay_bytes_{0};

    mutable std::mutex replay_mutex_;
    std::vector<MatchReplay> replays_;
    std::unordered_set<uint64_t> replay_exists_;
    uint64_t current_replay_index_{UINT64_MAX};
    uint64_t current_replay_id_{UINT64_MAX};

    //
    // Match specific data.
    //
    GameInstance current_instance_;

    // Current selected perspective.
    uint8_t current_perspective_{NO_PLAYER};

    // Store the number of moves applied.
    uint64_t applied_moves_{0};

    // Store when moves have no effect (moving into walls, etc).
    std::vector<MoveStatus> move_status_;

    PlayerView current_view_;

    // TODO: fetch data for match.
    UserListModel players_;
    UserList player_list_;

    //
    // Callbacks
    //
    PlaySoundCallback sound_callback_;
    PopupCallback popup_callback_;
};
