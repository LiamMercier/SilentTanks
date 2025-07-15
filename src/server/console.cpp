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
:strand_(io.get_executor())
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
