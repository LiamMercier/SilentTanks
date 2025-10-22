#pragma once

#include <filesystem>
#include <boost/asio/ssl.hpp>

struct ServerIdentity
{
    std::string address;
    uint16_t port;
    std::string display_hash;

    std::string get_identity_string()
    {
        return "["
               + address
               + "]:"
               + std::to_string(port)
               + ":"
               + display_hash;
    }

    bool try_parse_identity_string(std::string input);

    bool try_parse_endpoint(std::string input);
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
