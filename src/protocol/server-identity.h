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

#include <filesystem>
#include <boost/asio/ssl.hpp>

struct ServerIdentity
{
    std::string name;
    std::string address;
    uint16_t port;

    // SHA256 hash bytes in human readable format (lowercase). All zero
    // in the server list file if none was given (i.e for CA signed certificates).
    std::string display_hash;

    std::string get_identity_string() const
    {
        return "["
               + address
               + "]:"
               + std::to_string(port)
               + ":"
               + display_hash;
    }

    std::string get_hashmap_string() const
    {
        return address
               + ":"
               + std::to_string(port)
               + ":"
               + display_hash;
    }

    // Append the name to the connection related data.
    std::string get_file_line_string() const
    {
        return "{"
               + name
               + "}:"
               + get_identity_string();
    }

    bool try_parse_identity_string(std::string input);

    bool try_parse_endpoint(std::string input);

    bool try_parse_list_line(std::string input);

    static constexpr size_t EXPECTED_HASH_LENGTH = static_cast<size_t>(
                                                        SHA256_DIGEST_LENGTH * 2);
};

constexpr int hex_char_to_value(char c);

// Utility to convert the user's hexademical string to bytes.
std::vector<unsigned char> hex_string_to_bytes(std::string user_hex);

std::string bytes_to_hex_string(const unsigned char * buffer,
                                size_t length);

// Helper to fingerprint certificate public keys.
bool fingerprint_public_key(X509* cert, unsigned char out[SHA256_DIGEST_LENGTH]);

bool fill_server_fingerprint(boost::asio::ssl::context & ssl_context,
                             std::string & out_hex);
