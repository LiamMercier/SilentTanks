#include "server.h"
#include "console.h"

Server::Server(asio::io_context & cntx,
               tcp::endpoint endpoint)
:server_strand_(cntx.get_executor()),
acceptor_(cntx, endpoint),
user_manager_(std::make_shared<UserManager>(cntx)),
matcher_(cntx,
         std::string("mapfile.txt"),
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
            },
         // Pass a reference to the user manager.
         user_manager_
         ),
db_(cntx,
    // Database callback for authentication
    [this](UserData data,
           UserManager::UUIDHashSet friends,
           UserManager::UUIDHashSet blocked_users,
           std::shared_ptr<Session> session)
    {
        this->on_auth(data,
                      friends,
                      blocked_users,
                      session);
    },
    [this](boost::uuids::uuid user_id,
           std::chrono::system_clock::time_point banned_until,
           std::string reason)
    {
        this->on_ban_user(user_id, banned_until, reason);
    },
    // Pass a reference to the user manager.
    user_manager_
)
{
    // Load bans into the server's map on startup.
    //
    // This will block the server thread until they are loaded.
    bans_ = db_.load_bans();

    // Accept connections in a loop.
    do_accept();
}

void Server::CONSOLE_ban_user(std::string username,
                      std::chrono::system_clock::time_point banned_until,
                      std::string reason)
{
    // Call the database to ban the user by username.
    //
    // The database must callback to the server with the UUID
    // after it is done to evict the user from the user manager.
    db_.ban_user(std::move(username),
                 std::move(banned_until),
                 std::move(reason));
}

void Server::CONSOLE_ban_ip(std::string ip,
                            std::chrono::system_clock::time_point banned_until)
{
    db_.ban_ip(std::move(ip),
               std::move(banned_until));
}

void Server::do_accept()
{
    acceptor_.async_accept(
        asio::bind_executor(server_strand_,
        [this](boost::system::error_code ec, tcp::socket sock){
            this->handle_accept(ec, std::move(sock));
        }));
}

void Server::handle_accept(const boost::system::error_code & ec,
                           tcp::socket socket)
{
    if (!ec)
    {
        // Handle IP bans.
        auto client_ip = socket.remote_endpoint().address().to_string();
        auto ip_itr = bans_.find(client_ip);
        auto now = std::chrono::system_clock::now();

        //db_.ban_ip(client_ip, now + std::chrono::minutes(10));

        // Drop connections of banned players.
        if(ip_itr != bans_.end() && now < ip_itr->second)
        {
            // Send ban message.
            auto ban_expiration = ip_itr->second;
            send_banned(ban_expiration, std::move(socket));

            do_accept();
            return;
        }
        // If a ban exists but expired, clean it up
        else if (ip_itr != bans_.end())
        {
            bans_.erase(ip_itr);
            // Tell the database to drop this ban entry.
            db_.unban_ip(client_ip);
        }

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
        user_manager_->disconnect(session);

    });
}

