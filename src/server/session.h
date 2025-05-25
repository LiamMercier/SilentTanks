#pragma once
#include <memory>
#include <deque>
#include <iostream>
#include <utility>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "message.h"
#include "header.h"
#include "user-data.h"

namespace asio = boost::asio;

class Session : public std::enable_shared_from_this<Session>
{
public:
    using ptr = std::shared_ptr<Session>;
    using tcp = asio::ip::tcp;

    // callback function to relay messages through
    using MessageHandler = std::function<void(const ptr& session, Message msg)>;
    using DisconnectHandler = std::function<void(const ptr& session)>;

    Session(asio::io_context & cntx, uint64_t session_id);

    void set_message_handler(MessageHandler handler, DisconnectHandler d_handler);

    void start();

    // disable copy
    Session(const Session&)            = delete;
    Session& operator=(const Session&) = delete;

    // disable move
    Session(Session&&)                 = delete;
    Session& operator=(Session&&)      = delete;

    // Send a message to the client
    //
    // If the message does not need to be preserved, one can
    // use deliver(std::move(msg)) to avoid copies.
    void deliver(Message msg);

    void close_session();

    void set_session_data(UserData user_data);

    inline tcp::socket& socket();

    inline uint64_t id() const;

    inline bool is_authenticated() const;

    inline bool is_live() const;

    inline UserData get_user_data() const;

private:
    void do_read_header();

    void do_read_body();

    void do_write();

    void handle_message();

    void handle_read_error(boost::system::error_code ec);

    void handle_write_error(boost::system::error_code ec);

    void force_close_session();

    // TODO: handle invalid headers
    //       mitigate spam/syn flood style attacks

public:

private:
    tcp::socket socket_;
    asio::strand<asio::io_context::executor_type> strand_;

    // Unique ID for this session. Session IDs are used internally
    // so we can simply use uint64_t for our implementation.
    //
    // This value is never changed.
    const uint64_t session_id_;

    // Remains false until a login, then remains true until
    // object is destroyed.
    //
    // We want to use this to prevent calls to our database
    // when we already are authenticated.
    std::atomic<bool> authenticated_;

    // For managing game queues
    std::atomic<bool> live_;

    // Mutex to prevent needing a strand call for this data
    // since it will probably not be accessed by multiple people.
    mutable std::mutex user_data_mutex_;
    UserData user_data_;

    // Data related members.
    Header incoming_header_;
    std::vector<uint8_t> incoming_body_;
    std::deque<Message> write_queue_;

    // Callbacks.
    MessageHandler on_message_relay_;
    DisconnectHandler on_disconnect_relay_;
};

inline asio::ip::tcp::socket& Session::socket()
{
    return socket_;
}

inline uint64_t Session::id() const
{
    return session_id_;
}

bool Session::is_authenticated() const
{
    return authenticated_.load(std::memory_order_acquire);
}

inline bool Session::is_live() const
{
    return live_.load(std::memory_order_acquire);
}

inline UserData Session::get_user_data() const
{
    std::lock_guard lock(user_data_mutex_);
    return user_data_;
}
