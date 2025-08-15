#pragma once

#include "client.h"
#include "user-list-model.h"
#include "message-model.h"

#include <queue>

#include <QString>
#include <QObject>

class GUIClient : public QObject
{
    Q_OBJECT

public:
    Q_ENUM(ClientState)

    Q_PROPERTY(ClientState state
               READ state
               NOTIFY state_changed)

    Q_PROPERTY(QString username
                READ username
                NOTIFY username_changed)

    // Ensure these line up.
    static_assert(static_cast<uint8_t>(QueueType::NO_MODE)
                  == static_cast<uint8_t>(GameMode::NO_MODE));

    Q_PROPERTY(QueueType queued_mode
               READ queued_mode
               NOTIFY queue_updated)

    Q_PROPERTY(QueueType selected_mode
               READ selected_mode
               NOTIFY selected_mode_changed)

    Q_PROPERTY(UserListModel* friendsModel
               READ friends_model CONSTANT)

    Q_PROPERTY(UserListModel* requestsModel
               READ requests_model CONSTANT)

    Q_PROPERTY(UserListModel* blockedModel
               READ blocked_model CONSTANT)

    Q_PROPERTY(ChatMessageModel* messagesModel
               READ messages_model CONSTANT)

    explicit GUIClient(asio::io_context& ctx, QObject* parent = nullptr);

    ClientState state() const;

    QString username() const;

    QueueType queued_mode() const;

    QueueType selected_mode() const;

    UserListModel* friends_model();

    UserListModel* requests_model();

    UserListModel* blocked_model();

    ChatMessageModel* messages_model();

    Q_INVOKABLE void set_selected_mode(QueueType mode);

    Q_INVOKABLE void notify_popup_closed();

    Q_INVOKABLE void connect_to_server(const QString & endpoint);

    Q_INVOKABLE void login(const QString & username,
                           const QString & password);

    Q_INVOKABLE void register_account(const QString & username,
                                      const QString & password);

    Q_INVOKABLE void toggle_queue();

    Q_INVOKABLE void friend_user(const QString & username);

    Q_INVOKABLE void block_user(const QString & username);

    Q_INVOKABLE void respond_friend_request(const QString & uuid, bool decision);

    Q_INVOKABLE void unblock_user(const QString & uuid);

    Q_INVOKABLE void unfriend_user(const QString & uuid);

    Q_INVOKABLE void write_message(const QString & msg);
private:
    void try_show_popup();

signals:
    // Defined by compiler.
    void state_changed(ClientState new_state);

    void username_changed(QString username);

    void queue_updated(QueueType mode);

    void selected_mode_changed(QueueType mode);

    void show_popup(PopupType type,
                    QString title,
                    QString body);

private:
    Client client_;

    std::mutex popups_mutex_;

    std::queue<Popup> urgent_popup_queue_;
    std::queue<Popup> standard_popup_queue_;
    bool showing_popup_{false};

    UserListModel friends_;
    UserListModel friend_requests_;
    UserListModel blocked_;

    QString username_;

    // Handle concurrent mode changes.
    mutable std::mutex selection_mutex_;

    // Mode the user has selected.
    QueueType selected_mode_{QueueType::NO_MODE};

    mutable std::mutex queued_mutex_;

    // Server-client negotiated mode
    QueueType queued_mode_{QueueType::NO_MODE};

    // Message view for GUI.
    ChatMessageModel messages_;
};
