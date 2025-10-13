#include "client.h"

#include <argon2.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>

// Helper function to see if a prefix is in a message.
constexpr size_t found_prefix(std::string_view text, std::string_view prefix)
{
    // Return 0 if prefix not found.
    if (text.size() < prefix.size()
        || (text.substr(0, prefix.size()) != prefix))
    {
        return 0;
    }

    // Otherwise, return the size.
    return prefix.size();
}

// Helper to convert an entire string to lowercase.
constexpr std::string to_lower(std::string_view s)
{
    std::string result(s);

    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) {
                       return std::tolower(c);
                });

    return result;
}

Client::Client(asio::io_context & cntx,
               LoginCallback login_callback,
               StateChangeCallback state_change_callback,
               PopupCallback popup_callback,
               UsersUpdatedCallback users_updated_callback,
               QueueUpdateCallback queue_update_callback,
               DisplayMessageCallback display_message_callback,
               MatchHistoryCallback match_history_callback,
               ViewUpdateCallback view_callback,
               MatchDataCallback match_data_callback,
               MatchReplayCallback match_replay_callback)
:io_context_(cntx),
client_strand_(cntx.get_executor()),
state_(ClientState::ConnectScreen),
login_callback_(std::move(login_callback)),
state_change_callback_(std::move(state_change_callback)),
popup_callback_(std::move(popup_callback)),
users_updated_callback_(std::move(users_updated_callback)),
queue_update_callback_(std::move(queue_update_callback)),
display_message_callback_(std::move(display_message_callback)),
match_history_callback_(std::move(match_history_callback)),
view_callback_(std::move(view_callback)),
match_data_callback_(std::move(match_data_callback)),
match_replay_callback_(std::move(match_replay_callback))
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

        if (username.length() < 1)
        {
            popup_callback_(Popup(
                            PopupType::Info,
                            "Login Failed",
                            "Your username must not be empty!"),
                            STANDARD_POPUP);
            return;
        }

        if (password.length() < 1)
        {
            popup_callback_(Popup(
                            PopupType::Info,
                            "Login Failed",
                            "Your password must not be empty!"),
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
                            "Login Failed",
                            body),
                            STANDARD_POPUP);
            return;
        }

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
            std::string body = "Argon2 encountered an error during password hashing.";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Login Failed",
                            body),
                            URGENT_POPUP);
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
                            "Registration Failed",
                            "Your username must not be empty."),
                            STANDARD_POPUP);
            return;
        }

        if (password.length() < 1)
        {
            popup_callback_(Popup(
                            PopupType::Info,
                            "Registration Failed",
                            "Your password must not be empty."),
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
                            "Registration Failed",
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
            std::string body = "Argon2 encountered an error during password hashing.";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Registration Failed",
                            body),
                            URGENT_POPUP);
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

        if (username.length() < 1)
        {
            std::string body = "Username was empty.";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Friend Request Failed",
                            body),
                            STANDARD_POPUP);
            return;
        }

        if (username.length() > MAX_USERNAME_LENGTH)
        {
            std::string body = "Username is too long to exist.";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Friend Request Failed",
                            body),
                            STANDARD_POPUP);
            return;
        }

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

        // Remove friend request and update GUI.
        {
            std::lock_guard lock(data_mutex_);
            client_data_.friend_requests.erase(user_id);
        }

        users_updated_callback_(client_data_.friend_requests,
                                UserListType::FriendRequests);

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

        if (username.length() < 1)
        {
            std::string body = "Username was empty.";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Block Failed",
                            body),
                            STANDARD_POPUP);
            return;
        }

        if (username.length() > MAX_USERNAME_LENGTH)
        {
            std::string body = "Username is too long to exist.";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Block Failed",
                            body),
                            STANDARD_POPUP);
            return;
        }

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

    if (mode >= GameMode::NO_MODE)
    {
        std::string body = std::string("Selected mode for queue does not exist.");
        popup_callback_(Popup(
                        PopupType::Info,
                        "Queue Failed",
                        body),
                        URGENT_POPUP);
        return;
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

        // Set the current queued mode to this mode. Updated to
        // NO_MODE if the server sends BadQueue after.
        queue_update_callback_(mode);

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

    if (mode >= GameMode::NO_MODE)
    {
        std::string body = std::string("Selected mode for cancel does not exist.");
        popup_callback_(Popup(
                        PopupType::Info,
                        "Cancel Failed",
                        body),
                        URGENT_POPUP);
        return;
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

        // Set the current queued mode to NO_MODE. Updated if the
        // server sends BadCancel after.
        queue_update_callback_(GameMode::NO_MODE);
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

void Client::interpret_message(std::string message)
{
    if (message.size() < 1)
    {
        return;
    }

    asio::post(client_strand_,
        [this,
        text = std::move(message)]{

        // We are given a message that may either be a direct message
        // or a message to the game. If the text starts with "/msg" or
        // "/dm" or "/pm" then we convert this into a private message
        // assuming we can find the user on the friends list.
        constexpr std::string_view msg = "/msg";
        constexpr std::string_view dm = "/dm";
        constexpr std::string_view pm = "/pm";

        std::string_view text_view = text;

        // Hold the length of the prefix, if we find one.
        size_t prefix_len = 0;

        // If direct message, do not send to lobby.
        if ((prefix_len = found_prefix(text_view, msg))
            || (prefix_len = found_prefix(text_view, dm))
            || (prefix_len = found_prefix(text_view, pm)))
        {
            // We need to extract the username. A properly
            // formatted dm will have form:
            //
            // <DM command> <username> <content>

            // Start by removing the prefix and whitespace.
            text_view.remove_prefix(prefix_len);

            while (!text_view.empty()
                    && std::isspace(static_cast<unsigned char>(text_view.front())))
            {
                text_view.remove_prefix(1);
            }

            // Find next space. If it does not exist, message is malformed.
            size_t pos = text_view.find(' ');
            if (pos == std::string_view::npos)
            {
                return;
            }

            // Grab username.
            std::string_view username_view = text_view.substr(0, pos);
            std::string username = to_lower(username_view);
            text_view.remove_prefix(pos + 1);

            // Check that text content is not empty.
            if (text_view.size() < 1)
            {
                return;
            }

            boost::uuids::uuid friend_id;

            // Find the UUID for this user in our list of friends.
            //
            // We could also store username -> user in our client data,
            // but this shouldn't be too slow with a reasonable number of friends.
            {
                std::lock_guard lock(data_mutex_);
                for (const auto & [uuid, user] : client_data_.friends)
                {
                    if (to_lower(user.username) == username)
                    {
                        friend_id = user.user_id;
                        break;
                    }
                }
            }

            // If we cannot find the friend, stop now.
            if (friend_id == boost::uuids::nil_uuid())
            {
                return;
            }

            // Write the contents to our callback listener.
            std::string callback_formatted_string = "[To: "
                                                    + std::string(username_view)
                                                    + "]: "
                                                    + std::string(text_view);
            display_message_callback_(std::move(callback_formatted_string));

            // Send the message.
            this->send_direct_message(std::string(text_view), friend_id);
            return;
        }

        // Otherwise, check if the message starts with "/" to prevent
        // typos from exposing dm's to the lobby.
        if (text_view[0] == '/')
        {
            return;
        }

        // Finally, send message to lobby if we are sure it is
        // not a direct message.
        //
        // We also must write the contents.
        std::string callback_formatted_string = "[You]: " + std::string(text_view);
        display_message_callback_(std::move(callback_formatted_string));

        this->send_match_message(text);
    });
}

void Client::fetch_match_history(GameMode mode)
{
    asio::post(client_strand_,
        [this,
        mode]{

        MatchHistoryRequest history_req(mode);

        Message match_history_request;
        match_history_request.create_serialized(history_req);
        current_session_->deliver(match_history_request);
    });
}

void Client::request_match_replay(uint64_t match_id)
{
    asio::post(client_strand_,
        [this,
        match_id]{

        std::cout << "Trying to download match " << static_cast<uint64_t>(match_id) << "\n";

        ReplayRequest replay_req(match_id);

        Message replay_request_msg;
        replay_request_msg.create_serialized(replay_req);
        current_session_->deliver(replay_request_msg);

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
            std::string body = std::string("The server is not accepting your request.")
                               + std::string(" Try logging in first?");
            popup_callback_(Popup(
                            PopupType::Info,
                            "Action Unauthorized",
                            body),
                            STANDARD_POPUP);
            break;
        }
        case HeaderType::AlreadyAuthorized:
        {
            std::string body = "You are already logged into an account.";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Already Authorized",
                            body),
                            STANDARD_POPUP);
            break;
        }
        case HeaderType::GoodRegistration:
        {
            std::string body = "Your account has been registered successfully.";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Account Registered",
                            body),
                            STANDARD_POPUP);
            break;
        }
        case HeaderType::BadRegistration:
        {
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
                            "Registration Failed",
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

                // Set user elo's.
                std::array<int, RANKED_MODES_COUNT> elos = msg.to_elos();
                client_data_.display_elos = std::move(elos);

                bool should_change = false;

                {
                    std::lock_guard lock(state_mutex_);
                    // Prevent swapping back to lobby on bad packet ordering
                    // when we reconnect to a game.
                    if (state_ != ClientState::Playing)
                    {
                        should_change = true;
                    }
                }

                if (should_change)
                {
                    change_state(ClientState::Lobby);
                }

                login_callback_(client_data_.client_username);
            }

            // Request user lists on good auth.
            request_user_list(UserListType::Friends);

            request_user_list(UserListType::FriendRequests);

            request_user_list(UserListType::Blocks);
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
                            "Login Failed",
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
                // Callback to GUI.
                users_updated_callback_(client_data_.friends,
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

                // Callback to GUI.
                users_updated_callback_(client_data_.friend_requests,
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
                // Callback to GUI.
                users_updated_callback_(client_data_.blocked_users,
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
                users_updated_callback_(client_data_.friends,
                                        UserListType::Friends);
            }
            break;
        }
        case HeaderType::NotifyFriendRemoved:
        {
            ExternalUser user = msg.to_user();

            {
                std::lock_guard lock(data_mutex_);
                client_data_.friends.erase(user.user_id);
                users_updated_callback_(client_data_.friends,
                                        UserListType::Friends);
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
                users_updated_callback_(client_data_.friend_requests,
                                        UserListType::FriendRequests);
            }
            break;
        }
        case HeaderType::NotifyBlocked:
        {
            ExternalUser user = msg.to_user();

            {
                std::lock_guard lock(data_mutex_);
                client_data_.blocked_users.emplace(user.user_id, user);
                client_data_.friends.erase(user.user_id);
                client_data_.friend_requests.erase(user.user_id);

                users_updated_callback_(client_data_.blocked_users,
                                        UserListType::Blocks);

                // Also notify friends and requests since blocking removes entries.
                users_updated_callback_(client_data_.friends,
                                        UserListType::Friends);

                users_updated_callback_(client_data_.friend_requests,
                                        UserListType::FriendRequests);
            }
            break;
        }
        case HeaderType::NotifyUnblocked:
        {
            ExternalUser user = msg.to_user();

            {
                std::lock_guard lock(data_mutex_);
                client_data_.blocked_users.erase(user.user_id);
                users_updated_callback_(client_data_.blocked_users,
                                        UserListType::Blocks);
            }
            break;
        }
        case HeaderType::BadQueue:
        {
            {
                std::lock_guard lock(data_mutex_);
                last_queued_mode_ = GameMode::NO_MODE;
                queue_update_callback_(last_queued_mode_);
            }
            std::string body = std::string("The server failed to enqueue you.");
            popup_callback_(Popup(
                            PopupType::Info,
                            "Queue Failed",
                            body),
                            URGENT_POPUP);
            break;
        }
        case HeaderType::BadCancel:
        {
            {
                std::lock_guard lock(data_mutex_);
                queue_update_callback_(last_queued_mode_);
            }
            std::string body = std::string("You are not queued up or the ")
                               + std::string("server failed to cancel the queue.");
            popup_callback_(Popup(
                            PopupType::Info,
                            "Cancel Failed",
                            body),
                            URGENT_POPUP);
            break;
        }
        case HeaderType::QueueDropped:
        {
            {
                std::lock_guard lock(data_mutex_);
                last_queued_mode_ = GameMode::NO_MODE;
                queue_update_callback_(last_queued_mode_);
            }

            std::string body = std::string("Your queue was dropped by the server.");
            popup_callback_(Popup(
                            PopupType::Info,
                            "Queue Dropped",
                            body),
                            URGENT_POPUP);
            break;
        }
        case HeaderType::MatchStarting:
        {
            std::cerr << "Match is starting.\n";

            {
                std::lock_guard lock(data_mutex_);
                last_queued_mode_ = GameMode::NO_MODE;
                queue_update_callback_(last_queued_mode_);

                change_state(ClientState::Playing);
            }

            playing_.store(true, std::memory_order_release);

            break;
        }
        case HeaderType::MatchCreationError:
        {
            {
                std::lock_guard lock(data_mutex_);
                last_queued_mode_ = GameMode::NO_MODE;
                queue_update_callback_(last_queued_mode_);
            }
            std::string body = std::string("There was an error in match creation.");
            popup_callback_(Popup(
                            PopupType::Info,
                            "Match Creation Error",
                            body),
                            URGENT_POPUP);
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
                playing_.store(true, std::memory_order_release);
                change_state(ClientState::Playing);
            }

            break;
        }
        case HeaderType::StaticMatchData:
        {
            bool status;
            StaticMatchData match_data = msg.to_static_match_data(status);

            if (status == false)
            {
                std::cerr << "Failed to convert static data.\n";
                break;
            }

            match_data_callback_(match_data);

            break;
        }
        case HeaderType::PlayerView:
        {

            bool status;
            PlayerView current_view = msg.to_player_view(status);

            if (status == false)
            {
                std::cerr << "Failed to convert player view.\n";
                break;
            }

            view_callback_(current_view);

            // Change to playing if necessary.
            {
                std::lock_guard lock(state_mutex_);
                if (state_ != ClientState::Playing
                    && playing_.load(std::memory_order_acquire))
                {
                    change_state(ClientState::Playing);
                }
            }

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
            popup_callback_(Popup(
                            PopupType::Info,
                            "Eliminated",
                            "You were eliminated from the game."),
                            STANDARD_POPUP);

            playing_.store(false, std::memory_order_release);

            change_state(ClientState::Lobby);
            break;
        }
        case HeaderType::TimedOut:
        {
            std::cerr << "You were eliminated from the game via timeout.\n";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Eliminated",
                            "You were eliminated from the game via timeout."),
                            STANDARD_POPUP);

            playing_.store(false, std::memory_order_release);

            change_state(ClientState::Lobby);
            break;
        }
        case HeaderType::ForfeitMatch:
        {
            std::cerr << "You were eliminated from the game via forfeit.\n";

            playing_.store(false, std::memory_order_release);

            change_state(ClientState::Lobby);
            break;
        }
        case HeaderType::Victory:
        {
            std::cerr << "You won your match.\n";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Game Won",
                            "You won your match."),
                            STANDARD_POPUP);

            playing_.store(false, std::memory_order_release);

            change_state(ClientState::Lobby);
            break;
        }
        case HeaderType::GameEnded:
        {
            std::cerr << "Game has already ended.\n";
            playing_.store(false, std::memory_order_release);
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

            std::string body = "You are being rate limited.";

            popup_callback_(Popup(
                            PopupType::Info,
                            "Rate Limited",
                            body),
                            URGENT_POPUP);

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
            std::ostringstream oss;
            oss << "You are banned until "
                << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
                <<"\n" << "For: " << banned.reason;

            std::string msg = oss.str();

            popup_callback_(Popup(
                            PopupType::Info,
                            "Banned",
                            msg),
                            URGENT_POPUP);

            break;
        }
        case HeaderType::ServerFull:
        {
            std::cerr << "Server is full and cannot accept connections.\n";
            popup_callback_(Popup(
                            PopupType::Info,
                            "Server Full",
                            "Server is full and cannot accept connections."),
                            URGENT_POPUP);
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

            std::string callback_formatted_string = "[From: "
                                                    + sender_username
                                                    + "]: "
                                                    + dm.text;
            display_message_callback_(std::move(callback_formatted_string));
            break;
        }
        case HeaderType::FriendOffline:
        {
            std::string callback_formatted_string = "User is offline";
            display_message_callback_(std::move(callback_formatted_string));
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

            std::string callback_formatted_string = "["
                                                    + match_msg.sender_username
                                                    + "]: "
                                                    + match_msg.text;
            display_message_callback_(std::move(callback_formatted_string));
            break;
        }
        case HeaderType::MatchHistory:
        {
            MatchResultList results = msg.to_results_list();

            match_history_callback_(std::move(results));
            break;
        }
        case HeaderType::MatchReplay:
        {
            std::cout << "Got replay ("
                      << msg.payload.size()
                      << " bytes).\n";

            bool status;

            MatchReplay replay = msg.to_match_replay(status);

            if (status == false)
            {
                std::cerr << "Failed to convert match replay.\n";
                break;
            }

            match_replay_callback_(std::move(replay));
            break;
        }
        default:
            // do nothing, should never happen
            break;
    }
}
catch (std::exception & e)
{
    std::cerr << "Error in message handling! Closing!\n";
    std::cerr << e.what() << "\n";
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

            // Clear the friends, friend requests, blocked users.
            client_data_.friends.clear();
            client_data_.friend_requests.clear();
            client_data_.blocked_users.clear();

            // Zero out elos.
            std::array<int, RANKED_MODES_COUNT> elos{};
            client_data_.display_elos = elos;

            last_queued_mode_ = GameMode::NO_MODE;
        }

        popup_callback_(Popup(
                            PopupType::Info,
                            "Session Disconnected",
                            "The session was disconnected."),
                            URGENT_POPUP);

        playing_.store(false, std::memory_order_release);
        change_state(ClientState::ConnectScreen);
    });

}
