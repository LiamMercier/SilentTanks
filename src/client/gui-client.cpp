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
                this->game_manager_.update_match_data(data);
            }
        },
        Qt::QueuedConnection);
    }
),
friends_(this),
friend_requests_(this),
blocked_(this)
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

