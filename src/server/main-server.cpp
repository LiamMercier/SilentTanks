#include <string>
#include <iostream>
#include <thread>

#include "generic-constants.h"
#include "server.h"
#include "console.h"
#include "console-dispatch.h"

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/program_options.hpp>
#include <sodium.h>

namespace po = boost::program_options;

int main(int argc, char** argv)
{
    namespace asio = boost::asio;

    // basic test code
    if (sodium_init() == -1) {
        std::cerr << TERM_RED
                  << "libsodium failed to initialize. Exiting\n"
                  << TERM_RESET;
        return 1;
    }

    // Check all assets can be reached
    bool assets_exist = AppAssets::assert_server_assets_resolvable();

    if (!assets_exist)
    {
        std::cerr << TERM_RED
                  << "One or more required assets could not be found. Exiting.\n"
                  << TERM_RESET;
        return 1;
    }

    // Try to parse the command line.
    std::string address;
    int port = 0;

    po::options_description desc("Allowed options");

    desc.add_options()
        ("help,h", "show help message")
        ("port",
         po::value<int>(&port)->default_value(DEFAULT_SERVER_PORT),
         "Port to listen on (1-65535)")
        ("address",
         po::value<std::string>(&address)->default_value(DEFAULT_SERVER_ADDRESS),
         "Address to listen on (example: 127.0.0.1)");

    po::variables_map vars;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vars);
        po::notify(vars);
    }
    catch (const po::error & e)
    {
        std::cerr << TERM_RED
                  << "Command line error: "
                  << e.what()
                  << "\n"
                  << desc
                  << "\n"
                  << TERM_RESET;

        return 1;
    }

    if (vars.count("help"))
    {
        std::cout << desc << "\n";
        return 0;
    }

    // Ensure the port is in a valid range.
    if (port < 1 || port > 65535)
    {
        std::cerr << TERM_RED
                  << "Invalid port: "
                  << port
                  << " (must be between 1 and 65535)\n"
                  << TERM_RESET;
    }

    // TODO: address validation when ip is given

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

    asio::ip::tcp::endpoint endpoint(asio::ip::make_address(address), port);

    // Setup ssl context for the server.
    asio::ssl::context ssl_cntx(asio::ssl::context::tls_server);

    const auto cert_file = AppAssets::resolve_asset("certs/server.crt");
    const auto key_file = AppAssets::resolve_asset("certs/server.key");

    // Try to setup ssl context.
    try
    {
        ssl_cntx.use_certificate_chain_file(cert_file);
        ssl_cntx.use_private_key_file(key_file, asio::ssl::context::pem);
    }
    catch (const std::exception & e)
    {
        std::cerr << TERM_RED
                  << "SSL context setup error: " << e.what() << "\n"
                  << TERM_RESET;
        work_guard.reset();
        return 1;
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
                       + endpoint.address().to_string()
                       + ":"
                       + std::to_string(endpoint.port());

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
        std::cerr << TERM_RED
                  << "Server error: "
                  << e.what()
                  << "\n"
                  << TERM_RESET;
    }

    std::cerr << "Server stopped. Press anything to return.\n";

    Console::instance().stop_command_loop();
    restore_terminal();

    return 0;

}
