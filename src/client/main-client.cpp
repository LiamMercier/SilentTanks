#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <signal.h>

#include <utility>
#include <boost/asio.hpp>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>
#include <QStringLiteral>

#include "gui-client.h"

#include <boost/uuid/string_generator.hpp>

int main(int argc, char* argv[])
{
    try
    {
        QGuiApplication app(argc, argv);

        auto thread_count = std::thread::hardware_concurrency();

        if (thread_count == 0)
        {
            thread_count = 2;
        }

        asio::io_context io_context;
        auto work_guard = asio::make_work_guard(io_context);

        asio::signal_set signal_handler(io_context, SIGINT, SIGTERM);
        signal_handler.async_wait(
            [&](auto const & ec, int i){
            std::cerr << "\n" << "Client signaled for shutdown.\n";
            work_guard.reset();
            io_context.stop();
        });

        GUIClient client(io_context);

        // Setup engine and point at our GUI client wrapper.
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("Client", &client);

        // Load our main QML file.
        engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));

        // Ensure we can actually proceed with rendering.
        if (engine.rootObjects().isEmpty())
        {
            return 1;
        }

        app.exec();

        // Setup our ASIO thread pool.
        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (unsigned int i = 0; i < thread_count; i++)
        {
            threads.emplace_back([&]{
                io_context.run();
            });
        }

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
