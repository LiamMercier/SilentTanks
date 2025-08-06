#include "client.h"

#include <argon2.h>
#include <boost/uuid/uuid_io.hpp>

Client::Client(asio::io_context & cntx,
               StateChangeCallback state_change_callback,
               PopupCallback popup_callback)
:io_context_(cntx),
client_strand_(cntx.get_executor()),
state_(ClientState::ConnectScreen),
game_manager_(cntx),
state_change_callback_(std::move(state_change_callback)),
popup_callback_(std::move(popup_callback))
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
            [this](){ on_connection(); },
            [this](){ on_disconnect(); }
        );

        this->current_session_->start(host, port);

    });
}

void Client::login(std::string username, std::string password)
{
    // Prevent trying to login early or late.
    {
        std::lock_guard lock(state_mutex_);
        if (state_ != ClientState::LoginScreen)
        {
            return;
        }
    }

    asio::post(client_strand_,
        [this,
        username = std::move(username),
        password = std::move(password)]{

        // Copy the username first, then allow it to be moved.
        {
            std::lock_guard lock(data_mutex_);
            client_data_.client_username = username;
        }

        LoginRequest req;
        req.username = std::move(username);

        // First, compute H(password, GLOBAL_PEPPER) to obfuscate
        // our plaintext password. Then, we send this to the server.
        std::array<uint8_t, SALT_LENGTH> salt;

        std::copy
        (
            reinterpret_cast<const uint8_t *>(GLOBAL_CLIENT_SALT.data()),
            reinterpret_cast<const uint8_t *>(GLOBAL_CLIENT_SALT.data()
                                              + salt.size()),
            salt.begin()
        );

        int result = argon2id_hash_raw(
                    ARGON2_TIME,
                    ARGON2_MEMORY,
                    ARGON2_PARALLEL,
                    reinterpret_cast<const uint8_t*>(password.data()),
                    password.size(),
                    salt.data(),
                    salt.size(),
                    req.hash.data(),
                    req.hash.size());

        if (result != ARGON2_OK)
        {
            std::cerr << "Argon2 error!\n";
            return;
        }

        std::cout << "Sending login req\n";

        // If everything went well, send this to the server.
        Message login_request;
        login_request.create_serialized(req);
        current_session_->deliver(login_request);

    });
}

void Client::register_account(std::string username, std::string password)
{
    // Prevent trying to register early or late.
    {
        std::lock_guard lock(state_mutex_);
        if (state_ != ClientState::LoginScreen)
        {
            return;
        }
    }

    asio::post(client_strand_,
        [this,
        username = std::move(username),
        password = std::move(password)]{

        if (username.length() < 1)
        {
            popup_callback_(Popup(
                            PopupType::Info,
                            "Bad registration.",
                            "Your username must not be empty."),
                            STANDARD_POPUP);
            return;
        }

        if (username.length() > MAX_USERNAME_LENGTH)
        {
            std::string body = "Usernames can only be "
                               + std::to_string(MAX_USERNAME_LENGTH)
                               + " characters long.";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Bad registration.",
                            body),
                            STANDARD_POPUP);
            return;
        }

        RegisterRequest req;
        req.username = std::move(username);

        // First, compute H(password, GLOBAL_PEPPER) to obfuscate
        // our plaintext password. Then, we send this to the server.
        std::array<uint8_t, SALT_LENGTH> salt;

        std::copy
        (
            reinterpret_cast<const uint8_t *>(GLOBAL_CLIENT_SALT.data()),
            reinterpret_cast<const uint8_t *>(GLOBAL_CLIENT_SALT.data()
                                              + salt.size()),
            salt.begin()
        );

        int result = argon2id_hash_raw(
                    ARGON2_TIME,
                    ARGON2_MEMORY,
                    ARGON2_PARALLEL,
                    reinterpret_cast<const uint8_t*>(password.data()),
                    password.size(),
                    salt.data(),
                    salt.size(),
                    req.hash.data(),
                    req.hash.size());

        if (result != ARGON2_OK)
        {
            std::cerr << "Argon2 error!\n";
            return;
        }

        std::cout << "Sending registration request\n";

        // If everything went well, send this to the server.
        Message login_request;
        login_request.create_serialized(req);
        current_session_->deliver(login_request);

    });
}

