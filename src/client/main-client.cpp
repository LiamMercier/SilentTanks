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
            // Also shutdown the GUI.
            QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        });

        GUIClient client(io_context);

        // Setup engine and point at our GUI client wrapper.
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("Client", &client);

        // Setup user lists.
        engine.rootContext()->setContextProperty("FriendsModel",
                                                 client.friends_model());

        engine.rootContext()->setContextProperty("RequestsModel",
                                                 client.requests_model());

        engine.rootContext()->setContextProperty("BlockedModel",
                                                 client.blocked_model());

        engine.rootContext()->setContextProperty("MessagesModel",
                                                 client.messages_model());

        engine.rootContext()->setContextProperty("HistoryModel",
                                                 client.history_model());

        engine.rootContext()->setContextProperty("GameManager",
                                                 client.game_manager());

        engine.rootContext()->setContextProperty("ReplayManager",
                                                 client.replay_manager());

        engine.rootContext()->setContextProperty("PlayersModel",
                                                 client.players_model());

        engine.rootContext()->setContextProperty("ReplayPlayers",
                                                 client.replay_players());

        qmlRegisterUncreatableMetaObject(GUI::staticMetaObject,
                                         "GUICommon",
                                         1,
                                         0,
                                         "ClientState",
                                         "Tried to create GUI namespace enum.");

        qmlRegisterUncreatableMetaObject(GUI::staticMetaObject,
                                         "GUICommon",
                                         1,
                                         0,
                                         "PopupType",
                                         "Tried to create GUI namespace enum.");

        qmlRegisterUncreatableMetaObject(GUI::staticMetaObject,
                                         "GUICommon",
                                         1,
                                         0,
                                         "QueueType",
                                         "Tried to create GUI namespace enum.");

        qmlRegisterUncreatableMetaObject(GUI::staticMetaObject,
                                         "GUICommon",
                                         1,
                                         0,
                                         "SoundType",
                                         "Tried to create GUI namespace enum.");

        qmlRegisterSingletonType(QUrl("qrc:/SoundManager.qml"),
                                 "SoundManager",
                                 1,
                                 0,
                                 "SoundManager");

        // Load our main QML file.
        engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));

        qDebug() << "Enum keys:" << GUI::staticMetaObject.enumeratorCount();

        for (int i = 0; i < GUI::staticMetaObject.enumeratorCount(); ++i)
        {
            auto e = GUI::staticMetaObject.enumerator(i);
            qDebug() << "Enum: " << e.name();
        }

        // Ensure we can actually proceed with rendering.
        if (engine.rootObjects().isEmpty())
        {
            return 1;
        }

        // Setup our ASIO thread pool.
        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (unsigned int i = 0; i < thread_count; i++)
        {
            threads.emplace_back([&]{
                io_context.run();
            });
        }

        // Start the GUI, this thread is now being used.
        app.exec();

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
