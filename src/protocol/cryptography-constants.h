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
