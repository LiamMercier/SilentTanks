#include "session.h"

Session::Session(asio::io_context & cntx, uint64_t session_id)
:socket_(cntx),
strand_(cntx.get_executor()),
read_timer_(cntx),
ping_timer_(cntx),
pong_timer_(cntx),
session_id_(session_id),
authenticated_(false),
called_register_(false),
user_data_()
{

}

void Session::set_message_handler(MessageHandler m_handler, DisconnectHandler d_handler)
{
    on_message_relay_ = std::move(m_handler);
    on_disconnect_relay_ = std::move(d_handler);
}

void Session::start()
{
    std::cout << "Starting session!\n";
    live_.store(true, std::memory_order_release);
    awaiting_pong_ = false;
    do_read_header();
    start_ping();
}

void Session::deliver(Message msg)
{
    asio::post(strand_, [self = shared_from_this(), m = std::move(msg)]{
        // if the write queue is currently not empty
        bool write_in_progress = !self->write_queue_.empty();
        self->write_queue_.push_back(std::move(m));
        if (!write_in_progress)
        {
            self->do_write();
        }
    });
}

void Session::close_session()
{
    // if already being closed, ignore call and don't
    // waste resources on strand post
    if (!is_live())
    {
        return;
    }

    live_.store(false, std::memory_order_release);

    asio::post(strand_, [self = shared_from_this()]
    {
        self->force_close_session();
    });
}

// To be called when we see a login from the user manager.
void Session::set_session_data(UserData user_data)
{
    asio::post(strand_, [self = shared_from_this(), data = std::move(user_data)]
    {
        // Set authenticated to true, and move a copy of the data
        std::lock_guard lock(self->user_data_mutex_);
        self->user_data_ = std::move(data);
        self->authenticated_.store(true, std::memory_order_release);
    });
}

bool Session::spend_tokens(Header header)
{
    std::lock_guard lock(tokens_mutex_);

    // Compute the client's current token count.
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - last_update).count();

    tokens = std::min(MAX_TOKENS,
                      tokens
                      + static_cast<uint64_t>(elapsed * TOKENS_REFILL_RATE));

    last_update = now;

    // Attempt to remove tokens.
    uint64_t cost = weight_of_cmd(header);
    if (tokens >= cost)
    {
        std::cout << "Spending " << cost << " of " << tokens << " tokens.\n";
        tokens -= cost;
        return true;
    }
    else
    {
        return false;
    }
}

void Session::start_ping()
{
    // Setup interval for sending pings
    ping_timer_.expires_after(std::chrono::seconds(PING_INTERVAL));

    ping_timer_.async_wait(asio::bind_executor(strand_,
        [self = shared_from_this()](boost::system::error_code ec){
        if (!ec)
        {

            Message ping_msg;
            ping_msg.create_serialized(HeaderType::Ping);
            self->deliver(std::move(ping_msg));

            self->awaiting_pong_ = true;

            self->pong_timer_.expires_after
            (
                std::chrono::seconds(PING_TIMEOUT)
            );

            self->pong_timer_.async_wait(
                asio::bind_executor(self->strand_,
                    [self](boost::system::error_code ec)
                    {
                        // Handle pong timeout
                        if (!ec && self->awaiting_pong_)
                        {
                            // Tell the client that they are timed out.
                            Message timeout_message;
                            timeout_message.create_serialized(HeaderType::PingTimeout);

                            asio::const_buffer buf{
                                &timeout_message.header,
                                sizeof(timeout_message.header)
                            };

                            // Close the session after sending
                            asio::async_write(self->socket_, buf,
                                asio::bind_executor(self->strand_,
                                    [self](boost::system::error_code, size_t){
                                        self->close_session();

                            }));
                        }

                    }));

            // End pong timer lambda

            // Setup for the next ping after another interval
            self->start_ping();

        }

    }));
}

