#pragma once

#include <vector>
#include <cstdint>

// Default elo, must match create_tables.sql default if changed.
constexpr int DEFAULT_ELO = 1500;

constexpr int K_TWO_PLAYERS = 32;

constexpr int ELO_FLOOR = 500;

constexpr double ELO_DIFF_UPPER = 7.0;

constexpr double ELO_DIFF_LOWER = -7.0;

std::vector<int> elo_updates(const std::vector<int> & initial_elos,
                             const std::vector<uint8_t> & placement);
