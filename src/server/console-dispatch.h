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

#pragma once

#include <string>
#include <sstream>

#include "server.h"
#include "console.h"

inline void console_dispatch(Server & server, std::string line)
{
    std::istringstream iss(line);
    std::string cmd;

    if(!(iss >> cmd))
    {
        return;
    }

    // Strip prefix "-" or "--" around a command.
    if (cmd.size() >= 2 && cmd[0] == '-')
    {
        if (cmd[1] == '-')
        {
            cmd.erase(0, 2);
        }
        else
        {
            cmd.erase(0, 1);
        }
    }

    // To lower.
    std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                   [](unsigned char c){
                       return std::tolower(c);
                    });

    if (cmd == "banuser")
    {
        std::string username;
        int length;

        // Try to grab username and length
        if (!(iss >> username >> length) || length <= 0)
        {
            Console::instance().log(
                "Usage: BanUser <username> <duration (minutes)> <reason>",
                LogLevel::CONSOLE
            );
            return;
        }

        // Get the rest of the line for the reason.
        std::string reason;
        std::getline(iss, reason);

        // Strip initial space.
        if (!reason.empty() && reason[0] == ' ')
        {
            reason.erase(0, 1);
        }

        // Create the timepoint.
        using namespace std::chrono;
        system_clock::time_point banned_until = (system_clock::now()
                                                + minutes(length));

        server.CONSOLE_ban_user(username, banned_until, reason);
    }
    else if (cmd == "banip")
    {
        std::string ip;
        int length;

        // Try to grab username and length.
        if (!(iss >> ip >> length) || length <= 0)
        {
            Console::instance().log(
                "Usage: BanIP <ipv4_address> <duration (minutes)>",
                LogLevel::CONSOLE
            );
            return;
        }

        // Check that the IP is valid.
        boost::system::error_code ec;
        asio::ip::make_address_v4(ip, ec);
        if (ec)
        {
            Console::instance().log(
                "IP " + ip + " is not valid.",
                LogLevel::CONSOLE
            );
            return;
        }

        // Create the timepoint.
        using namespace std::chrono;
        system_clock::time_point banned_until = (system_clock::now()
                                                + minutes(length));

        server.CONSOLE_ban_ip(ip, banned_until);
    }
    else if (cmd == "help")
    {
        Console::instance().log(
                "Available commands: \n"
                "           ShowIdentity\n"
                "           BanUser <username> <duration (minutes)> <reason>\n"
                "           BanIP <ipv4_address> <duration (minutes)>\n"
                "           Shutdown",
                LogLevel::CONSOLE
            );
    }
    else if (cmd == "shutdown")
    {
        Console::instance().log(
                "Shutting down server, please wait.",
                LogLevel::CONSOLE
            );
        server.shutdown();
    }
    else if (cmd == "showidentity")
    {
        std::string identity_line = server.get_identity_string();
        Console::instance().log(
                identity_line,
                LogLevel::CONSOLE
            );
    }
    else
    {
        Console::instance().log("Console command: "
                                + cmd
                                + " is unrecognized, try help, -help, or --help",
                                LogLevel::CONSOLE);
    }

}