void Session::do_read_header()
{
    auto self = shared_from_this();
    // read from the header using buffer of size of the header
    asio::async_read(socket_,
        asio::buffer(&incoming_header_, sizeof(Header)),
        asio::bind_executor(strand_,
            [self](boost::system::error_code ec, std::size_t)
            {
                // If no error, turn header into host bytes
                if (!ec)
                {
                    self->incoming_header_ = self->incoming_header_.from_network();

                    // If valid, read the body
                    if (self->incoming_header_.valid())
                    {
                        self->do_read_body();
                    }

                    // Otherwise, invalid header detected. Do fail fast
                    // disconnecting on the misbehaving client.
                    //
                    // If the client is not malicious, they can simply
                    // reconnect to the server.
                    else
                    {
                        // Tell the client that they have sent a bad message.
                        Message bad_message;
                        bad_message.create_serialized(HeaderType::BadMessage);

                        asio::const_buffer buf{
                            &bad_message.header,
                            sizeof(bad_message.header)
                        };

                        // Close the session after sending
                        asio::async_write(self->socket_, buf,
                                asio::bind_executor(self->strand_,
                                    [self](boost::system::error_code, size_t){
                                        self->close_session();
                        }));
                    }
                }
                else
                {
                    self->handle_read_error(ec);
                }
            }));
}

void Session::do_read_body()
{
    auto self = shared_from_this();
    // capped by valid message size
    incoming_body_.resize(self->incoming_header_.payload_len);

    // Setup an async timer to ensure malicious clients don't just
    // pretend to send data and never go through with it.
    read_timer_.expires_after(std::chrono::seconds(READ_TIMEOUT));
    read_timer_.async_wait(asio::bind_executor(strand_, [self](boost::system::error_code ec){

        if (!ec)
        {
            self->socket_.cancel();
        }

    }));

    asio::async_read(socket_,
        asio::buffer(incoming_body_),
        asio::bind_executor(strand_,
            [self](boost::system::error_code ec, std::size_t)
            {
                self->read_timer_.cancel();

                if(!ec)
                {
                    self->handle_message();

                    // back to reading for headers
                    self->do_read_header();
                }
                else
                {
                    // handle error ec
                    self->handle_read_error(ec);
                }
            }));
}

void Session::do_write()
{
    auto self = shared_from_this();

    const Message& msg = write_queue_.front();

    // msg buffer
    std::array<asio::const_buffer, 2> bufs
    {
        // Buffer over the Header struct and payload
        {
            asio::buffer(&msg.header, sizeof(Header)),
            asio::buffer(msg.payload)
        }
    };

    asio::async_write(socket_,
                      bufs,
                      asio::bind_executor(strand_,
                        [self](boost::system::error_code ec, std::size_t)
                        {
                            if (!ec)
                            {
                                self->write_queue_.pop_front();
                                if (!self->write_queue_.empty())
                                {
                                    self->do_write();
                                }
                            }
                            else
                            {
                                // handle error with msg sending
                                self->handle_write_error(ec);
                            }
                        }));
}

void Session::handle_message()
{
    // Turn bytes into message
    Message msg;
    msg.header = incoming_header_;
    msg.payload = std::move(incoming_body_);

    if (msg.header.type_ == HeaderType::PingResponse)
    {
        awaiting_pong_ = false;
        pong_timer_.cancel();
        return;
    }

    // callback to server/client code if callback exists
    if (on_message_relay_)
    {
        asio::post(strand_,
                   [self = shared_from_this(), m = std::move(msg)]
                   {self->on_message_relay_(self, m);});
    }
    // no callback set
    else
    {
        std::cerr << "No message callback was set!\n";
    }
}

void Session::handle_read_error(boost::system::error_code ec)
{
    switch (ec.value())
    {
        case asio::error::eof:
            close_session();
            break;
        case asio::error::connection_reset:
            close_session();
            break;
        case asio::error::connection_aborted:
            close_session();
            break;
        case asio::error::operation_aborted:
            close_session();
            break;
        default:
            close_session();
            break;

    }
}

void Session::handle_write_error(boost::system::error_code ec)
{
    switch (ec.value())
    {
        case asio::error::broken_pipe:
            close_session();
            break;
        case asio::error::connection_reset:
            close_session();
            break;
        case asio::error::connection_aborted:
            close_session();
            break;
        case asio::error::operation_aborted:
            close_session();
            break;
        default:
            close_session();
            break;

    }
}

void Session::force_close_session()
{
    asio::dispatch(strand_, [self = shared_from_this()]
    {
        boost::system::error_code ignored;

        // Cancel all timers
        self->read_timer_.cancel();
        self->ping_timer_.cancel();
        self->pong_timer_.cancel();

        self->socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
        self->socket_.close(ignored);

        if(self->on_disconnect_relay_)
        {
            self->on_disconnect_relay_(self);
        }
        else
        {
            std::cerr << "Disconnect handler wasn't set!\n";
        }

    });
}
