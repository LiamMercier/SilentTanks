#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <signal.h>

#include <utility>
#include <boost/asio.hpp>

#include "client.h"

#include <boost/uuid/string_generator.hpp>

int main()
{
    try
    {
        auto thread_count = std::thread::hardware_concurrency();

        asio::io_context io_context;
        auto work_guard = asio::make_work_guard(io_context);

        asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait(
            [&](auto const & ec, int i){
            std::cerr << "\n" << "Client signaled for shutdown.\n";
            work_guard.reset();
            io_context.stop();
        });

        Client client(io_context);

        if (thread_count == 0)
        {
            thread_count = 2;
        }

        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (unsigned int i = 0; i < thread_count; i++)
        {
            threads.emplace_back([&]{
                io_context.run();
            });
        }

        // Tests
        client.connect("127.0.0.1:12345");

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        std::string username;
        std::cin >> username;

        // client.register_account(username, "123");

        client.login(username, "123");

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        client.request_user_list(UserListType::Friends);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        client.request_user_list(UserListType::FriendRequests);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        client.request_user_list(UserListType::Blocks);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        client.send_friend_request("chungus");

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        /*
        {
            std::string uuid_str = "3b7a2f4b-4d1e-4332-8b99-a57845098a13";
            boost::uuids::string_generator gen;
            boost::uuids::uuid user_id = gen(uuid_str);
            client.respond_friend_request(user_id, ACCEPT_FRIEND_REQUEST);
        }
        */

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        /*
        {
            std::string username = "lobster";
            client.send_block_request(username);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        {
            std::string uuid_str = "16071a46-2856-4ba6-9c24-17732c1d8378";
            boost::uuids::string_generator gen;
            boost::uuids::uuid user_id = gen(uuid_str);
            client.send_unblock_request(user_id);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        {
            std::string uuid_str = "3b7a2f4b-4d1e-4332-8b99-a57845098a13";
            boost::uuids::string_generator gen;
            boost::uuids::uuid user_id = gen(uuid_str);
            client.send_unfriend_request(user_id);
        }
        */

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        client.queue_request(GameMode::ClassicTwoPlayer);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Once we stop the context, join our threads back
        for (auto & thread : threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }

    }
    catch (std::exception & e)
    {
        std::cerr << "Fatal error starting client: " << e.what() << "\n";
        return 1;
    }
}
