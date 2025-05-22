#include <string>
#include <iostream>
#include <thread>

#include "server.h"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <sodium.h>

int main()
{
    namespace asio = boost::asio;

    // basic test code

    if (sodium_init() == -1) {
        std::cout << "libsodium failed to initialize \n";
        return 1;
    }

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
    asio::ip::tcp::socket client_sock2(client_io);

    const static std::string colors_array[4] = {"\033[48;5;196m", "\033[48;5;21m", "\033[48;5;46m", "\033[48;5;226m"};

    try {
    client_sock.connect(endpoint);
    client_sock2.connect(endpoint);

    auto s1 = std::make_shared<Session>(client_io, 6);
    auto s2 = std::make_shared<Session>(client_io, 7);

    s1->socket() = std::move(client_sock);
    s2->socket() = std::move(client_sock2);

    auto callbk = [](const Session::ptr & s, Message m){

    if (m.header.type_ == HeaderType::NoMatchFound)
    {
        std::cout << "[" << s->id() << "] " << "match not found \n";
        return;
    }

    if (m.header.type_ == HeaderType::FailedMove)
    {
        std::cout << "[" << s->id() << "] " << "Move failed \n";
        return;
    }

    if (m.header.type_ == HeaderType::StaleMove)
    {
        std::cout << "[" << s->id() << "] " << "Stale move \n";
        return;
    }

    if (m.header.type_ == HeaderType::PlayerView)
    {
        bool success = false;
        PlayerView p1 = m.to_player_view(success);

        if (success == false)
        {
            std::cout << "[" << s->id() << "] " << "Failed to get player view\n";
            return;
        }

        std::cout << "[" << s->id() << "] " << "View for session " << "\n";

        for (int y = 0; y < p1.map_view.get_height(); y++)
        {
            for (int x = 0; x < p1.map_view.get_width(); x++)
            {
                GridCell curr = p1.map_view[p1.map_view.idx(x,y)];

                if (curr.visible_ == true)
                {
                    uint8_t occ = curr.occupant_;

                    if (occ == NO_OCCUPANT)
                    {
                        std::cout << "\033[48;5;184m" << "_" << "\033[0m ";
                    }
                    else
                    {
                        auto this_tank = std::find_if(
                            p1.visible_tanks.begin(), p1.visible_tanks.end(),
                            [&](auto const & obj){return obj.id_ == occ;});

                        if (this_tank != p1.visible_tanks.end())
                        {
                            std::cout << colors_array[this_tank->owner_] << +this_tank->id_ << "\033[0m ";
                        }
                        else
                        {
                            std::cout << "\033[48;5;184m" << "_" << "\033[0m ";
                        }

                    }
                }
                else
                {
                    if (curr.type_ == CellType::Terrain)
                    {
                        std::cout << "\033[48;5;130m" << curr << "\033[0m ";
                    }
                    else
                    {
                        std::cout << "?" << " ";
                    }
                }
            }
            std::cout << "\n";
        }

        std::cout << "\n";

    }
    else if (m.header.type_ == HeaderType::MatchStarting)
    {
        std::cout << "[" << s->id() << "] " << "Match starting! pid: " << +m.payload[0] <<"\n";

    }
    else
    {
        return;
    }

    };

    s1->set_message_handler
                (callbk, nullptr);

    s2->set_message_handler
                (callbk, nullptr);

    std::cout << "clients connecting to server\n";

    s1->start();
    s2->start();

    std::thread io_thread([&]()
    {
        client_io.run();
    });

    RegisterRequest u1_req;
    std::string u1_pass = "apples";
    u1_req.username = "oranges";
    std::vector<uint8_t> salt;
    salt.push_back(1);
    salt.push_back(2);
    salt.push_back(3);
    salt.push_back(4);
    salt.push_back(5);
    salt.push_back(6);
    salt.push_back(7);
    salt.push_back(8);

    int result = argon2id_hash_raw(
                    ARGON2_TIME,
                    ARGON2_MEMORY,
                    ARGON2_PARALLEL,
                    reinterpret_cast<const uint8_t*>(u1_pass.data()),
                    u1_pass.size(),
                    salt.data(),
                    salt.size(),
                    u1_req.hash.data(),
                    u1_req.hash.size());

    if (result != ARGON2_OK)
    {
        std::cout << "Failed to hash in main server \n";
        return 1;
    }

    Message msg;
    msg.create_serialized(u1_req);
    s1->deliver(msg);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    /*
    Message msg;
    msg.create_serialized(QueueMatchRequest(GameMode::ClassicTwoPlayer));

    s1->deliver(msg);

    Message msg2;
    msg2.create_serialized(QueueMatchRequest(GameMode::ClassicTwoPlayer));

    s2->deliver(msg2);

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 0;
        c.type = CommandType::Place;
        c.payload_first = 1;
        c.payload_second = 3;
        c.sequence_number = 1;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 1;
        c.tank_id = 2;
        c.type = CommandType::Place;
        c.payload_first = 5;
        c.payload_second = 5;
        c.sequence_number = 2;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 0;
        c.type = CommandType::Place;
        c.payload_first = 5;
        c.payload_second = 6;
        c.sequence_number = 3;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 0;
        c.type = CommandType::Place;
        c.payload_first = 6;
        c.payload_second = 5;
        c.sequence_number = 3;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Fire;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 5;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Load;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 6;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Fire;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 7;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 2;
        c.type = CommandType::RotateBarrel;
        c.payload_first = 1;
        c.payload_second = 0;
        c.sequence_number = 7;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 2;
        c.type = CommandType::RotateTank;
        c.payload_first = 1;
        c.payload_second = 0;
        c.sequence_number = 8;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 2;
        c.type = CommandType::RotateBarrel;
        c.payload_first = 1;
        c.payload_second = 0;
        c.sequence_number = 9;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Load;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 10;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Fire;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 11;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::RotateBarrel;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 12;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 3;
        c.type = CommandType::RotateTank;
        c.payload_first = 1;
        c.payload_second = 0;
        c.sequence_number = 17;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 3;
        c.type = CommandType::RotateTank;
        c.payload_first = 1;
        c.payload_second = 0;
        c.sequence_number = 18;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 3;
        c.type = CommandType::RotateTank;
        c.payload_first = 1;
        c.payload_second = 0;
        c.sequence_number = 19;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Load;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 21;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Fire;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 22;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Load;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 23;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 3;
        c.type = CommandType::RotateTank;
        c.payload_first = 1;
        c.payload_second = 0;
        c.sequence_number = 20;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 3;
        c.type = CommandType::RotateTank;
        c.payload_first = 1;
        c.payload_second = 0;
        c.sequence_number = 21;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 3;
        c.type = CommandType::RotateTank;
        c.payload_first = 1;
        c.payload_second = 0;
        c.sequence_number = 22;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Fire;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 26;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Load;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 27;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Fire;
        c.payload_first = 0;
        c.payload_second = 0;
        c.sequence_number = 28;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    */

    client_io.stop();

    io_thread.join();

    }

    catch(const boost::system::system_error& e)
    {
        std::cerr << "Client error: " << e.what() << std::endl;
    }


    server_io_context.stop();

    server_thread.join();

    return 0;

}
