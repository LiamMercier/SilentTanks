#include "gui-client.h"

GUIClient::GUIClient(asio::io_context & cntx, QObject* parent)
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

