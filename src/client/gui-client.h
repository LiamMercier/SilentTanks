#pragma once

#include "client.h"

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

    Q_INVOKABLE void connect_to_server(const QString & endpoint);

signals:
    // Defined by compiler.
    void state_changed(ClientState new_state);

private:
    Client client_;

};