void Client::request_user_list(UserListType list_type)
{
    asio::post(client_strand_,
        [this,
        list_type]{

    switch (list_type)
    {
        case UserListType::Friends:
        {
            Message fetch_friends;
            fetch_friends.create_serialized(HeaderType::FetchFriends);
            current_session_->deliver(fetch_friends);
            break;
        }
        case UserListType::FriendRequests:
        {
            Message fetch_friend_requests;
            fetch_friend_requests.create_serialized
                                        (
                                            HeaderType::FetchFriendRequests
                                        );
            current_session_->deliver(fetch_friend_requests);
            break;
        }
        case UserListType::Blocks:
        {
            Message fetch_blocks;
            fetch_blocks.create_serialized(HeaderType::FetchBlocks);
            current_session_->deliver(fetch_blocks);
            break;
        }
        default:
        {
            std::cerr << "Invalid list type in request creation!\n";
            break;
        }
    }

    });
}

void Client::send_friend_request(std::string username)
{
    asio::post(client_strand_,
        [this,
        username = std::move(username)]{

        FriendRequest friend_data;
        friend_data.username = std::move(username);

        Message friend_request;
        friend_request.create_serialized(friend_data);

        current_session_->deliver(friend_request);

    });
}

void Client::respond_friend_request(boost::uuids::uuid user_id,
                                    bool decision)
{
    std::cout << (decision ? "Accepting " : "Rejecting ") << "friend request.\n";

    asio::post(client_strand_,
        [this,
        user_id = std::move(user_id),
        decision]{

        FriendDecision decision_data;
        decision_data.user_id = std::move(user_id);
        decision_data.decision = decision;

        Message friend_decision;
        friend_decision.create_serialized(decision_data);
        current_session_->deliver(friend_decision);

    });
}

void Client::send_unfriend_request(boost::uuids::uuid user_id)
{
    asio::post(client_strand_,
        [this,
        user_id = std::move(user_id)]{

        {
            std::lock_guard lock(data_mutex_);

            std::string username;
            auto user_ref = client_data_.friends.find(user_id);

            if (user_ref != client_data_.friends.end())
            {
                username = user_ref->second.username;
            }
            else
            {
                username = "UNKNOWN";
            }

            std::cout << "Unfriending user " << username << "\n";
        }

        UnfriendRequest unfriend_data;
        unfriend_data.user_id = std::move(user_id);

        Message unfriend_request;
        unfriend_request.create_serialized(unfriend_data);
        current_session_->deliver(unfriend_request);

    });
}

void Client::send_block_request(std::string username)
{
    std::cout << "Blocking user " << username << "\n";

    asio::post(client_strand_,
        [this,
        username = std::move(username)]{

        BlockRequest block_data;
        block_data.username = std::move(username);

        Message block_request;
        block_request.create_serialized(block_data);
        current_session_->deliver(block_request);

    });
}

void Client::send_unblock_request(boost::uuids::uuid user_id)
{
    asio::post(client_strand_,
        [this,
        user_id = std::move(user_id)]{

        {
            std::lock_guard lock(data_mutex_);

            std::string username;
            auto user_ref = client_data_.blocked_users.find(user_id);

            if (user_ref != client_data_.blocked_users.end())
            {
                username = user_ref->second.username;
            }
            else
            {
                username = "UNKNOWN";
            }

            std::cout << "Unblocking user " << username << "\n";
        }

        UnblockRequest unblock_data;
        unblock_data.user_id = std::move(user_id);

        Message unblock_request;
        unblock_request.create_serialized(unblock_data);
        current_session_->deliver(unblock_request);

    });
}

void Client::queue_request(GameMode mode)
{
    // Prevent queue when not in lobby.
    {
        std::lock_guard lock(state_mutex_);
        if (state_ != ClientState::Lobby)
        {
            return;
        }
    }

    asio::post(client_strand_,
        [this,
        mode]{

        std::cout << "Queued up for gamemode "
                  << +static_cast<uint8_t>(mode)
                  << "\n";

        QueueMatchRequest queue_data(mode);

        Message queue_request;
        queue_request.create_serialized(queue_data);
        current_session_->deliver(queue_request);

    });
}

void Client::cancel_request(GameMode mode)
{
    // Prevent cancel when not in lobby.
    {
        std::lock_guard lock(state_mutex_);
        if (state_ != ClientState::Lobby)
        {
            return;
        }
    }

    asio::post(client_strand_,
        [this,
        mode]{

        std::cout << "Cancelled queue for gamemode "
                  << +static_cast<uint8_t>(mode)
                  << "\n";

        CancelMatchRequest cancel_data(mode);

        Message cancel_request;
        cancel_request.create_serialized(cancel_data);
        current_session_->deliver(cancel_request);

    });
}

