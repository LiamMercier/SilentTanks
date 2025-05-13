#include "message.h"

template<class>
struct always_false : std::false_type {};

bool Message::valid_matching_command() const
{
    return (payload[0] < uint8_t(GameMode::NO_MODE));
}

template void Message::create_serialized<QueueMatchRequest>(QueueMatchRequest const&);

template void Message::create_serialized<CancelMatchRequest>(CancelMatchRequest const&);

template void Message::create_serialized<PlayerView>(PlayerView const&);

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

// Modify view with success return type.
PlayerView Message::to_player_view(bool & op_status)
{
    // If our payload is not able to be turned into
    // a view then return failure.
    if (payload.size() < 3)
    {
        op_status = false;
        return PlayerView{};
    }

    uint8_t n_tanks = payload[0];
    uint8_t width = payload[1];
    uint8_t height = payload[2];

    // Grab the view dimensions and check that we have valid data
    //
    // We need 3 bytes for the previous data, plus width*height bytes
    // for the environment, plus 8*num_tanks to hold the tank data
    size_t view_size = 3
                     + (size_t(width) * size_t(height) * 3)
                     + size_t(n_tanks) * 8;

    // If we have malformed data, simply return false
    if (payload.size() != view_size)
    {
        std::cout << "wrong size in message to player view \n";
        std::cout << "Payload " << payload.size() << "vs " << view_size << "\n";
        op_status = false;
        return PlayerView{};
    }

    PlayerView view(width, height);

    // Otherwise, create the player view from the message data
    Environment & map_view = view.map_view;

    map_view.set_width(width);
    map_view.set_height(height);

    // Start at payload[3] since we already read the first 3 bytes
    size_t idx = 3;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // Read 3 bytes at a time, these are the cell values
            GridCell curr_cell;
            curr_cell.type_ = static_cast<CellType>(payload[idx++]);
            curr_cell.occupant_ = payload[idx++];
            curr_cell.visible_ = static_cast<bool>(payload[idx++]);

            // Put the grid cell back into the map view
            map_view[map_view.idx(x,y)] = curr_cell;
        }
    }

    // Now add the tank information
    view.visible_tanks.clear();
    view.visible_tanks.reserve(n_tanks);

    for (uint8_t i = 0; i < n_tanks; i++)
    {
        Tank this_tank;

        this_tank.pos_.x_ = payload[idx++];
        this_tank.pos_.y_ = payload[idx++];
        this_tank.current_direction_ = payload[idx++];
        this_tank.barrel_direction_ = payload[idx++];
        this_tank.health_ = payload[idx++];
        this_tank.aim_focused_ = static_cast<bool>(payload[idx++]);
        this_tank.loaded_ = static_cast<bool>(payload[idx++]);
        this_tank.owner_ = payload[idx++];

        view.visible_tanks.push_back(this_tank);
    }

    // Return our result
    return view;
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

    // For QueueMatchRequest we already have uint8_t.
    if constexpr (std::is_same_v<mType, QueueMatchRequest>)
    {
        // just push into the buffer, no network specific differences.
        payload_buffer.push_back(static_cast<uint8_t>(req.mode));
        header.type_ = HeaderType::QueueMatch;
    }
    // For CancelMatchRequest we already have uint8_t.
    else if constexpr (std::is_same_v<mType, CancelMatchRequest>)
    {
        payload_buffer.push_back(static_cast<uint8_t>(req.mode));
        header.type_ = HeaderType::CancelMatch;
    }
    // For views, we need to put the FlatArray of grid cells into the buffer.
    //
    // We also need to include other state information like
    // the tanks seen and their status.
    //
    // Our data is uint8_t so we simply need to create a defined format
    // for the data.
    else if constexpr (std::is_same_v<mType, PlayerView>)
    {
        const Environment & map_view = req.map_view;

        // First, set the header to the correct type
        header.type_ = HeaderType::PlayerView;

        // Next add the number of visible tanks
        uint8_t n_tanks = static_cast<uint8_t>(req.visible_tanks.size());
        payload_buffer.push_back(n_tanks);

        // Now serialize the environment data, starting with dimensions.
        payload_buffer.push_back(map_view.get_width());
        payload_buffer.push_back(map_view.get_height());

        // Then all of the environment data
        for (int y = 0; y < map_view.get_height(); y++)
        {
            for (int x = 0; x < map_view.get_width(); x++)
            {
                GridCell curr = map_view[map_view.idx(x,y)];

                // For each cell, append the grid cell members
                payload_buffer.push_back(static_cast<uint8_t>(curr.type_));
                payload_buffer.push_back(curr.occupant_);
                payload_buffer.push_back(curr.visible_);

            }
        }

        // Finally, add the tank information
        for (int i = 0; i < n_tanks; i++)
        {
            const Tank & curr_tank = req.visible_tanks[i];

            payload_buffer.push_back(curr_tank.pos_.x_);
            payload_buffer.push_back(curr_tank.pos_.y_);
            payload_buffer.push_back(curr_tank.current_direction_);
            payload_buffer.push_back(curr_tank.barrel_direction_);
            payload_buffer.push_back(curr_tank.health_);
            payload_buffer.push_back(static_cast<uint8_t>(curr_tank.aim_focused_));
            payload_buffer.push_back(static_cast<uint8_t>(curr_tank.loaded_));
            payload_buffer.push_back(curr_tank.owner_);
        }

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
