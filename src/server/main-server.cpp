#include <string>
#include <iostream>
#include <thread>

#include "server.h"
#include <asio.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/local/stream_protocol.hpp>

int main()
{

    // basic test code
    asio::io_context server_io_context;

    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 12345);
        Server server(server_io_context, endpoint);

    std::cout << "Server started on " << endpoint << std::endl;

    std::thread server_thread([&]()
    {
        server_io_context.run();
    });

    // client logic
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    asio::io_context client_io;
    asio::ip::tcp::socket client_sock(client_io);
    try {
    client_sock.connect(endpoint);

    std::cout << "client connecting to server\n";

    Message msg;
    msg.create_serialized(QueueMatchRequest(GameMode::ClassicTwoPlayer));

    std::array<asio::const_buffer, 2> bufs
    {
        // Buffer over the Header struct and payload
        {
            asio::buffer(&msg.header, sizeof(Header)),
            asio::buffer(msg.payload)
        }
    };

    asio::write(client_sock, bufs);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    client_sock.close();

    }

    catch(const asio::system_error& e)
    {
        std::cerr << "Client error: " << e.what() << std::endl;
    }


    server_io_context.stop();

    server_thread.join();

    return 0;

}
