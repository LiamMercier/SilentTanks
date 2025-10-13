#include "gui-client.h"

#include <boost/uuid/uuid_generators.hpp>

GUIClient::GUIClient(asio::io_context & cntx, QObject* parent)
:QObject(parent),
client_
(
    cntx,
    // construct callback for when client logs in.
    [this](std::string username)
    {
        QMetaObject::invokeMethod(this, [this, username]{
            this->username_ = QString::fromStdString(username);
            emit username_changed(this->username_);
        },
        Qt::QueuedConnection);
    },
    // construct client callback for state changes.
    [this](ClientState new_state)
    {
        QMetaObject::invokeMethod(this, [this, new_state]{
            emit state_changed(new_state);
        },
        Qt::QueuedConnection);
    },
    [this](Popup p,
           bool urgent)
    {
        {
            std::lock_guard lock(popups_mutex_);
            if (urgent)
            {
                urgent_popup_queue_.push(p);
            }
            else
            {
                standard_popup_queue_.push(p);
            }
        }
        // Try to send popup to Qt.
        try_show_popup();
    },
    [this](const UserMap & users, UserListType type){
        QMetaObject::invokeMethod(this, [this, users, type](){
            switch (type)
            {
                case UserListType::Friends:
                {
                    friends_.set_users(users);
                    break;
                }
                case UserListType::FriendRequests:
                {
                    friend_requests_.set_users(users);
                    break;
                }
                case UserListType::Blocks:
                {
                    blocked_.set_users(users);
                    break;
                }
                default:
                {
                    return;
                }
            }
        }, Qt::QueuedConnection);
    },
    [this](GameMode mode){
        QMetaObject::invokeMethod(this, [this, mode]{
            {
                std::lock_guard lock(this->queued_mutex_);
                this->queued_mode_ = static_cast<QueueType>(mode);
            }
            emit queue_updated(this->queued_mode_);
        },
        Qt::QueuedConnection);
    },
    [this](std::string message){
        QMetaObject::invokeMethod(this, [this, message]{
            this->messages_.add_message(message);
        },
        Qt::QueuedConnection);
    },
    [this](MatchResultList results){
        QMetaObject::invokeMethod(this, [this, results]{
            {
                this->match_history_.set_history_row(results);
            }
        },
        Qt::QueuedConnection);
    },
    [this](PlayerView new_view){
        QMetaObject::invokeMethod(this, [this, view = std::move(new_view)]{
            {
                this->game_manager_.update_view(view);
            }
        },
        Qt::QueuedConnection);
    },
    [this](StaticMatchData data){
        QMetaObject::invokeMethod(this, [this, data = std::move(data)]{
            {
                this->game_manager_.update_match_data(data, username_.toStdString());
            }
        },
        Qt::QueuedConnection);
    },
    [this](MatchReplay replay){
        QMetaObject::invokeMethod(this, [this, replay = std::move(replay)]{
            {
                this->replay_manager_.add_replay(replay);
            }
        },
        Qt::QueuedConnection);
    }
),
friends_(this),
friend_requests_(this),
blocked_(this),
game_manager_(nullptr,
    [this](SoundType sound){
        emit play_sound(sound);
    }
),
replay_manager_(nullptr,
    [this](SoundType sound){
        emit play_sound(sound);
    },
    [this](Popup p,
           bool urgent)
    {
        {
            std::lock_guard lock(popups_mutex_);
            if (urgent)
            {
                urgent_popup_queue_.push(p);
            }
            else
            {
                standard_popup_queue_.push(p);
            }
        }
        // Try to send popup to Qt.
        try_show_popup();
    }
)
{

}

ClientState GUIClient::state() const
{
    return static_cast<ClientState>(client_.get_state());
}

QString GUIClient::username() const
{
    return username_;
}

QueueType GUIClient::queued_mode() const
{
    return queued_mode_;
}

QueueType GUIClient::selected_mode() const
{
    std::lock_guard lock(selection_mutex_);
    return selected_mode_;
}

Q_INVOKABLE void GUIClient::set_selected_mode(QueueType mode)
{
    QMetaObject::invokeMethod(this, [this, mode]{
            {
                std::lock_guard lock(this->selection_mutex_);

                if (this->selected_mode_ == mode)
                {
                    return;
                }

                this->selected_mode_ = mode;
            }
            emit selected_mode_changed(this->selected_mode_);
        },
        Qt::QueuedConnection);
}

