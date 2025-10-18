#include "client-session.h"

ClientSession::ClientSession(asio::io_context & cntx)
:ssl_cntx_(asio::ssl::context::tls_client),
ssl_socket_(cntx, ssl_cntx_),
strand_(cntx.get_executor()),
resolver_(cntx),
connect_timer_(cntx)
{
    ssl_cntx_.set_options(
        asio::ssl::context::default_workarounds
        | asio::ssl::context::no_sslv2
        | asio::ssl::context::no_sslv3
        | asio::ssl::context::no_tlsv1
        | asio::ssl::context::no_tlsv1_1
    );
}

void ClientSession::set_message_handler(MessageHandler m_handler,
                                        ConnectionHandler c_handler,
                                        DisconnectHandler d_handler)
{
    on_message_relay_ = std::move(m_handler);
    on_connection_relay_ = std::move(c_handler);
    on_disconnect_relay_ = std::move(d_handler);
}

void ClientSession::start(std::string host, std::string port)
{
    std::cout << "Starting client session!\n";

    ssl_cntx_.set_default_verify_paths();
    ssl_socket_.set_verify_mode(asio::ssl::verify_peer);
    ssl_socket_.set_verify_callback(asio::ssl::rfc2818_verification(host));

    resolver_.async_resolve(
      host, port,
      asio::bind_executor(strand_,
        [self = shared_from_this()](auto ec, auto endpoints){

            self->on_resolve(ec, endpoints);

        }));
}

void ClientSession::deliver(Message msg)
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

void ClientSession::on_resolve(boost::system::error_code ec,
                               asio::ip::tcp::resolver::results_type endpoints)
{
    if (ec)
    {
        std::cerr << "Resolve failed " << ec.message() << "\n";
        return;
    }

    // Start connect timer so we don't get stuck waiting forever.
    connect_timer_.expires_after(std::chrono::seconds(CONNECTION_WAIT_TIME));
    connect_timer_.async_wait(
        asio::bind_executor(strand_,
            [self = shared_from_this()](auto ec){
                self->on_connect_timeout(ec);
        }));

    // Start async connection to server.
    asio::async_connect(ssl_socket_.lowest_layer(), endpoints,
        asio::bind_executor(strand_,
            [self = shared_from_this()](boost::system::error_code ec,
                                        asio::ip::tcp::endpoint endpoints){
                self->on_connect(ec, endpoints);
        }));
}

void ClientSession::on_connect(boost::system::error_code ec,
                               asio::ip::tcp::endpoint ep)
{
    connect_timer_.cancel();

    if (ec)
    {
        return;
    }

    std::cout << "Successful connection to " << ep << " starting TLS handshake \n";

    ssl_socket_.async_handshake(asio::ssl::stream_base::client,
        asio::bind_executor(strand_,
            [self = shared_from_this()](boost::system::error_code ec)
            {
                std::cout << "Handshake callback, ec=" << ec.message()
                      << " (" << ec.value() << ")\n";
                if (!ec)
                {
                    std::cerr << "TLS handshake successful (CA verified)\n";

                    self->live_.store(true);

                    // Tell the client that we connected.
                    if (self->on_connection_relay_)
                    {
                        self->on_connection_relay_();
                    }
                    else
                    {
                        std::cerr << "Connection handler not set in session instance!\n";
                    }

                    self->do_read_header();
                    return;
                }
                else
                {
                    return;
                    std::cerr << "CA based TLS handshake failed: "
                              << ec.message()
                              << " Falling back to fingerprint verification.\n";

                    if(self->verify_fingerprint())
                    {
                        std::cerr << "Fingerprint verified, TLS is enabled.\n";

                        self->live_.store(true);

                        // Tell the client that we connected.
                        if (self->on_connection_relay_)
                        {
                            self->on_connection_relay_();
                        }
                        else
                        {
                            std::cerr << "Connection handler not set in session instance!\n";
                        }

                        self->do_read_header();
                        return;
                    }
                    else
                    {
                        std::cerr << "Fingerprint failed to verify. Closing session.\n";
                        self->force_close_session();
                        return;
                    }
                }

            }));
}

void ClientSession::on_connect_timeout(boost::system::error_code ec)
{
    // Connect finished and raced with timer.
    if (ec == asio::error::operation_aborted)
    {
        return;
    }

    std::cerr << "Connection timed out, cancelling\n";
    ssl_socket_.lowest_layer().cancel();
}

bool ClientSession::verify_fingerprint()
{
    return false;
}

void ClientSession::do_read_header()
{
    auto self = shared_from_this();
    // Read from the header using buffer of size of the header.
    asio::async_read(ssl_socket_,
        asio::buffer(&incoming_header_, sizeof(Header)),
        asio::bind_executor(strand_,
            [self](boost::system::error_code ec, std::size_t)
            {
                // If no error, turn header into host bytes
                if (!ec)
                {
                    self->incoming_header_ = self->incoming_header_.from_network();

                    // If valid, read the body.
                    if (self->incoming_header_.valid_client())
                    {
                        self->do_read_body();
                    }

                    // Otherwise, invalid header detected. Do fail fast
                    // disconnect.
                    else
                    {
                        std::cerr << "Invalid header from server: " << +(static_cast<uint8_t>(self->incoming_header_.type_)) << "\n";

                        // Close the session after sending
                        self->force_close_session();
                    }
                }
                else
                {
                    self->handle_read_error(ec);
                }
            }));
}

void ClientSession::do_read_body()
{
    auto self = shared_from_this();
    // capped by valid message size
    incoming_body_.resize(self->incoming_header_.payload_len);

    asio::async_read(ssl_socket_,
        asio::buffer(incoming_body_),
        asio::bind_executor(strand_,
            [self](boost::system::error_code ec, std::size_t)
            {
                if(!ec)
                {
                    // Post work to client.
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

// TODO <optimization>: consider optimizing move semantics here.
void ClientSession::do_write()
{
    auto self = shared_from_this();

    const Message & msg = write_queue_.front();

    // msg buffer
    std::array<asio::const_buffer, 2> bufs
    {
        // Buffer over the Header struct and payload
        {
            asio::buffer(&msg.header, sizeof(Header)),
            asio::buffer(msg.payload)
        }
    };

    asio::async_write(ssl_socket_,
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

void ClientSession::handle_message()
{
    // Turn bytes into message
    Message msg;
    msg.header = incoming_header_;
    msg.payload = std::move(incoming_body_);

    if (msg.header.type_ == HeaderType::Ping)
    {
        // Respond immediately to server heartbeat pings.
        Message heartbeat;
        heartbeat.create_serialized(HeaderType::PingResponse);
        deliver(heartbeat);

        std::cout << "Responded to server ping.\n";
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

void ClientSession::handle_read_error(boost::system::error_code ec)
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

void ClientSession::handle_write_error(boost::system::error_code ec)
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

void ClientSession::close_session()
{
    // if already being closed, ignore call and don't
    // waste resources on strand post
    if (!is_live())
    {
        return;
    }

    live_.store(false, std::memory_order_release);

    force_close_session();
}

void ClientSession::force_close_session()
{
    asio::dispatch(strand_, [self = shared_from_this()]
    {
        boost::system::error_code ignored;

        self->ssl_socket_.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both,
                                                  ignored);
        self->ssl_socket_.lowest_layer().close(ignored);

        if(self->on_disconnect_relay_)
        {
            self->on_disconnect_relay_();
        }
        else
        {
            std::cerr << "Disconnect handler wasn't set!\n";
        }

    });
}
