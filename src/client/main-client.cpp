// Copyright (c) 2025 Liam Mercier
//
// This file is part of SilentTanks.
//
// SilentTanks is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License Version 3.0
// as published by the Free Software Foundation.
//
// SilentTanks is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License v3.0
// for more details.
//
// You should have received a copy of the GNU Affero General Public License v3.0
// along with SilentTanks. If not, see <https://www.gnu.org/licenses/agpl-3.0.txt>

#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <signal.h>

#include <utility>
#include <boost/asio.hpp>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQmlContext>
#include <QUrl>
#include <QStringLiteral>
#include <QDir>
#include <QIcon>

#ifdef _WIN32
#include <QQuickStyle>
#endif

#include "generic-constants.h"
#include "gui-client.h"
#include "asset-resolver.h"

#include <boost/uuid/string_generator.hpp>

int main(int argc, char* argv[])
{
    // Check all assets can be reached
    bool assets_exist = AppAssets::assert_client_assets_resolvable();

    if (!assets_exist)
    {
        std::cerr << TERM_RED
                  << "One or more required assets could not be found. Exiting.\n"
                  << TERM_RESET;
        return 1;
    }

    std::string server_list_filename = std::string(DEFAULT_SERVER_LIST_FILENAME);
    std::vector<ServerIdentity> server_list;
    bool list_read_success = read_server_list(server_list_filename,
                                              server_list);

    // If the server list could not be read, report to user.
    if (!list_read_success)
    {
        std::cerr << TERM_YELLOW
                  << "Server list is malformed. Some servers could not be read "
                  << "and will be removed if broken.\n"
                  << TERM_RESET;
    }

    try
    {
        QGuiApplication app(argc, argv);

        // Setup engine and set paths.
        QQmlApplicationEngine engine;

#ifdef _WIN32
        QQuickStyle::setStyle("Universal");
        engine.addImportPath(QDir(qApp->applicationDirPath()).filePath("qml"));
#else
        // set desktop filename on linux.
        app.setDesktopFileName("silent-tanks.desktop");
#endif

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
            QMetaObject::invokeMethod(qApp, "exit", Qt::QueuedConnection);
        });

        GUIClient client(io_context,
                         server_list,
                         std::move(server_list_filename));

        engine.rootContext()->setContextProperty("Client", &client);

        // Server list model for connection screen.
        engine.rootContext()->setContextProperty("ServerListModel",
                                                 client.server_list_model());

        // Setup user lists.
        engine.rootContext()->setContextProperty("FriendsModel",
                                                 client.friends_model());

        engine.rootContext()->setContextProperty("RequestsModel",
                                                 client.requests_model());

        engine.rootContext()->setContextProperty("BlockedModel",
                                                 client.blocked_model());

        // Message history, match history, game manager, replay manager, players for
        // the game, players for replay.
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

        // Ensure we can actually proceed with rendering.
        if (engine.rootObjects().isEmpty())
        {
            return 1;
        }

        // Set the icon.
        QIcon app_icon(QStringLiteral(":/icons/silent-tanks.ico"));

        // We can call the app to set the window from the api.
        app.setWindowIcon(app_icon);

        // Setup our ASIO thread pool.
        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        for (unsigned int i = 0; i < thread_count; i++)
        {
            threads.emplace_back([&]{
                io_context.run();
            });
        }

        QObject::connect(
            &client,
            &GUIClient::client_shutdown,
            &app,
            [&]{
                std::cerr << "GUI signaled for shutdown.\n";
                work_guard.reset();
                io_context.stop();
                // Also shutdown the GUI.
                QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
            },
            Qt::QueuedConnection);

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
        std::cerr << TERM_RED
                  << "Fatal error starting client: "
                  << e.what()
                  << "\n"
                  << TERM_RESET;
        return 1;
    }
}
