#include "client.h"

#include <argon2.h>
#include <boost/uuid/uuid_io.hpp>

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
            [this](){ on_connection(); },
            [this](){ on_disconnect(); }
        );

        this->current_session_->start(host, port);

    });
}

void Client::login(std::string username, std::string password)
{
    asio::post(client_strand_,
        [this,
        username = std::move(username),
        password = std::move(password)]{

        // Prevent trying to login early or late.
        {
            std::lock_guard lock(state_mutex_);
            if (state_ != ClientState::LoginScreen)
            {
                return;
            }
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
    asio::post(client_strand_,
        [this,
        username = std::move(username),
        password = std::move(password)]{

        // Prevent trying to login early or late.
        {
            std::lock_guard lock(state_mutex_);
            if (state_ != ClientState::LoginScreen)
            {
                return;
            }
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

        std::cout << "Sending login req\n";

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
            std::cerr << "Bad registration: ";

            using Reason = BadRegNotification::Reason;

            // We have exactly 1 byte here, or the header
            // would have been rejected.
            Reason reason = static_cast<Reason>(msg.payload[0]);

            switch (reason)
            {
                case Reason::NotUnique:
                {
                    std::cout << "Username was not unique.\n";
                    break;
                }
                case Reason::InvalidUsername:
                {
                    std::cout << "Username was invalid.\n";
                    break;
                }
                case Reason::CurrentlyAuthenticated:
                {
                    std::cout << "Currently authenticated.\n";
                    break;
                }
                case Reason::ServerError:
                {
                    std::cout << "Server error.\n";
                    break;
                }
                default:
                {
                    std::cout << "Server provided unknown "
                                 "bad registration reason.\n";
                    break;
                }
            }

            break;
        }
        case HeaderType::GoodAuth:
        {
            {
                std::lock_guard lock(data_mutex_);
                std::cerr << "Good auth. Logged in as: "
                          << client_data_.client_username << "\n";
            }
            break;
        }
        case HeaderType::BadAuth:
        {
            std::cerr << "Bad auth.\n";
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
        std::lock_guard lock(state_mutex_);
        state_ = ClientState::LoginScreen;
    }
}

// Have our disconnecting session object post to our client, since the
// client runs for the lifetime of the application.
void Client::on_disconnect()
{
    asio::post(client_strand_,
        [this]{

        {
            std::lock_guard lock(state_mutex_);
            state_ = ClientState::Disconnected;
        }

        {
            std::lock_guard lock(data_mutex_);
            client_data_.client_username.clear();
        }

        std::cout << "Disconnected\n";

        // TODO: figure out how disconnects should work later.
    });

}
