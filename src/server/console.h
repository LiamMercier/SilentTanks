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

#include "generic-constants.h"

#include <utility>
#include <array>
#include <boost/asio.hpp>

enum class LogLevel : uint8_t
{
    INFO,
    WARN,
    ERROR,
    CONSOLE
};

constexpr size_t NUMBER_OF_LOG_LEVELS = static_cast<size_t>(LogLevel::CONSOLE) + 1;

constexpr std::array<std::string_view, NUMBER_OF_LOG_LEVELS> log_prefix = []
{
    std::array<std::string_view, NUMBER_OF_LOG_LEVELS> a{};

    a[static_cast<size_t>(LogLevel::INFO)] = "\033[32m[INFO]:\033[0m    ";
    a[static_cast<size_t>(LogLevel::WARN)] = "\033[33m[WARN]:\033[0m    ";
    a[static_cast<size_t>(LogLevel::ERROR)] = "\033[31m[ERROR]:\033[0m   ";
    a[static_cast<size_t>(LogLevel::CONSOLE)] = "\033[36m[CONSOLE]:\033[0m ";

    return a;
}();

// For use in main.
void restore_terminal();

namespace asio = boost::asio;

class Console
{
public:
    using CommandHandler = std::function<void(std::string)>;

    static Console & instance();

    void set_command_handler(CommandHandler h);

    void start_command_loop();

    void stop_command_loop();

    static void init(asio::io_context & io_context, LogLevel level);

    void log(std::string msg, LogLevel level);

private:
    explicit Console(asio::io_context & io);

    Console(const Console &) = delete;
    Console & operator=(const Console &) = delete;

    void do_log(std::string msg, LogLevel level);

private:
    // Main strand to serialize console logs.
    asio::strand<asio::io_context::executor_type> strand_;

    // Context pointer for setup.
    static inline asio::io_context * io_context_ = nullptr;

    static inline LogLevel level_;

    // Thread state data.
    std::thread reader_thread_;
    std::atomic<bool> running_;
    CommandHandler cmd_handler_;

    // User input buffer.
    std::mutex buffer_mutex_;
    std::string current_input_buffer_;
};
