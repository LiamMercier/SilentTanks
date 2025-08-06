#pragma once

#include "client.h"

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

    explicit GUIClient(asio::io_context& ctx, QObject* parent = nullptr);

    ClientState state() const;

    Q_INVOKABLE void notify_popup_closed();

    Q_INVOKABLE void connect_to_server(const QString & endpoint);

    Q_INVOKABLE void login(const QString & username,
                           const QString & password);

    Q_INVOKABLE void register_account(const QString & username,
                                      const QString & password);

private:
    void try_show_popup();

signals:
    // Defined by compiler.
    void state_changed(ClientState new_state);

    void show_popup(PopupType type,
                    QString title,
                    QString body);

private:
    Client client_;

    std::mutex popups_mutex_;

    std::queue<Popup> urgent_popup_queue_;
    std::queue<Popup> standard_popup_queue_;
    bool showing_popup_{false};

};
