#pragma once

#include <glaze/glaze.hpp>
#include <cstdint>

enum class CommandType : uint8_t
{
    Move,
    RotateTank,
    RotateBarrel,
    Fire,
    Place,
    Load,
    NO_OP
};

// Command structure
struct Command
{
    uint8_t sender;
    CommandType type;
    // Doubles as "direction" when placed in setup phase.
    uint8_t tank_id;
    // Primary place to put a command parameter.
    uint8_t payload_first;
    // Mostly used to give a second x coordinate for placement.
    uint8_t payload_second;
    // Necessary since the client is asynchronous.
    uint16_t sequence_number;

    static constexpr std::size_t COMMAND_SIZE = 5 * sizeof(uint8_t)
                                                + sizeof(uint16_t);
};

// Data structure to record game moves without sequence numbers
struct CommandHead
{
    CommandHead(Command cmd)
    :sender(cmd.sender),
    type(cmd.type),
    tank_id(cmd.tank_id),
    payload_first(cmd.payload_first),
    payload_second(cmd.payload_second)
    {}

    CommandHead()
    :sender(0),
    type(static_cast<CommandType>(0)),
    tank_id(0),
    payload_first(0),
    payload_second(0)
    {}

    uint8_t sender;
    CommandType type;
    uint8_t tank_id;
    // Primary place to put a command parameter.
    uint8_t payload_first;
    // Mostly used to give a second x coordinate for placement.
    uint8_t payload_second;

    static constexpr std::size_t COMMAND_SIZE = 5 * sizeof(uint8_t);
};

namespace glz{

template <>
struct meta<CommandHead> {
    using T = CommandHead;
    static constexpr auto value = object(
        &T::sender,
        &T::type,
        &T::tank_id,
        &T::payload_first,
        &T::payload_second
    );
};

};
