#pragma once

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

    a[static_cast<size_t>(LogLevel::INFO)] = "[INFO]: ";
    a[static_cast<size_t>(LogLevel::WARN)] = "[WARN]: ";
    a[static_cast<size_t>(LogLevel::ERROR)] = "[ERROR]: ";
    a[static_cast<size_t>(LogLevel::CONSOLE)] = "[CONSOLE]: ";

    return a;
}();

namespace asio = boost::asio;

// TODO: async commands from admin.
class Console
{
public:
    static Console & instance();

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
};
