#pragma once
#include <memory>
#include <deque>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "message.h"
#include "header.h"

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

    // Send a message to the client
    //
    // If the message does not need to be preserved, one can
    // use deliver(std::move(msg)) to avoid copies.
    void deliver(Message msg);

    inline tcp::socket& socket();

    inline uint64_t id() const;

private:
    void do_read_header();

    void do_read_body();

    void do_write();

    void handle_message();

    void handle_read_error(boost::system::error_code ec);

    void handle_write_error(boost::system::error_code ec);

    void close_session();

    void set_login_true();

    // TODO: handle invalid headers
    //       mitigate spam/syn flood style attacks

public:

private:
    tcp::socket socket_;
    asio::strand<asio::io_context::executor_type> strand_;

    // Unique ID for this session. Session IDs are used internally
    // so we can simply use uint64_t for our implementation.
    uint64_t session_id_;
    // Remains false until a login, then remains true until
    // object is destroyed.
    //
    // We want to use this to prevent calls to our database
    // when we already are logged in.
    bool logged_in_;

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
