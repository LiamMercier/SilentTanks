#pragma once

constexpr size_t HASH_LENGTH = 32;
constexpr size_t SALT_LENGTH = 16;

constexpr uint32_t ARGON2_TIME = 4;
constexpr uint32_t ARGON2_MEMORY = 65536;
constexpr uint32_t ARGON2_PARALLEL = 1;

// Embedded into the client for hashing before
// passwords are sent to the server. Generated randomly
// with libsodium's randombytes_buf function on linux.
//
// The salt only provides protection in the sense that it
// forces network attackers on the server to precompute a table
// specifically for SilentTanks clients.
const std::array<uint8_t, SALT_LENGTH> GLOBAL_CLIENT_SALT =
{
    0x71, 0x3B, 0xD4, 0x5B,
    0xF0, 0xA3, 0x19, 0x70,
    0xE2, 0xDB, 0xD7, 0xF8,
    0x1B, 0x2B, 0x84, 0xEA
};
