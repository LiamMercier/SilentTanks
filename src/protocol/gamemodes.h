#pragma once

// This must be ordered so that all ranked modes are together.
enum class GameMode : uint8_t
{
    ClassicTwoPlayer = 0,
    RankedTwoPlayer,
    NO_MODE
};

// First GameMode which is ranked, for fast checks of a game mode.
constexpr uint8_t RANKED_MODES_START = static_cast<uint8_t>(GameMode::RankedTwoPlayer);

constexpr uint8_t NUMBER_OF_MODES = static_cast<uint8_t>(GameMode::NO_MODE);

constexpr uint8_t RANKED_MODES_COUNT = static_cast<uint8_t>(GameMode::NO_MODE)
                                       - RANKED_MODES_START;

// Helpers to get the index for elo vectors.
inline uint8_t elo_ranked_index(GameMode m)
{
    uint8_t idx = static_cast<uint8_t>(m);
    return idx - RANKED_MODES_START;
}

inline uint8_t elo_ranked_index(uint8_t m)
{
    return m - RANKED_MODES_START;
}