void Client::send_command(Command cmd)
{
    // Prevent commands when not playing.
    {
        std::lock_guard lock(state_mutex_);
        if (state_ != ClientState::Playing)
        {
            return;
        }
    }

    asio::post(client_strand_,
        [this,
        cmd]{

        Message command;
        command.create_serialized(cmd);
        current_session_->deliver(command);

    });
}

void Client::forfeit_request()
{
    // Prevent forfeit when not playing.
    {
        std::lock_guard lock(state_mutex_);
        if (state_ != ClientState::Playing)
        {
            return;
        }
    }

    asio::post(client_strand_,
        [this]{

        Message forfeit;
        forfeit.create_serialized(HeaderType::ForfeitMatch);
        current_session_->deliver(forfeit);

    });
}

void Client::send_direct_message(std::string text,
                                 boost::uuids::uuid receiver)
{
    asio::post(client_strand_,
        [this,
        text = std::move(text),
        receiver = std::move(receiver)]{

        TextMessage dm;
        dm.user_id = receiver;
        dm.text = std::move(text);

        Message direct_message;
        direct_message.create_serialized(dm);
        direct_message.header.type_ = HeaderType::DirectTextMessage;
        current_session_->deliver(direct_message);

    });
}

void Client::send_match_message(std::string text)
{
    asio::post(client_strand_,
        [this,
        text = std::move(text)]{

        TextMessage dm{};
        dm.text = std::move(text);

        Message direct_message;
        direct_message.create_serialized(dm);
        direct_message.header.type_ = HeaderType::MatchTextMessage;
        current_session_->deliver(direct_message);

    });
}

