#include "server.h"

Server::Server(asio::io_context & cntx,
               tcp::endpoint endpoint)
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
            },
         // Callback for match results
         [this](MatchResult result)
            {
                // Hand this off to the results recorder.
                db_.record_match(result);
            }),
user_manager_(cntx),
db_(cntx,
    // Database callback for authentication
    [this](UserData data, std::shared_ptr<Session> session)
    {
        this->on_auth(data, session);
    }
)
{
    // Accept connections in a loop.
    do_accept();
}

// TODO: IP ban handling
void Server::do_accept()
{
    acceptor_.async_accept(
        asio::bind_executor(server_strand_,
        [this](boost::system::error_code ec, tcp::socket socket)
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
    // Run removal of session on the server's strand.
    asio::post(server_strand_, [this, session]{

        // Remove this session by s_ID.
        sessions_.erase(session->id());

        // Cancel all queues, if any exist
        for (int mode = 0; mode < static_cast<int>(GameMode::NO_MODE); mode++)
        {
            matcher_.cancel(session, static_cast<GameMode>(mode), false);
        }

        // Call for the user manager to handle removal.
        //
        // If there is no game for the user, we drop the user struct from memory.
        user_manager_.disconnect(session);

    });
}

// TODO: Error handling
// TODO: message validation
// TODO: any filtering of excessive packets
//
// This function is used in session's strand. As such, it must not
// modify any members outside of the session instance. All actions
// should only post to the strand of the object being utilized.
void Server::on_message(const ptr & session, Message msg)
{
    std::cout << "Header Type: " << +uint8_t(msg.header.type_) << "\n";
    std::cout << "Payload Size: " << +uint8_t(msg.header.payload_len) << "\n";

    // handle different types of messages
    switch(msg.header.type_)
    {
        case HeaderType::LoginRequest:
        {
            // Prevent login attempts when already logged in.
            if (session->is_authenticated())
            {
                Message not_authorized;
                // TODO: bad auth notification instead?
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);

                break;
            }

            db_.authenticate(msg, session);
            break;
        }
        case HeaderType::RegistrationRequest:
        {
            // Prevent registration attempts when already logged in.
            if (session->is_authenticated())
            {
                Message bad_reg;
                BadRegNotification r_notif(BadRegNotification::Reason::CurrentlyAuthenticated);
                bad_reg.create_serialized(r_notif);

                session->deliver(bad_reg);
                break;
            }

            db_.register_account(msg, session);
            break;
        }
        case HeaderType::QueueMatch:
        {

            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }

            if (!msg.valid_matching_command())
            {
                std::cout << "Invalid game mode detected\n";
                // TODO: bad matchmaking operation message
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

            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }

            if (!msg.valid_matching_command())
            {
                // TODO: bad matchmaking operation message
                std::cout << "Invalid game mode detected\n";
                break;
            }
            // Quickly grab the game mode so we can drop the message data.
            GameMode queued_mode = GameMode(msg.payload[0]);
            std::cout << "Trying to cancel queue for game mode " << +uint8_t(queued_mode) << "\n";
            matcher_.cancel(session, queued_mode, true);
            break;
        }
        case HeaderType::SendCommand:

            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }

            // Route to match deals with message validation.
            matcher_.route_to_match(session, msg);
            break;
        case HeaderType::Text:

            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }

            // not implemented for now
            break;
        case HeaderType::ForfeitMatch:

            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }

            //matcher_.
            break;
        default:
            // do nothing, should never happen
            break;
    }
}

void Server::on_auth(UserData data, std::shared_ptr<Session> session)
{
    asio::post(server_strand_, [this, user_data = std::move(data), s = std::move(session)]{

        // First, check if the uuid is nil, if so then auth failed
        // and we should tell the client to try again.
        if (user_data.user_id == boost::uuids::nil_uuid())
        {
            // Notify session with BadAuth
            Message bad_auth_msg;
            bad_auth_msg.create_serialized(HeaderType::BadAuth);
            s->deliver(bad_auth_msg);
            return;
        }

        // Otherwise, we authenticated correctly, lets add this user
        // to the user manager.
        user_manager_.on_login(std::move(user_data), s);

        // Notify session of good auth
        Message good_auth_msg;
        good_auth_msg.create_serialized(HeaderType::GoodAuth);
        s->deliver(good_auth_msg);

    });
}

