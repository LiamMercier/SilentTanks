#include "server-identity.h"

namespace asio = boost::asio;

constexpr int hex_char_to_value(char c)
{
    if (c >= '0' && c <= '9')
    {
        return static_cast<unsigned char>(c - '0');
    }
    else if (c >= 'a' && c <= 'f')
    {
        return static_cast<unsigned char>(10 + (c - 'a'));
    }
    else if (c >= 'A' && c <= 'F')
    {
        return static_cast<unsigned char>(10 + (c - 'A'));
    }
    else
    {
        return -1;
    }
}

// Utility to convert the user's hexademical string to bytes.
std::vector<unsigned char> hex_string_to_bytes(std::string user_hex)
{
    // Remove leading "0x" if present.
    size_t index = 0;

    if (user_hex.size() >= 2
        && user_hex[0] == '0'
        && (user_hex[1] == 'x' || user_hex[1] == 'X'))
    {
        index = 2;
    }

    // We expect twice as many characters to represent the bytes,
    // since "FF" fits in one byte but takes two bytes of characters.
    if (user_hex.size() - index != SHA256_DIGEST_LENGTH * 2)
    {
        return {};
    }

    std::vector<unsigned char> bytes;
    bytes.reserve(SHA256_DIGEST_LENGTH);

    // Step by 2 each time.
    for (; index < user_hex.size(); index += 2)
    {
        int high = hex_char_to_value(user_hex[index]);
        int low = hex_char_to_value(user_hex[index + 1]);

        if (high < 0 || low < 0)
        {
            return {};
        }

        // Shifting over by 4 lets us avoid computing high and low differently.
        bytes.push_back(static_cast<unsigned char>((high << 4) | low));
    }

    return bytes;
}

std::string bytes_to_hex_string(const unsigned char * buffer,
                                size_t length)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (size_t i = 0; i < length; i++)
    {
        oss << std::setw(2) << static_cast<int>(buffer[i]);
    }

    return oss.str();
}

// Helper to fingerprint certificate public keys.
bool fingerprint_public_key(X509* cert, unsigned char out[SHA256_DIGEST_LENGTH])
{
    if (!cert || !out)
    {
        return false;
    }

    EVP_PKEY* public_key = X509_get_pubkey(cert);

    if (!public_key)
    {
        return false;
    }

    // Convert public key to DER (i2d_PUBKEY) into a buffer
    //
    // First we grab the length.
    int len = i2d_PUBKEY(public_key, nullptr);

    if (len <= 0)
    {
        EVP_PKEY_free(public_key);
        return false;
    }

    unsigned char* buf = static_cast<unsigned char*>(OPENSSL_malloc(len));

    if (!buf)
    {
        EVP_PKEY_free(public_key);
        return false;
    }

    // Read public key
    unsigned char* p = buf;
    int length_pubkey = i2d_PUBKEY(public_key, &p);

    if(!SHA256(buf, length_pubkey, out))
    {
        OPENSSL_free(buf);
        EVP_PKEY_free(public_key);
        return false;
    }

    OPENSSL_free(buf);
    EVP_PKEY_free(public_key);
    return true;
}

bool fill_server_fingerprint(asio::ssl::context & ssl_context,
                             std::string & out_hex)
{
    SSL_CTX* cntx = ssl_context.native_handle();
    if (!cntx)
    {
        return false;
    }

    // Does not need to be freed after.
    X509* cert = SSL_CTX_get0_certificate(cntx);

    if (!cert)
    {
        return false;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    bool success = fingerprint_public_key(cert, hash);

    if (!success)
    {
        return false;
    }

    // If we get here we have a good sha256 hash of the public key, turn it into
    // a hex string.
    out_hex = bytes_to_hex_string(hash, SHA256_DIGEST_LENGTH);

    return success;
}
