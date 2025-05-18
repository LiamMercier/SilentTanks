#include "session.h"

Session::Session(asio::io_context & cntx, uint64_t session_id)
:socket_(cntx),
strand_(cntx.get_executor()),
session_id_(session_id)
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
    do_read_header();
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

                    // Otherwise, invalid header detected.
                    //
                    // we might want to flush the socket or something, and track
                    // how often this happens to potentially close/reset the connection.
                    //
                    // we need some mitigation because an attacker would be able to
                    // send many bad headers to us over and over, taking up resources.
                    else
                    {
                        self->do_read_header();
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
    asio::async_read(socket_,
        asio::buffer(incoming_body_),
        asio::bind_executor(strand_,
            [self](boost::system::error_code ec, std::size_t)
            {
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
            break;

    }
}

void Session::close_session()
{
    asio::dispatch(strand_, [self = shared_from_this()]
    {
        boost::system::error_code ignored;

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

// To be called when we see a login from the user manager.
void set_login_true()
{
    asio::post(strand_, [self = shared_from_this()=]
    {
        self->logged_in_ = true;
    };
}
