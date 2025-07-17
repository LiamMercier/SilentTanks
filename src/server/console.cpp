#include "console.h"

#include <iostream>

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
        // Grab lines and push them to the command handler for
        // parsing commands.
        std::string line;
        while (running_ && std::getline(std::cin, line))
        {
            boost::asio::post(strand_, [this, l = std::move(line)]()
            {
                if (cmd_handler_)
                {
                    cmd_handler_(l);
                }
            });
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
    if (level >= LogLevel::ERROR)
    {
        std::clog << log_prefix[static_cast<size_t>(level)] << msg << std::endl;
    }
    else
    {
        std::clog << log_prefix[static_cast<size_t>(level)] << msg << "\n";
    }
}