UserListModel* GUIClient::friends_model()
{
    return & friends_;
}

UserListModel* GUIClient::requests_model()
{
    return & friend_requests_;
}

UserListModel* GUIClient::blocked_model()
{
    return & blocked_;
}

ChatMessageModel* GUIClient::messages_model()
{
    return & messages_;
}

MatchHistoryModel* GUIClient::history_model()
{
    return & match_history_;
}

GameManager* GUIClient::game_manager()
{
    return & game_manager_;
}

ReplayManager* GUIClient::replay_manager()
{
    return & replay_manager_;
}

UserListModel* GUIClient::players_model()
{
    return game_manager_.players_model();
}

UserListModel* GUIClient::replay_players()
{
    return replay_manager_.replay_players();
}

Q_INVOKABLE void GUIClient::notify_popup_closed()
{
    {
        std::lock_guard lock(popups_mutex_);
        showing_popup_ = false;
    }
    try_show_popup();
}

Q_INVOKABLE void GUIClient::connect_to_server(const QString & endpoint)
{
    client_.connect(endpoint.toStdString());
}

Q_INVOKABLE void GUIClient::login(const QString & username,
                                  const QString & password)
{
    client_.login(username.toStdString(), password.toStdString());
}

Q_INVOKABLE void GUIClient::register_account(const QString & username,
                                             const QString & password)
{
    client_.register_account(username.toStdString(), password.toStdString());
}

Q_INVOKABLE void GUIClient::toggle_queue()
{
    std::lock_guard lock(queued_mutex_);

    std::cout << "queued_mode_: " << +static_cast<uint8_t>(queued_mode_) << "\n";

    // If not in queue, treat as queue request.
    if (queued_mode_ == QueueType::NO_MODE)
    {
        std::lock_guard lock(selection_mutex_);
        client_.queue_request(static_cast<GameMode>(selected_mode_));
    }
    // Otherwise, treat as cancel request.
    else
    {
        client_.cancel_request(static_cast<GameMode>(queued_mode_));
    }
}

Q_INVOKABLE void GUIClient::friend_user(const QString & username)
{
    client_.send_friend_request(username.toStdString());
}

Q_INVOKABLE void GUIClient::block_user(const QString & username)
{
    client_.send_block_request(username.toStdString());
}

Q_INVOKABLE void GUIClient::respond_friend_request(const QString & uuid, bool decision)
{
    boost::uuids::string_generator gen;
    boost::uuids::uuid user_id = gen(uuid.toStdString());
    client_.respond_friend_request(user_id, decision);
}

Q_INVOKABLE void GUIClient::unblock_user(const QString & uuid)
{
    boost::uuids::string_generator gen;
    boost::uuids::uuid user_id = gen(uuid.toStdString());
    client_.send_unblock_request(user_id);
}

Q_INVOKABLE void GUIClient::unfriend_user(const QString & uuid)
{
    boost::uuids::string_generator gen;
    boost::uuids::uuid user_id = gen(uuid.toStdString());
    client_.send_unfriend_request(user_id);
}

Q_INVOKABLE void GUIClient::write_message(const QString & msg)
{
    client_.interpret_message(msg.toStdString());
}

Q_INVOKABLE QVariant GUIClient::get_elo(QueueType mode)
{
    int elo = client_.get_elo(static_cast<GameMode>(mode));
    if (elo <= 0)
    {
        return QVariant();
    }
    else
    {
        return QVariant(elo);
    }
}

Q_INVOKABLE void GUIClient::fetch_match_history(QueueType mode)
{
    client_.fetch_match_history(static_cast<GameMode>(mode));
}

Q_INVOKABLE void GUIClient::download_match_by_id(qint64 match_id)
{
    client_.request_match_replay(static_cast<uint64_t>(match_id));
}

Q_INVOKABLE void GUIClient::start_replay(qint64 match_id)
{
    replay_manager_.set_replay(match_id);
    client_.change_state(ClientState::Replaying);
}

Q_INVOKABLE void GUIClient::close_replay()
{
    client_.change_state(ClientState::Lobby);
}

Q_INVOKABLE void GUIClient::send_forfeit()
{
    client_.forfeit_request();
}