// TODO: Error handling
// TODO: any filtering of excessive packets
//
// This function is used in session's strand. As such, it must not
// modify any members outside of the session instance. All actions
// should only post to the strand of the object being utilized.
void Server::on_message(const ptr & session, Message msg)
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

    std::cout << "[Server]: Header Type: " << +uint8_t(msg.header.type_) << "\n";
    std::cout << "[Server]: Payload Size: " << +uint8_t(msg.header.payload_len) << "\n";

    // handle different types of messages
    switch(msg.header.type_)
    {
        case HeaderType::LoginRequest:
        {
            // Prevent login attempts when already logged in.
            if (session->is_authenticated())
            {
                Message already_authorized;
                BadAuthNotification b_auth(BadAuthNotification::Reason::CurrentlyAuthenticated);
                already_authorized.create_serialized(b_auth);
                session->deliver(already_authorized);

                break;
            }

            std::string client_ip;

            // Grab the current client IP.
            try
            {
                client_ip = session->socket().remote_endpoint().address().to_string();
            }
            catch (const std::exception & e)
            {
                // Set to null IP on error.
                client_ip = "0.0.0.0";
            }

            // Check if the user is banned.
            //
            // Necessary if this person's user was banned
            // before they authenticated and thus their IP
            // was not filtered by the accept loop.

            // First, grab the server's ban map mutex.
            std::lock_guard<std::mutex> lock(bans_mutex_);

            auto ip_itr = bans_.find(client_ip);
            auto now = std::chrono::system_clock::now();

            if(ip_itr != bans_.end() && now < ip_itr->second)
            {
                // Send a banned message and close the session.
                auto ban_expiration = ip_itr->second;

                BanMessage banned;
                banned.time_till_unban = ban_expiration;

                Message banned_msg;
                banned_msg.create_serialized(banned);

                session->deliver(banned_msg);
                session->close_session();

                return;
            }

            db_.authenticate(msg, session, client_ip);
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

            std::string client_ip;

            // Grab the current client IP.
            try
            {
                client_ip = session->socket().remote_endpoint().address().to_string();
            }
            catch (const std::exception & e)
            {
                // Set to null IP on error.
                client_ip = "0.0.0.0";
            }

            // Check if the user is banned.
            //
            // Necessary if this person's user was banned
            // before they authenticated and thus their IP
            // was not filtered by the accept loop.

            // First, grab the server's ban map mutex.
            std::lock_guard<std::mutex> lock(bans_mutex_);

            auto ip_itr = bans_.find(client_ip);
            auto now = std::chrono::system_clock::now();

            if(ip_itr != bans_.end() && now < ip_itr->second)
            {
                // Send a banned message and close the session.
                auto ban_expiration = ip_itr->second;

                BanMessage banned;
                banned.time_till_unban = ban_expiration;

                Message banned_msg;
                banned_msg.create_serialized(banned);

                session->deliver(banned_msg);
                session->close_session();
                return;
            }

            db_.register_account(msg, session, client_ip);
            break;
        }
        case HeaderType::FetchFriends:
        {
            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }
            boost::uuids::uuid user_id = (session->get_user_data()).user_id;
            db_.fetch_friends(user_id, session);
            break;
        }
        case HeaderType::FetchFriendRequests:
        {
            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }
            boost::uuids::uuid user_id = (session->get_user_data()).user_id;
            db_.fetch_friend_requests(user_id, session);
            break;
        }
        case HeaderType::FetchBlocks:
        {
            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }
            boost::uuids::uuid user_id = (session->get_user_data()).user_id;
            db_.fetch_blocks(user_id, session);
            break;
        }
        case HeaderType::SendFriendRequest:
        {
            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }
            boost::uuids::uuid user_id = (session->get_user_data()).user_id;
            db_.send_friend_request(user_id, msg, session);
            break;
        }
        case HeaderType::RespondFriendRequest:
        {
            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }
            boost::uuids::uuid user_id = (session->get_user_data()).user_id;
            db_.respond_friend_request(user_id, msg, session);
            break;
        }
        case HeaderType::RemoveFriend:
        {
            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }
            boost::uuids::uuid user_id = (session->get_user_data()).user_id;
            db_.remove_friend(user_id, msg, session);
            break;
        }
        case HeaderType::BlockUser:
        {
            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }
            boost::uuids::uuid user_id = (session->get_user_data()).user_id;
            db_.block_user(user_id, msg, session);
            break;
        }
        case HeaderType::UnblockUser:
        {
            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }
            boost::uuids::uuid user_id = (session->get_user_data()).user_id;
            db_.unblock_user(user_id, msg, session);
            break;
        }
        case HeaderType::DirectTextMessage:
        {
            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }

            // Convert to text and forward.
            TextMessage dm = msg.to_text_message();

            user_manager_->direct_message_user
                            (
                                session->get_user_data().user_id,
                                dm
                            );
            break;
        }
        case HeaderType::MatchTextMessage:
        {
            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }

            // Convert to match text and forward.
            TextMessage raw_msg = msg.to_text_message();

            UserData data = session->get_user_data();

            // Other fields are filled later.
            InternalMatchMessage msg{};
            msg.sender_username = data.username;
            msg.text = std::move(raw_msg.text);

            matcher_.send_match_message
                            (
                                session,
                                data.user_id,
                                msg
                            );
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
                Message bad_queue;
                bad_queue.create_serialized(HeaderType::BadQueue);
                session->deliver(bad_queue);
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
                Message bad_queue;
                bad_queue.create_serialized(HeaderType::BadQueue);
                session->deliver(bad_queue);
                break;
            }
            // Quickly grab the game mode so we can drop the message data.
            GameMode queued_mode = GameMode(msg.payload[0]);
            std::cout << "Trying to cancel queue for game mode " << +uint8_t(queued_mode) << "\n";
            matcher_.cancel(session, queued_mode, true);
            break;
        }
        case HeaderType::SendCommand:
        {
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
        }
        case HeaderType::ForfeitMatch:
        {
            // Prevent actions before login.
            if (!session->is_authenticated())
            {
                Message not_authorized;
                not_authorized.create_serialized(HeaderType::Unauthorized);
                session->deliver(not_authorized);
                break;
            }

            matcher_.forfeit(session);
            break;
        }
        default:
            // do nothing, should never happen
            break;
    }
}
// Close bad sessions.
catch (std::exception & e)
{
    std::string lmsg = "Exception in session's on_message handler: "
                        + std::string(e.what());
    Console::instance().log(lmsg,
                            LogLevel::ERROR);
    Message b_req;
    b_req.create_serialized(HeaderType::BadMessage);
    session->deliver(b_req);

    session->close_session();
}
}

