#include "message.h"

template<class>
struct always_false : std::false_type {};

bool Message::valid_matching_command() const
{
    return (payload[0] < uint8_t(GameMode::NO_MODE));
}

template void Message::create_serialized<QueueMatchRequest>(QueueMatchRequest const&);

// Convert the current message to a command to be used in a game instance.
Command Message::to_command()
{
    Command cmd;

    // Check that command is of valid size
    if (payload.size() < sizeof(Command))
    {
        // Set to invalid and allow caller to handle this.
        cmd.type = CommandType::NO_OP;
        return cmd;
    }

    // Start creating the command from the message.
    //
    // Most of our Command values are uint8_t and
    // thus are fine for network transfer.
    cmd.sender = payload[0];
    cmd.type = static_cast<CommandType>(payload[1]);
    cmd.tank_id = payload[2];
    cmd.payload_first = payload[3];
    cmd.payload_second = payload[4];

    // Handle network ordering of the sequence number.
    // This would be bytes payload[5] ... payload[8]
    uint32_t net_sequence;
    std::memcpy(&net_sequence, payload.data() + 5, sizeof(net_sequence));

    // Which we then convert with network to host long.
    cmd.sequence_number = ntohl(net_sequence);

    return cmd;

}

// TODO: more implementations
//
// NOTE: remember to force command to message implementation to
//       send data for all members even if payload_second is useless.
//
//       otherwise it will be rejected.
//
// Function to create a network serialized message for a message type.
template <typename mType>
void Message::create_serialized(const mType & req)
{
    std::vector<uint8_t> payload_buffer;

    // pack the payload based on the message type

    // for QueueMatchRequest we already have uint8_t
    if constexpr (std::is_same_v<mType, QueueMatchRequest>)
    {
        // just push into the buffer, no network specific differences.
        payload_buffer.push_back(static_cast<uint8_t>(req.mode));
        header.type_ = HeaderType::QueueMatch;
    }
    // for CancelMatchRequest we already have uint8_t
    else if constexpr (std::is_same_v<mType, CancelMatchRequest>)
    {
        payload_buffer.push_back(static_cast<uint8_t>(req.mode));
        header.type_ = HeaderType::CancelMatch;
    }
    else
    {
        // We shouldn't call serialize if we haven't implemented the serialization scheme.
        static_assert(always_false<mType>::value, "serialize() not implemented for this type");
    }

    // calculate payload length
    header.payload_len = static_cast<uint32_t>(payload_buffer.size());

    header = header.to_network();

    // add payload to the message, which is in network format
    payload = std::move(payload_buffer);

    return;
}

// Create a header only message (for calls that have no payload)
void Message::create_serialized(HeaderType h_type)
{
    header.type_ = h_type;
    header.payload_len = 0;
    header = header.to_network();
    payload.clear();
}
