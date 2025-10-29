#pragma once

#include <string>

constexpr std::string_view TERM_YELLOW = "\033[33m";

constexpr std::string_view TERM_RED = "\033[31m";

constexpr std::string_view TERM_RESET = "\033[0m";

constexpr int DEFAULT_SERVER_PORT = 49656;

// Default to localhost.
constexpr std::string_view DEFAULT_SERVER_ADDRESS = "127.0.0.1";

constexpr std::string_view DEFAULT_SERVER_NAME = "SilentTanks Server";

// By default our display hash should be all zeros if none exists.
inline std::string make_empty_hash_string(size_t size)
{
    return std::string(size, '0');
}