Q_INVOKABLE void GUIClient::send_place_tank(int x,
                                            int y,
                                            int placement_direction)
{
    if (!game_manager_.is_turn())
    {
        return;
    }

    Command cmd;

    // Fetch from game manager.
    cmd.type = CommandType::Place;
    cmd.sequence_number = game_manager_.sequence_number();
    cmd.payload_first = x;
    cmd.payload_second = y;

    // tank_id holds placement direction for placement command.
    cmd.tank_id = placement_direction;

    // Fill empty field with 0.
    cmd.sender = 0;


    client_.send_command(std::move(cmd));
}

Q_INVOKABLE void GUIClient::send_rotate_barrel(int x, int y, int rotation)
{
    if (!game_manager_.is_turn())
    {
        return;
    }

    Command cmd;

    cmd.type = CommandType::RotateBarrel;
    cmd.sequence_number = game_manager_.sequence_number();
    cmd.tank_id = game_manager_.tank_at(x, y);
    cmd.payload_first = rotation;

    // Fill empty fields with 0.
    cmd.payload_second = 0;
    cmd.sender = 0;

    client_.send_command(std::move(cmd));

    emit play_sound(SoundType::Rotate);
}

Q_INVOKABLE void GUIClient::send_move_tank(int x, int y, int dir)
{
    if (!game_manager_.is_turn())
    {
        return;
    }

    Command cmd;

    cmd.type = CommandType::Move;
    cmd.sequence_number = game_manager_.sequence_number();
    cmd.tank_id = game_manager_.tank_at(x, y);
    cmd.payload_first = dir;

    // Fill empty fields with 0.
    cmd.payload_second = 0;
    cmd.sender = 0;

    client_.send_command(std::move(cmd));

    emit play_sound(SoundType::Move);
}

Q_INVOKABLE void GUIClient::send_rotate_tank(int x, int y, int rotation)
{
    if (!game_manager_.is_turn())
    {
        return;
    }

    Command cmd;

    cmd.type = CommandType::RotateTank;
    cmd.sequence_number = game_manager_.sequence_number();
    cmd.tank_id = game_manager_.tank_at(x, y);
    cmd.payload_first = rotation;

    // Fill empty fields with 0.
    cmd.payload_second = 0;
    cmd.sender = 0;

    client_.send_command(std::move(cmd));

    emit play_sound(SoundType::Rotate);
}

Q_INVOKABLE void GUIClient::send_fire_tank(int x, int y)
{
    if (!game_manager_.is_turn())
    {
        return;
    }

    Command cmd;
    cmd.tank_id = game_manager_.tank_at(x, y);

    // See if there is ammo before sending.
    if (!game_manager_.tank_has_ammo(cmd.tank_id))
    {
        emit play_sound(SoundType::OutOfAmmo);
        return;
    }

    // If we have ammo, send firing command.
    cmd.type = CommandType::Fire;
    cmd.sequence_number = game_manager_.sequence_number();

    // Fill empty fields with 0.
    cmd.payload_first = 0;
    cmd.payload_second = 0;
    cmd.sender = 0;

    client_.send_command(std::move(cmd));

    emit play_sound(SoundType::CannonFiring);
}

Q_INVOKABLE void GUIClient::send_reload_tank(int x, int y)
{
    if (!game_manager_.is_turn())
    {
        return;
    }

    Command cmd;

    cmd.type = CommandType::Load;
    cmd.sequence_number = game_manager_.sequence_number();
    cmd.tank_id = game_manager_.tank_at(x, y);

    // Fill empty fields with 0.
    cmd.payload_first = 0;
    cmd.payload_second = 0;
    cmd.sender = 0;

    client_.send_command(std::move(cmd));

    emit play_sound(SoundType::Reload);
}

void GUIClient::try_show_popup()
{
    Popup p;

    {
        std::lock_guard lock(popups_mutex_);

        if(showing_popup_)
        {
            return;
        }

        // Pop a popup if possible.
        if (!urgent_popup_queue_.empty())
        {
            p = urgent_popup_queue_.front();
            urgent_popup_queue_.pop();
        }
        else if (!standard_popup_queue_.empty())
        {
            p = standard_popup_queue_.front();
            standard_popup_queue_.pop();
        }
        else
        {
            return;
        }

        showing_popup_ = true;
    }
    // Post to Qt thread.
    QMetaObject::invokeMethod(this, [this, p]{
            emit show_popup(p.type,
                            QString::fromStdString(p.title),
                            QString::fromStdString(p.body));
        },
        Qt::QueuedConnection);
}

