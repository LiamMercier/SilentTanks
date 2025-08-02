#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <signal.h>

#include <utility>
#include <boost/asio.hpp>

#include "client.h"

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
