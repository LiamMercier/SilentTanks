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

#include "console.h"

#include <iostream>

// ANSI console setup for windows
#ifdef _WIN32
#include <windows.h>
void enable_ansi_windows()
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(h, &mode);
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(h, mode);
}
#else
void enable_ansi_windows()
{

}
#endif

// Disable buffering on linux
#ifndef _WIN32
#include <termios.h>
#include <unistd.h>

static struct termios g_original_termios;

void restore_terminal()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &g_original_termios);
}

struct TermGuard {
    TermGuard()
    {
        tcgetattr(STDIN_FILENO, &g_original_termios);
        termios raw = g_original_termios;
        raw.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }
    ~TermGuard()
    {
        restore_terminal();
    }
};
#else
// Nothing on windows
struct TermGuard { TermGuard(){} ~TermGuard(){} };
#endif

Console & Console::instance()
{
    static Console * inst = nullptr;

    // Thread safe initialization under the assumption that init is
    // called before instance.
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        if (!io_context_)
        {
            throw std::runtime_error("Console not initialized!");
        }
        inst = new Console(*io_context_);
    });

    return *inst;
}

void Console::set_command_handler(CommandHandler h)
{
    cmd_handler_ = std::move(h);
}

void Console::start_command_loop()
{
    if (running_)
    {
        return;
    }
    running_ = true;

    reader_thread_ = std::thread([this]()
    {
        // Terminal settings to prevent tearing.
        TermGuard tg;
        enable_ansi_windows();

        // Grab lines and push them to the command handler for
        // parsing commands.
        std::string buffer;
        while (running_)
        {
            int ch = std::cin.get();
            std::lock_guard<std::mutex> lock(buffer_mutex_);

            // Termination.
            if (ch == EOF || ch == '\x04')
            {
                running_ = false;
                break;
            }

            // User pressed enter.
            if (ch == '\r' || ch == '\n')
            {
                std::cout << "\r\n" << std::flush;
                std::string line = std::move(current_input_buffer_);
                current_input_buffer_.clear();

                // Now give the full line over.
                boost::asio::post(strand_, [this, l = std::move(line)]()
                {
                    try
                    {
                        if (cmd_handler_)
                        {
                            cmd_handler_(l);
                        }
                        else
                        {
                            std::string lmsg = "cmd_handler_ not set";
                            Console::instance().log(lmsg, LogLevel::ERROR);
                        }
                    }
                    catch (std::exception & e)
                    {
                        std::string lmsg = std::string("Command threw: ")
                                            + e.what();
                        Console::instance().log(lmsg, LogLevel::ERROR);
                    }
                    catch (...)
                    {
                        std::string lmsg = std::string("Command threw "
                                            "nonstandard exception");
                        Console::instance().log(lmsg, LogLevel::ERROR);
                    }
                });
            }
            // Backspace was pressed
            else if (ch == 127 || ch == '\b')
            {
                // Delete if not empty
                if (!current_input_buffer_.empty())
                {
                    current_input_buffer_.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            }
            // Anything else.
            else
            {
                current_input_buffer_.push_back(char(ch));
                std::cout << char(ch) << std::flush;
            }
        }

        log("Console input was closed.", LogLevel::WARN);
    });
}

void Console::stop_command_loop()
{
    running_ = false;

    if (reader_thread_.joinable())
    {
        reader_thread_.join();
    }
}

void Console::init(asio::io_context & io_context, LogLevel level)
{
    io_context_ = &io_context;
    level_ = level;
}

void Console::log(std::string msg, LogLevel level)
{
    if (level < level_)
    {
        return;
    }

    asio::post(strand_,
        [self = this, msg = std::move(msg), level]()
    {
        self->do_log(std::move(msg), level);
    });
}

Console::Console(asio::io_context & io)
:strand_(io.get_executor()),
running_(false)
{

}

void Console::do_log(std::string msg, LogLevel level)
{
    // Grab a copy of the current buffer
    std::string snapshot;
    {
        // Lock only for enough time to grab this data.
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        snapshot = current_input_buffer_;
    }

    // Clear the line.
    std::clog << "\r\033[2K";

    std::clog << log_prefix[static_cast<size_t>(level)] << msg << "\n";

    // Redraw the buffer.
    std::clog << "\033[1;34m[USER]:\033[0m    " << snapshot << std::flush;
}
