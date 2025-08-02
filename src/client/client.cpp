#include "client.h"

Client::Client(asio::io_context & cntx)
:io_context_(cntx),
client_strand_(cntx.get_executor()),
state_(ClientState::ConnectScreen)
{

}

void Client::connect(std::string endpoint)
{
    asio::post(client_strand_,
        [this, endpoint = std::move(endpoint)]{

        auto host = "127.0.0.1";
        auto port = "12345";

        this->current_session_ = std::make_shared<ClientSession>
                                    (
                                        io_context_
                                    );

        this->current_session_->set_message_handler
        (
            [this](const ptr & s, Message m){ on_message(s, m); },
            [this](){ on_disconnect(); }
        );

        this->current_session_->start(host, port);

    });
}

// This function is used in session's strand. As such, it must not
// modify any members outside of the session instance. All actions
// should only post to the strand of the object being utilized.
void Client::on_message(const ptr & session, Message msg)
{
    // Prevent malformed or incorrect data.
    if (msg.header.payload_len != msg.payload.size())
    {
        Message b_req;
        b_req.create_serialized(HeaderType::BadMessage);
        session->deliver(b_req);

        session->close_session();
        return;
    }

try {

    // handle different types of messages
    switch(msg.header.type_)
    {
        case HeaderType::Unauthorized:
        {
            std::cerr << "Unauthorized action, login first?\n";
            break;
        }
        default:
            // do nothing, should never happen
            break;
    }
}
catch (std::exception & e)
{
    // TODO: log this at some point.

    session->close_session();
}
}

void Client::on_disconnect()
{
    asio::post(client_strand_,
        [this]{

        state_ = ClientState::Disconnected;

        // TODO: figure out how disconnects should work later.
    });
}
