#include "server.h"

Server::Server(asio::io_context & cntx, tcp::endpoint endpoint)
:server_strand_(cntx.get_executor()),
acceptor_(cntx, endpoint),
matcher_(cntx,
         // Callback function to send messages to sessions
         [this](uint64_t s_id, Message msg)
            {
            auto target_session = sessions_.find(s_id);
            if (target_session != sessions_.end())
            {
                target_session->second->deliver(std::move(msg));
            }
            // Otherwise, session no longer exists!
            })
{
    // Accept connections in a loop.
    do_accept();
}

void Server::do_accept()
{
    acceptor_.async_accept(
        asio::bind_executor(server_strand_,
        [this](asio::error_code ec, tcp::socket socket)
        {
            // start lambda
            //
            // this could use its own function with std::bind eventually for more complex
            // logic (for example, IP bans).
            if (!ec)
            {

                // Increment and store the next session ID.
                uint64_t s_id = next_session_id_++;

                // Construct new session for the client
                //
                // We need to cast to io_context which is fine because we
                // only accept io_context as the construction argument.
                auto session = std::make_shared<Session>(
                    static_cast<asio::io_context&>(
                        acceptor_.get_executor().context()),
                        s_id);

                // Move socket into session.
                session->socket() = std::move(socket);

                // Set callback to on_message.
                session->set_message_handler
                (
                    [this](const ptr & s, Message m){ on_message(s, m); },
                    [this](const ptr & s){ remove_session(s); }
                );

                // Add to map of sessions and start the session.
                sessions_.emplace(s_id, session);
                session->start();
            }

            // Recursively accept in our handler.
            do_accept();
        }));
}

void Server::remove_session(const ptr & session)
{
    // run removal of session on the server's strand
    asio::post(server_strand_, [this, session]{

        sessions_.erase(session->id());

    });
}

// TODO: Error handling
// TODO: message validation
// TODO: network to host bytes
// This function is used in session's strand.
void Server::on_message(const ptr & session, Message msg)
{
    std::cout << "Header Type: " << +uint8_t(msg.header.type_) << "\n";
    std::cout << "Payload Size: " << +uint8_t(msg.header.payload_len) << "\n";
    // handle different types of messages
    switch(msg.header.type_)
    {
        case HeaderType::Login:
            // not implemented for now
            break;
        case HeaderType::QueueMatch:
        {
            if (!msg.valid_matching_command())
            {
                std::cout << "Invalid game mode detected\n";
                break;
            }
            // Quickly grab the game mode so we can drop the message data.
            GameMode queued_mode = GameMode(msg.payload[0]);
            std::cout << "Trying to queue for game mode " << +uint8_t(queued_mode) << "\n";
            matcher_.enqueue(session, queued_mode);
            break;
        }
        case HeaderType::CancelMatch:
        {
            if (!msg.valid_matching_command())
            {
                std::cout << "Invalid game mode detected\n";
                break;
            }
            // Quickly grab the game mode so we can drop the message data.
            GameMode queued_mode = GameMode(msg.payload[0]);
            std::cout << "Trying to cancel queue for game mode " << +uint8_t(queued_mode) << "\n";
            matcher_.cancel(session, queued_mode);
            break;
        }
        case HeaderType::SendCommand:
            matcher_.route_to_match(session, msg);
            break;
        case HeaderType::Text:
            // not implemented for now
            break;
        case HeaderType::ForfeitMatch:
            //matcher_.
            break;
        default:
            // do nothing, should never happen
            break;
    }
}


