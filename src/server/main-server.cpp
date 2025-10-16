#include <string>
#include <iostream>
#include <thread>
#include <filesystem>

#include "server.h"
#include "console.h"
#include "console-dispatch.h"
#include "elo-updates.h"

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <sodium.h>

int main()
{
    namespace asio = boost::asio;

    // basic test code

    if (sodium_init() == -1) {
        std::cerr << "libsodium failed to initialize \n";
        return 1;
    }

try
{

    auto thread_count = std::thread::hardware_concurrency();

    if (thread_count == 0)
    {
        thread_count = 2;
    }

    asio::io_context server_io_context;
    auto work_guard = asio::make_work_guard(server_io_context);

    std::vector<std::thread> server_threads;
    server_threads.reserve(thread_count);

    Console::init(server_io_context, LogLevel::WARN);

    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 12345);

    // Setup ssl context for the server.
    asio::ssl::context ssl_cntx(asio::ssl::context::tls_server);

    const std::string cert_file = "certs/server.crt";
    const std::string key_file = "certs/server.key";

    // Check ssl certificate file exists.
    if (!std::filesystem::exists(cert_file))
    {
        std::cerr << "ERROR ON STARTUP: Missing server certificate file.\n"
                  << "Searched: " << cert_file << "\n";
        work_guard.reset();
        std::exit(EXIT_FAILURE);
    }

    // Check ssl keyfile exists
    if (!std::filesystem::exists(key_file))
    {
        std::cerr << "ERROR ON STARTUP: Missing server key file.\n"
                  << "Searched: " << key_file << "\n";
        work_guard.reset();
        std::exit(EXIT_FAILURE);
    }

    // Try to setup ssl context.
    try
    {
        ssl_cntx.use_certificate_chain_file(cert_file);
        ssl_cntx.use_private_key_file(key_file, asio::ssl::context::pem);
    }
    catch (const std::exception & e)
    {
        std::cerr << "SSL context setup error: " << e.what() << "\n";
        work_guard.reset();
        std::exit(EXIT_FAILURE);
    }

    // Prevent downgraded versions.
    ssl_cntx.set_options(
        asio::ssl::context::default_workarounds
        | asio::ssl::context::no_sslv2
        | asio::ssl::context::no_sslv3
        | asio::ssl::context::no_tlsv1
        | asio::ssl::context::no_tlsv1_1
    );

    Server server(server_io_context, endpoint, ssl_cntx);

    std::string lmsg = "Server started on "
                       + endpoint.address().to_string();

    Console::instance().log(lmsg, LogLevel::CONSOLE);

    // Bind the line parsing handle for the console to use.
    auto cmd_handler = std::bind(&console_dispatch,
                                 std::ref(server),
                                 std::placeholders::_1);

    Console::instance().set_command_handler(cmd_handler);
    Console::instance().start_command_loop();

    asio::signal_set signals(server_io_context, SIGINT, SIGTERM);
        signals.async_wait(
            [&](auto const & ec, int i){
            std::string lmsg = "Got SIGINT/SIGTERM from terminal.";
            Console::instance().log(lmsg, LogLevel::CONSOLE);
            server.shutdown();
            work_guard.reset();
        });

    for (unsigned int i = 0; i < thread_count; i++)
    {
        server_threads.emplace_back([&]{
            server_io_context.run();
        });
    }

    for (auto & thread : server_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

}

    catch(const boost::system::system_error& e)
    {
        std::cerr << "Client error: " << e.what() << std::endl;
    }

    std::cerr << "Server stopped. Press anything to return.\n";

    Console::instance().stop_command_loop();
    restore_terminal();

    return 0;

}
