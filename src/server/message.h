#pragma once

#include "header.h"

#include <type_traits>
#include <vector>

enum class GameMode : uint8_t
{
    ClassicTwoPlayer = 0,
    RankedTwoPlayer,
    NO_MODE
};

struct QueueMatchRequest
{
    QueueMatchRequest(GameMode input_mode)
    :mode(input_mode)
    {
    }

    GameMode mode;
};

struct CancelMatchRequest
{
    CancelMatchRequest(GameMode input_mode)
    :mode(input_mode)
    {
    }

    GameMode mode;
};

// TODO: login request format when feature is to be added
struct LoginRequest
{
    uint64_t temp;
};

struct Message
{
public:
    template <typename mType>
    void create_serialized(const mType & req);

    bool valid_matching_command() const;

public:
    Header header;
    std::vector<uint8_t> payload;
};
