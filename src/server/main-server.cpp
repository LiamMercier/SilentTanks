#include <string>
#include <iostream>
#include <thread>

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
        std::cout << "libsodium failed to initialize \n";
        return 1;
    }

    asio::io_context server_io_context;

    Console::init(server_io_context, LogLevel::INFO);
    Console::instance().log("Test error text", LogLevel::ERROR);

    asio::ip::tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 12345);

    Server server(server_io_context, endpoint);

    std::cout << "Server started on " << endpoint << std::endl;

    // Bind the line parsing handle for the console to use.
    auto cmd_handler = std::bind(&console_dispatch,
                                 std::ref(server),
                                 std::placeholders::_1);

    Console::instance().set_command_handler(cmd_handler);
    Console::instance().start_command_loop();

    std::thread server_thread([&]()
    {
        server_io_context.run();
    });

    // Some early console tests
    Console::instance().log("Text should be replaced in the CLI", LogLevel::WARN);

    std::this_thread::sleep_for(std::chrono::milliseconds(750));

    Console::instance().log("Text should be replaced in the CLI", LogLevel::WARN);

    std::this_thread::sleep_for(std::chrono::milliseconds(750));

    Console::instance().log("Text should be replaced in the CLI", LogLevel::WARN);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Console::instance().log("Text should be replaced in the CLI", LogLevel::WARN);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Console::instance().log("Text should be replaced in the CLI", LogLevel::WARN);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Console::instance().log("Testing result of elo changes for two 1500 elo players. Expected result is 1484, 1516.", LogLevel::INFO);

    {
        std::vector<int> init_elos_1 = {1500, 1500};
        std::vector<uint8_t> placement_1 = {0, 1};

        std::vector<int> new_elos = elo_updates(init_elos_1, placement_1);

        std::string lmsg = std::to_string(new_elos[0]) + " " + std::to_string(new_elos[1]);
        Console::instance().log(lmsg, LogLevel::INFO);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    Console::instance().log("Testing result of elo changes for three 1500 elo players. Expected result is 1525, 1500, 1475.", LogLevel::INFO);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        std::vector<int> init_elos_1 = {1500, 1500, 1500};
        std::vector<uint8_t> placement_1 = {2, 1, 0};

        std::vector<int> new_elos = elo_updates(init_elos_1, placement_1);

        std::string lmsg = std::to_string(new_elos[0]) + " " + std::to_string(new_elos[1]) + " " + std::to_string(new_elos[2]);
        Console::instance().log(lmsg, LogLevel::INFO);
    }

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

    if (m.header.type_ == HeaderType::Ping)
    {
        std::cout << "[" << s->id() << "] " << "Pinged by server \n";
        return;
    }

    if (m.header.type_ == HeaderType::PingTimeout)
    {
        std::cout << "[" << s->id() << "] " << "Ping Timed Out \n";
        return;
    }
    if (m.header.type_ == HeaderType::BadQueue)
    {
        std::cout << "[" << s->id() << "] " << "Bad queue \n";
        return;
    }
    if (m.header.type_ == HeaderType::MatchCreationError)
    {
        std::cout << "[" << s->id() << "] " << "Match creation error \n";
        return;
    }
    if (m.header.type_ == HeaderType::GameEnded)
    {
        std::cout << "[" << s->id() << "] " << "Match does not exist \n";
        return;
    }
    if (m.header.type_ == HeaderType::DirectTextMessage)
    {
        TextMessage dm = m.to_text_message();
        std::cout << "[" << s->id() << "] " << "Message from user " << dm.user_id << "\n" << dm.text << "\n";
        return;
    }

    if (m.header.type_ == HeaderType::MatchTextMessage)
    {
        InternalMatchMessage msg = m.to_match_message();
        std::cout << "[" << s->id() << "] " << "Match message from: " << msg.user_id << "\n" << msg.sender_username << "\n" << msg.text << "\n";
        return;
    }

    if (m.header.type_ == HeaderType::NotifyFriendAdded)
    {
        ExternalUser new_friend = m.to_user();
        std::cout << "[" << s->id() << "] " << "Added friend: " << new_friend.user_id << "\n" << new_friend.username << "\n";
        return;
    }

    if (m.header.type_ == HeaderType::NotifyFriendRemoved)
    {
        ExternalUser new_friend = m.to_user();
        std::cout << "[" << s->id() << "] " << "Removed friend: " << new_friend.user_id << "\n" << new_friend.username << "\n";
        return;
    }

    if (m.header.type_ == HeaderType::NotifyBlocked)
    {
        ExternalUser new_friend = m.to_user();
        std::cout << "[" << s->id() << "] " << "Blocked user: " << new_friend.user_id << "\n" << new_friend.username << "\n";
        return;
    }

    if (m.header.type_ == HeaderType::NotifyUnblocked)
    {
        ExternalUser new_friend = m.to_user();
        std::cout << "[" << s->id() << "] " << "Unblocked user: " << new_friend.user_id << "\n" << new_friend.username << "\n";
        return;
    }

    if (m.header.type_ == HeaderType::Banned)
    {
        BanMessage banned = m.to_ban_message();
        if (banned.time_till_unban == std::chrono::system_clock::time_point::min())
        {
            std::cout << "ban msg failed\n";
        }

        std::cout << "[" << s->id() << "] " << "Banned by server until: ";

        std::time_t time = std::chrono::system_clock::to_time_t(banned.time_till_unban);
        std::tm tm = *std::localtime(&time);
        std::cout << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

        std::cout << "\n";

        std::cout << "    Reason: " << banned.reason << "\n";

        return;
    }

    if (m.header.type_ == HeaderType::NoMatchFound)
    {
        std::cout << "[" << s->id() << "] " << "match not found \n";
        return;
    }

    if (m.header.type_ == HeaderType::BadAuth)
    {
        std::cout << "[" << s->id() << "] " << "Bad auth! \n";
        return;
    }

    if (m.header.type_ == HeaderType::GoodAuth)
    {
        std::cout << "[" << s->id() << "] " << "Good auth! \n";
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

    LoginRequest u1_login;
    u1_login.username = u1_req.username;
    u1_login.hash = u1_req.hash;

    {
        Message msg;
        msg.create_serialized(u1_req);
        //s1->deliver(msg);
    }

    {
        Message msg;
        msg.create_serialized(u1_login);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));


    Message msg;
    msg.create_serialized(QueueMatchRequest(GameMode::ClassicTwoPlayer));

    s1->deliver(msg);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        RegisterRequest u2_req;
        std::string u2_pass = "cannon";
        u2_req.username = "bananas";

        int result2 = argon2id_hash_raw(
                        ARGON2_TIME,
                        ARGON2_MEMORY,
                        ARGON2_PARALLEL,
                        reinterpret_cast<const uint8_t*>(u2_pass.data()),
                        u2_pass.size(),
                        salt.data(),
                        salt.size(),
                        u2_req.hash.data(),
                        u2_req.hash.size());

        if (result2 != ARGON2_OK)
        {
            std::cout << "Failed to hash in main server \n";
            return 1;
        }

        LoginRequest u2_login;
        u2_login.username = u2_req.username;
        u2_login.hash = u2_req.hash;

        Message msg;
        msg.create_serialized(u2_req);
        //s2->deliver(msg);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        Message msg2;
        msg2.create_serialized(u2_login);
        s2->deliver(msg2);

    }

    // Test console ban
    /*
    server.CONSOLE_ban_user("oranges",
                            std::chrono::system_clock::now() + std::chrono::seconds(30),
                            std::string("test ban console")
                            );

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    server.CONSOLE_ban_user("bananas",
                            std::chrono::system_clock::now() + std::chrono::seconds(30),
                            std::string("test ban console")
                            );
    */

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Send friend request from bananas to apples (does not exist).
    {
        Message msg;
        FriendRequest request;
        request.username = "APPLES";
        msg.create_serialized(request);

        s2->deliver(msg);
    }

    // Send friend request from bananas to oranges.
    {
        Message msg;
        FriendRequest request;
        request.username = "ORANGES";
        msg.create_serialized(request);

        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Accept the friend request
    {
        Message msg;
        FriendDecision request;
        boost::uuids::string_generator gen;
        request.user_id = gen("b98c81fe-1da8-4d95-b823-7f65d660fd6c");
        request.decision = true;

        msg.create_serialized(request);

        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // send msg from s1 to s2
    {
        Message msg;
        TextMessage dm;
        boost::uuids::string_generator gen;
        dm.user_id = gen("b98c81fe-1da8-4d95-b823-7f65d660fd6c");
        dm.text = "Direct message from s1 (oranges) to s2 (bananas)";
        msg.header.type_ = HeaderType::DirectTextMessage;
        msg.create_serialized(dm);
        s1->deliver(msg);
    }

    // send msg from s2 to s1
    {
        Message msg;
        TextMessage dm;
        boost::uuids::string_generator gen;
        dm.user_id = gen("d96fc731-ecc6-4e77-a113-da914302fa30");
        dm.text = "Direct message from s2 (bananas) to s1 (oranges)";
        msg.header.type_ = HeaderType::DirectTextMessage;
        msg.create_serialized(dm);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        Message msg;
        TextMessage dm;
        boost::uuids::string_generator gen;
        dm.user_id = gen("d96fc731-ecc6-4e77-a113-da914302fa30");
        dm.text = "s2 is going to unfriend s1";
        msg.header.type_ = HeaderType::DirectTextMessage;
        msg.create_serialized(dm);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Unfriend the user from s2's side.
    {
        Message msg;
        UnfriendRequest request;
        boost::uuids::string_generator gen;
        request.user_id = gen("d96fc731-ecc6-4e77-a113-da914302fa30");
        msg.create_serialized(request);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        Message msg;
        TextMessage dm;
        boost::uuids::string_generator gen;
        dm.user_id = gen("b98c81fe-1da8-4d95-b823-7f65d660fd6c");
        dm.text = "s1 should not be able to send this to s2, but can you see it? Hello there.";
        msg.header.type_ = HeaderType::DirectTextMessage;
        msg.create_serialized(dm);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Should fail as we are already queued.
    {
        Message msg;
        msg.create_serialized(QueueMatchRequest(GameMode::RankedTwoPlayer));

        s1->deliver(msg);
    }

    // Cancel the casual queue.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        Message msg;
        msg.create_serialized(CancelMatchRequest(GameMode::ClassicTwoPlayer));

        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        Message msg;
        msg.create_serialized(QueueMatchRequest(GameMode::RankedTwoPlayer));

        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        Message msg2;
        msg2.create_serialized(QueueMatchRequest(GameMode::RankedTwoPlayer));

        s2->deliver(msg2);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    Console::instance().log("Waiting for tick timer.", LogLevel::WARN);

    std::this_thread::sleep_for(std::chrono::milliseconds(45000));

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        Message msg;
        TextMessage mm;
        mm.text = "Lets have a good match!";
        msg.header.type_ = HeaderType::MatchTextMessage;
        msg.create_serialized(mm);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        Message msg;
        TextMessage mm;
        mm.text = "Good luck!";
        msg.header.type_ = HeaderType::MatchTextMessage;
        msg.create_serialized(mm);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Test blocking mid game and see that it doesn't prevent messages.
    {
        Message msg;
        BlockRequest request;
        request.username = "BANANAS";
        msg.create_serialized(request);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        Message msg;
        TextMessage mm;
        mm.text = "You should see this, but I blocked you.";
        msg.header.type_ = HeaderType::MatchTextMessage;
        msg.create_serialized(mm);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        Message msg;
        TextMessage mm;
        mm.text = "Message should not be shown, is it?";
        msg.header.type_ = HeaderType::MatchTextMessage;
        msg.create_serialized(mm);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Place tank for player 1.
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

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Try to make a bad placement.
    {
        Message msg;
        Command c;

        c.sender = 1;
        c.tank_id = 0;
        c.type = CommandType::Place;
        c.payload_first = 17;
        c.payload_second = 21;
        c.sequence_number = 2;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Bad placement due to mask.
    {
        Message msg;
        Command c;

        c.sender = 1;
        c.tank_id = 1;
        c.type = CommandType::Place;
        c.payload_first = 5;
        c.payload_second = 5;
        c.sequence_number = 2;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Place tank for p2.
    {
        Message msg;
        Command c;

        c.sender = 1;
        c.tank_id = 1;
        c.type = CommandType::Place;
        c.payload_first = 6;
        c.payload_second = 5;
        c.sequence_number = 2;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Place tank for p1.
    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 2;
        c.type = CommandType::Place;
        c.payload_first = 6;
        c.payload_second = 5;
        c.sequence_number = 2;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Forfeit the game.
    {
        Message msg;
        msg.create_serialized(HeaderType::ForfeitMatch);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Queue up again, ensure we can still re-use the map.
    {
        Message msg;
        msg.create_serialized(QueueMatchRequest(GameMode::ClassicTwoPlayer));
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        Message msg;
        msg.create_serialized(QueueMatchRequest(GameMode::ClassicTwoPlayer));
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        Message msg;
        TextMessage mm;
        mm.text = "Message should not be shown still, is it????";
        msg.header.type_ = HeaderType::MatchTextMessage;
        msg.create_serialized(mm);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Place tank for player 1.
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

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Place tank for player 2.
    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Place;
        c.payload_first = 6;
        c.payload_second = 5;
        c.sequence_number = 3;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Place tank for player 1.
    {
        Message msg;
        Command c;

        c.sender = 0;
        c.tank_id = 1;
        c.type = CommandType::Place;
        c.payload_first = 3;
        c.payload_second = 3;
        c.sequence_number = 4;

        msg.create_serialized(c);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Place tank for player 2.
    {
        Message msg;
        Command c;

        c.sender = 1;
        c.tank_id = 3;
        c.type = CommandType::Place;
        c.payload_first = 7;
        c.payload_second = 7;
        c.sequence_number = 5;

        msg.create_serialized(c);
        s2->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Forfeit the game.
    {
        Message msg;
        msg.create_serialized(HeaderType::ForfeitMatch);
        s1->deliver(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    client_io.stop();

    io_thread.join();

    }

    catch(const boost::system::system_error& e)
    {
        std::cerr << "Client error: " << e.what() << std::endl;
    }

    // Clean up system resources

    server_io_context.stop();

    server_thread.join();

    std::cerr << "Server stopped. Press anything to return.\n";

    Console::instance().stop_command_loop();

    return 0;

}
