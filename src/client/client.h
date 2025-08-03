#pragma once

#include "client-session.h"
#include "client-state.h"
#include "client-data.h"
#include "game-manager.h"

class Client
{
public:
    using ptr = ClientSession::ptr;

    using StateChangeCallback = std::function<void(ClientState state)>;
public:
    Client(asio::io_context & cntx,
           StateChangeCallback state_change_callback);

    inline ClientState get_state() const;

    inline void change_state(ClientState new_state);

    void connect(std::string endpoint);

    void login(std::string username, std::string password);

    void register_account(std::string username, std::string password);

    void request_user_list(UserListType list_type);

    void send_friend_request(std::string username);

    void respond_friend_request(boost::uuids::uuid user_id,
                                bool decision);

    void send_unfriend_request(boost::uuids::uuid user_id);

    void send_block_request(std::string username);

    void send_unblock_request(boost::uuids::uuid user_id);

    void queue_request(GameMode mode);

    void cancel_request(GameMode mode);

    void send_command(Command cmd);

    void forfeit_request();

    void send_direct_message(std::string text,
                             boost::uuids::uuid receiver);

    void send_match_message(std::string text);

private:
    void on_message(const ptr & session, Message msg);

    void on_connection();

    void on_disconnect();

private:
    asio::io_context & io_context_;
    asio::strand<asio::io_context::executor_type> client_strand_;

    mutable std::mutex state_mutex_;
    ClientState state_;

    mutable std::mutex data_mutex_;
    ClientData client_data_;

    ClientSession::ptr current_session_;

    GameManager game_manager_;

    StateChangeCallback state_change_callback_;
};

inline ClientState Client::get_state() const
{
    std::lock_guard lock(state_mutex_);
    return state_;
}

inline void Client::change_state(ClientState new_state)
{
    {
        std::lock_guard lock(state_mutex_);
        state_ = new_state;
    }

    state_change_callback_(new_state);
}
