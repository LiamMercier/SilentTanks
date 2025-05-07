#include "message.h"

template<class>
struct always_false : std::false_type {};

bool Message::valid_matching_command() const
{
    return (payload[0] < uint8_t(GameMode::NO_MODE));
}

template std::vector<uint8_t> Message::create_serialized<QueueMatchRequest>(QueueMatchRequest const&);

// TODO: more implementations
// TODO: revise scope
// Function to create a network serialized message for a message type.
template <typename mType>
std::vector<uint8_t> Message::create_serialized(const mType & req)
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
    Header net_header  = header.to_network();

    // create new buffer to return
    std::vector<uint8_t> buffer;

    // create a "header pointer" and fill the buffer with our network data.
    auto hptr = reinterpret_cast<const uint8_t*>(&net_header);
    buffer.insert(buffer.end(), hptr, hptr + sizeof(net_header));
    buffer.insert(buffer.end(), payload_buffer.begin(), payload_buffer.end());

    return buffer;
}
