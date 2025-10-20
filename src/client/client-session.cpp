#include "client-session.h"

using SHA256_ARRAY = std::array<unsigned char, SHA256_DIGEST_LENGTH>;

constexpr int hex_char_to_value(char c)
{
    if (c >= '0' && c <= '9')
    {
        return static_cast<unsigned char>(c - '0');
    }
    else if (c >= 'a' && c <= 'f')
    {
        return static_cast<unsigned char>(10 + (c - 'a'));
    }
    else if (c >= 'A' && c <= 'F')
    {
        return static_cast<unsigned char>(10 + (c - 'A'));
    }
    else
    {
        return -1;
    }
}

// Utility to convert the user's hexademical string to bytes.
std::vector<unsigned char> hex_string_to_bytes(std::string user_hex)
{
    // Remove leading "0x" if present.
    std::string temp = user_hex;
    if (temp.size() >= 2
        && temp[0] == '0'
        && (temp[1] == 'x' || temp[1] == 'X'))
    {
        temp.erase(0, 2);
    }

    // We expect twice as many characters to represent the bytes,
    // since "FF" fits in one byte but takes two bytes of characters.
    if (temp.size() != SHA256_DIGEST_LENGTH * 2)
    {
        return {};
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(SHA256_DIGEST_LENGTH);

    // Step by 2 each time.
    for (size_t i = 0; i < temp.size(); i += 2)
    {
        int high = hex_char_to_value(temp[i]);
        int low = hex_char_to_value(temp[i + 1]);

        if (high < 0 || low < 0)
        {
            return {};
        }

        // Shifting over by 4 lets us avoid computing high and low differently.
        bytes.push_back(static_cast<unsigned char>((high << 4) | low));
    }

    return bytes;
}

bool fingerprint_public_key(X509* cert, unsigned char out[SHA256_DIGEST_LENGTH])
{
    if (!cert)
    {
        return false;
    }

    EVP_PKEY* public_key = X509_get_pubkey(cert);

    if (!public_key)
    {
        return false;
    }

    // Convert public key to DER (i2d_PUBKEY) into a buffer
    int len = i2d_PUBKEY(public_key, nullptr);

    if (len <= 0)
    {
        EVP_PKEY_free(public_key);
        return false;
    }

    unsigned char* buf = (unsigned char*) OPENSSL_malloc(len);

    if (!buf)
    {
        EVP_PKEY_free(public_key);
        return false;
    }

    unsigned char* p = buf;
    i2d_PUBKEY(public_key, &p);

    SHA256(buf, len, out);

    OPENSSL_free(buf);
    EVP_PKEY_free(public_key);
    return true;
}

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

void ClientSession::start(std::string host,
                          std::string port,
                          std::string fingerprint)
{
    std::cout << "Starting client session!\n";

    in_fingerprint_ = std::move(fingerprint);

    ssl_cntx_.set_default_verify_paths();
    ssl_socket_.set_verify_mode(asio::ssl::verify_peer);

    // Custom verification callback. First check if the certificate
    // is signed by a CA, then check against the given fingerprint.
    ssl_socket_.set_verify_callback(
    [this](bool preverified, asio::ssl::verify_context & cntx)
    {
        X509_STORE_CTX* x509_ctx = cntx.native_handle();
        int depth = X509_STORE_CTX_get_error_depth(x509_ctx);

        // We only want to use hash fallbacks on the leaf certificate.
        if (depth != 0)
        {
            return preverified;
        }

        X509* cert = X509_STORE_CTX_get_current_cert(x509_ctx);

        // If no cert, just return now.
        if (!cert)
        {
            return preverified;
        }

        // If depth 0 and preverified, return now, we had a CA signed cert.
        if (preverified)
        {
            return preverified;
        }

        // Otherwise, try to match the public key hash of this leaf to the
        // user submitted hash.
        unsigned char server_hash[SHA256_DIGEST_LENGTH];
        if (!fingerprint_public_key(cert, server_hash))
        {
            std::cerr << "Failed to compute public key fingerprint.\n";
            return preverified;
        }

        std::vector<unsigned char> user_hash = hex_string_to_bytes(in_fingerprint_);

        if (user_hash.size() != SHA256_DIGEST_LENGTH)
        {
            std::cerr << "Fingerprint given by user has bad size.";
            return preverified;
        }

        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
        {
            if (user_hash[i] != server_hash[i])
            {
                return false;
            }
        }

        // If we pass all checks, good cert.
        return true;
    });

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
                if (!ec)
                {
                    std::cerr << "TLS handshake successful\n";

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
                    std::cerr << "TLS failed to verify. Closing session.\n";
                    self->force_close_session();
                    return;
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
