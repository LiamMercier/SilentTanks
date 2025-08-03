#include "gui-client.h"

GUIClient::GUIClient(asio::io_context & cntx, QObject* parent = nullptr)
:QObject(parent),
client_
(
    cntx,
    // construct client callback for state changes.
    [this](ClientState new_state)
    {
        QMetaObject::invokeMethod(this, [this, new_state]{
            emit state_changed(new_state);
        },
        Qt::QueuedConnection);
    }
)
{

}

ClientState state() const
{
    return static_cast<ClientState>(client_.get_state());
}

Q_INVOKABLE void GUIClient::connect_to_server(const QString & endpoint)
{
    client_.connect(endpoint.toStdString());
}