void Server::on_auth(UserData data,
                     UserManager::UUIDHashSet friends,
                     UserManager::UUIDHashSet blocked_users,
                     std::shared_ptr<Session> session)
{
    asio::post(server_strand_,
               [this,
               user_data = std::move(data),
               friends = std::move(friends),
               blocks = std::move(blocked_users),
               s = std::move(session)]{

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
        user_manager_->on_login(std::move(user_data),
                                std::move(friends),
                                std::move(blocks),
                                s);

        // Notify session of good auth
        Message good_auth_msg;
        good_auth_msg.create_serialized(HeaderType::GoodAuth);
        s->deliver(good_auth_msg);

    });
}

void Server::on_ban_user(boost::uuids::uuid user_id,
                     std::chrono::system_clock::time_point banned_until,
                     std::string reason)
{
    asio::post(server_strand_,
               [this,
               user_id = std::move(user_id),
               banned_until = std::move(banned_until),
               reason = std::move(reason)]{

        user_manager_->on_ban_user(user_id, banned_until, reason);
    });
}

void Server::send_banned(std::chrono::system_clock::time_point banned_until,
                         tcp::socket socket)
{
    // Send a banned message and close the socket.
    BanMessage banned;
    banned.time_till_unban = banned_until;

    auto banned_msg_ptr = std::make_shared<Message>();
    banned_msg_ptr->create_serialized(banned);

    // Buffer over header and payload.
    const std::array<asio::const_buffer, 2> & bufs
    {
        {
            asio::buffer(&banned_msg_ptr->header, sizeof(Header)),
            asio::buffer(banned_msg_ptr->payload)
        }
    };

    // Keep data alive until socket is fully closed.
    auto socket_ptr = std::make_shared<tcp::socket>(std::move(socket));

    // Close the session after sending.
    asio::async_write(*socket_ptr, bufs,
            [socket_ptr, banned_msg_ptr]
            (boost::system::error_code ec, std::size_t){
                boost::system::error_code ignored_ec;

                socket_ptr->shutdown(tcp::socket::shutdown_both, ignored_ec);

                socket_ptr->close(ignored_ec);
    });
}