// This function is used in the session's strand. As such, all
// actions should be protected from multi threading side effects.
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
        case HeaderType::AlreadyAuthorized:
        {
            std::cerr << "Already authorized, no need to login again.\n";
            break;
        }
        case HeaderType::GoodRegistration:
        {
            std::cerr << "Account registered.\n";
            break;
        }
        case HeaderType::BadRegistration:
        {
            std::cerr << "Bad reg.\n";

            std::string body;

            using Reason = BadRegNotification::Reason;

            // We have exactly 1 byte here, or the header
            // would have been rejected.
            Reason reason = static_cast<Reason>(msg.payload[0]);

            switch (reason)
            {
                case Reason::NotUnique:
                {
                    body = "Username was not unique.";
                    break;
                }
                case Reason::InvalidUsername:
                {
                    body = "Username was invalid.";
                    break;
                }
                case Reason::CurrentlyAuthenticated:
                {
                    body = "Currently authenticated.";
                    break;
                }
                case Reason::ServerError:
                {
                    body = "Server error.";
                    break;
                }
                default:
                {
                    body = "Server provided unknown reason.";
                    break;
                }
            }

            popup_callback_(Popup(
                            PopupType::Info,
                            "Bad registration.",
                            body),
                            URGENT_POPUP);

            break;
        }
        case HeaderType::GoodAuth:
        {
            {
                std::lock_guard lock(data_mutex_);
                std::cerr << "Good auth. Logged in as: "
                          << client_data_.client_username << "\n";
                change_state(ClientState::Lobby);
            }
            break;
        }
        case HeaderType::BadAuth:
        {
            std::cerr << "Bad auth.\n";

            std::string body;

            using Reason = BadAuthNotification::Reason;

            // We have exactly 1 byte here, or the header
            // would have been rejected.
            Reason reason = static_cast<Reason>(msg.payload[0]);

            switch (reason)
            {
                case Reason::BadHash:
                {
                    body = "Password hash does not match.";
                    break;
                }
                case Reason::InvalidUsername:
                {
                    body = "Username was invalid.";
                    break;
                }
                case Reason::CurrentlyAuthenticated:
                {
                    body = "Currently authenticated.";
                    break;
                }
                case Reason::ServerError:
                {
                    body = "Server error.";
                    break;
                }
                default:
                {
                    body = "Server provided unknown reason.";
                    break;
                }
            }

            popup_callback_(Popup(
                            PopupType::Info,
                            "Bad login.",
                            body),
                            URGENT_POPUP);

            break;
        }
        case HeaderType::FriendList:
        {
            std::cerr << "Friends retrieved: \n";

            // Convert the server's data into a list of users
            // for our friends list.
            bool status;
            UserList friends = msg.to_user_list(status);

            // If data was malformed.
            if (status == false)
            {
                std::cerr << "Malformed data in friends list! Closing!\n";
                current_session_->close_session();
                break;
            }

            for (const auto & user : friends.users)
            {
                std::cout << user.username
                          << " ("
                          << boost::uuids::to_string(user.user_id)
                          << ")"
                          << "\n";
            }

            std::cout << "\n";

            // Otherwise, add friends to our internal map.
            {
                std::lock_guard lock(data_mutex_);
                client_data_.load_user_list(friends,
                                            UserListType::Friends);
            }

            break;
        }
        case HeaderType::FriendRequestList:
        {
            std::cerr << "Friend requests retrieved: \n";

            // Convert the server's data into a list of users
            // for our friend requests list.
            bool status;
            UserList friend_requests = msg.to_user_list(status);

            // If data was malformed.
            if (status == false)
            {
                std::cerr << "Malformed data in friend request list! Closing!\n";
                current_session_->close_session();
                break;
            }

            for (const auto & user : friend_requests.users)
            {
                std::cout << user.username
                          << " ("
                          << boost::uuids::to_string(user.user_id)
                          << ")"
                          << "\n";
            }

            std::cout << "\n";

            // Otherwise, add friend requests to our internal map.
            {
                std::lock_guard lock(data_mutex_);
                client_data_.load_user_list(friend_requests,
                                            UserListType::FriendRequests);
            }

            break;
        }
        case HeaderType::BlockList:
        {
            std::cerr << "Blocked users retrieved: \n";

            // Convert the server's data into a list of users
            // for our block list.
            bool status;
            UserList block_list = msg.to_user_list(status);

            // If data was malformed.
            if (status == false)
            {
                std::cerr << "Malformed data in friend request list! Closing!\n";
                current_session_->close_session();
                break;
            }

            for (const auto & user : block_list.users)
            {
                std::cout << user.username
                          << " ("
                          << boost::uuids::to_string(user.user_id)
                          << ")"
                          << "\n";
            }

            std::cout << "\n";

            // Otherwise, add blocked users to our internal map.
            {
                std::lock_guard lock(data_mutex_);
                client_data_.load_user_list(block_list,
                                            UserListType::Blocks);
            }

            break;
        }
        case HeaderType::NotifyFriendAdded:
        {
            ExternalUser user = msg.to_user();

            {
                std::lock_guard lock(data_mutex_);
                client_data_.friends.emplace(user.user_id, user);
            }
            break;
        }
        case HeaderType::NotifyFriendRemoved:
        {
            ExternalUser user = msg.to_user();

            {
                std::lock_guard lock(data_mutex_);
                client_data_.friends.erase(user.user_id);
            }
            break;
        }
        case HeaderType::NotifyFriendRequest:
        {
            ExternalUser user = msg.to_user();

            std::cout << "Got friend request from "
                      << user.username
                      << " ("
                      << user.user_id
                      << ")\n";

            {
                std::lock_guard lock(data_mutex_);
                client_data_.friend_requests.emplace(user.user_id, user);
            }
            break;
        }
        case HeaderType::NotifyBlocked:
        {
            ExternalUser user = msg.to_user();

            {
                std::lock_guard lock(data_mutex_);
                client_data_.blocked_users.emplace(user.user_id, user);
            }
            break;
        }
        case HeaderType::NotifyUnblocked:
        {
            ExternalUser user = msg.to_user();

            {
                std::lock_guard lock(data_mutex_);
                client_data_.blocked_users.erase(user.user_id);
            }
            break;
        }
        case HeaderType::BadQueue:
        {
            std::cerr << "Bad queue.\n";
            break;
        }
        case HeaderType::BadCancel:
        {
            std::cerr << "Bad cancel.\n";
            break;
        }
        case HeaderType::QueueDropped:
        {
            std::cerr << "Queue was dropped.\n";
            break;
        }
        case HeaderType::MatchStarting:
        {
            std::cerr << "Match is starting.\n";

            {
                change_state(ClientState::Playing);
            }

            break;
        }
        case HeaderType::MatchCreationError:
        {
            std::cerr << "Error in match creation.\n";
            break;
        }
        case HeaderType::NoMatchFound:
        {
            std::cerr << "Cannot send command, match not found.\n";
            break;
        }
        case HeaderType::MatchInProgress:
        {
            std::cerr << "You have a match in progress.\n";

            {
                change_state(ClientState::Playing);
            }

            break;
        }
        case HeaderType::PlayerView:
        {
            std::cerr << "Player view update available.\n";

            bool status;
            PlayerView current_view = msg.to_player_view(status);

            if (status == false)
            {
                std::cerr << "Failed to convert player view.\n";
                break;
            }


            // TEMPORARY PRINT FOR VIEW
            // TODO: remove this.

        const static std::string colors_array[4] = {"\033[48;5;196m", "\033[48;5;21m", "\033[48;5;46m", "\033[48;5;226m"};

        for (int y = 0; y < current_view.map_view.get_height(); y++)
        {
            for (int x = 0; x < current_view.map_view.get_width(); x++)
            {
                GridCell curr = current_view.map_view[current_view.map_view.idx(x,y)];

                if (curr.visible_ == true)
                {
                    uint8_t occ = curr.occupant_;

                    if (occ == NO_OCCUPANT)
                    {
                        std::cout << "\033[48;5;184m" << "_" << "\033[0m ";
                    }
                    else
                    {
                        auto this_tank = std::find_if(
                            current_view.visible_tanks.begin(), current_view.visible_tanks.end(),
                            [&](auto const & obj){return obj.id_ == occ;});

                        if (this_tank != current_view.visible_tanks.end())
                        {
                            std::cout << colors_array[this_tank->owner_] << +this_tank->id_ << "\033[0m ";
                        }
                        else
                        {
                            std::cout << "\033[48;5;184m" << "_" << "\033[0m ";
                        }

                    }
                }
                else
                {
                    if (curr.type_ == CellType::Terrain)
                    {
                        std::cout << "\033[48;5;130m" << curr << "\033[0m ";
                    }
                    else
                    {
                        std::cout << "?" << " ";
                    }
                }
            }
            std::cout << "\n";
        }

        std::cout << "\n";



            game_manager_.update_view(current_view);

            break;
        }
        case HeaderType::FailedMove:
        {
            std::cerr << "Failed to execute move.\n";
            break;
        }
        case HeaderType::StaleMove:
        {
            std::cerr << "Stale move detected.\n";
            break;
        }
        case HeaderType::Eliminated:
        {
            std::cerr << "You were eliminated from the game.\n";
            break;
        }
        case HeaderType::TimedOut:
        {
            std::cerr << "You were eliminated from the game via timeout.\n";
            break;
        }
        case HeaderType::Victory:
        {
            std::cerr << "You won your match.\n";
            break;
        }
        case HeaderType::GameEnded:
        {
            std::cerr << "Game has already ended.\n";
            break;
        }
        case HeaderType::BadMessage:
        {
            std::cerr << "Bad message, server terminating connection.\n";
            break;
        }
        case HeaderType::PingTimeout:
        {
            std::cerr << "Connection dropped by server because heartbeat "
                      << "ping was not responded to.\n";
            break;
        }
        case HeaderType::RateLimited:
        {
            std::cerr << "You are being rate limited.\n";
            break;
        }
        case HeaderType::Banned:
        {
            BanMessage banned = msg.to_ban_message();

            std::time_t time = std::chrono::system_clock::to_time_t
                                    (
                                        banned.time_till_unban
                                    );

            std::tm tm;
#ifdef _WIN32
            localtime_s(&tm, &time);
#else
            localtime_r(&time, &tm);
#endif
            std::cout << "You are banned until "
                      << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
                      << "\n";

            break;
        }
        case HeaderType::ServerFull:
        {
            std::cerr << "Server is full and cannot accept connections.\n";
            break;
        }
        case HeaderType::DirectTextMessage:
        {
            TextMessage dm = msg.to_text_message();
            std::string sender_username;

            {
                std::lock_guard lock(data_mutex_);
                auto user_ref = client_data_.friends.find(dm.user_id);
                sender_username = (user_ref->second).username;
            }

            std::cout << "["
                      << sender_username << "]: "
                      << dm.text
                      << "\n";
            break;
        }
        case HeaderType::MatchTextMessage:
        {
            // Get an internal representation of the message.
            InternalMatchMessage match_msg = msg.to_match_message();
            std::cout << "["
                      << match_msg.sender_username
                      << "]: "
                      << match_msg.text
                      << "\n";
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

    std::cerr << "Error in message handling! Closing!\n";
    session->close_session();
}
}

void Client::on_connection()
{
    {
        change_state(ClientState::LoginScreen);
    }
}

// Have our disconnecting session object post to our client, since the
// client runs for the lifetime of the application.
void Client::on_disconnect()
{
    asio::post(client_strand_,
        [this]{

        {
            change_state(ClientState::Disconnected);
        }

        {
            std::lock_guard lock(data_mutex_);
            client_data_.client_username.clear();
        }

        std::cout << "Disconnected\n";

        // TODO: figure out how disconnects should work later.
    });

}
