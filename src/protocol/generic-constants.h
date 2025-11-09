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
