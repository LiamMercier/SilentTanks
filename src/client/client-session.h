#pragma once

#include <memory>
#include <utility>
#include <deque>
#include <functional>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

#include "message.h"
#include "header.h"
#include "server-identity.h"

constexpr int CONNECTION_WAIT_TIME = 5;

namespace asio = boost::asio;

class ClientSession : public std::enable_shared_from_this<ClientSession>
{
public:
    using ptr = std::shared_ptr<ClientSession>;
    using tcp = asio::ip::tcp;

    // callback function to relay messages through
    using MessageHandler = std::function<void(const ptr & session, Message msg)>;
    using ConnectionHandler = std::function<void()>;
    using DisconnectHandler = std::function<void()>;

    ClientSession(asio::io_context & cntx);

    void set_message_handler(MessageHandler handler,
                             ConnectionHandler c_handler,
                             DisconnectHandler d_handler);

    void start(ServerIdentity identity);

    // Disable copy.
    ClientSession(const ClientSession &) = delete;
    ClientSession & operator=(const ClientSession &) = delete;

    // Disable move.
    ClientSession(ClientSession &&) = delete;
    ClientSession & operator=(ClientSession &&) = delete;

    // Send a message to the server.
    //
    // If the message does not need to be preserved, one can
    // use deliver(std::move(msg)) to avoid copies.
    void deliver(Message msg);

    void close_session();

    inline tcp::socket & socket();

    inline bool is_live() const;

private:
    void on_resolve(boost::system::error_code ec,
                    asio::ip::tcp::resolver::results_type results);

    void on_connect(boost::system::error_code ec,
                    asio::ip::tcp::endpoint ep);

    void on_connect_timeout(boost::system::error_code ec);

    void do_read_header();

    void do_read_body();

    void do_write();

    void handle_message();

    void handle_read_error(boost::system::error_code ec);

    void handle_write_error(boost::system::error_code ec);

    void force_close_session();

public:

private:
    asio::ssl::context ssl_cntx_;
    asio::ssl::stream<tcp::socket> ssl_socket_;
    asio::strand<asio::io_context::executor_type> strand_;
    asio::ip::tcp::resolver resolver_;
    asio::steady_timer connect_timer_;

    std::atomic<bool> live_;

    std::string in_fingerprint_;

    // Data related members.
    Header incoming_header_;
    std::vector<uint8_t> incoming_body_;
    std::deque<Message> write_queue_;

    std::string dropped_reason_;

    // Callbacks.
    MessageHandler on_message_relay_;
    ConnectionHandler on_connection_relay_;
    DisconnectHandler on_disconnect_relay_;
};

inline asio::ip::tcp::socket & ClientSession::socket()
{
    return ssl_socket_.next_layer();
}

inline bool ClientSession::is_live() const
{
    return live_.load(std::memory_order_acquire);
}
