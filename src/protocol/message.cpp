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

#include "message.h"

// Utility function to turn uint64_t into network order.
int64_t htonll(int64_t host_long_long)
{
    if constexpr (std::endian::native == std::endian::little)
    {
        return std::byteswap(host_long_long);
    }
    else
    {
        return host_long_long;
    }
}

template<class>
struct always_false : std::false_type {};

bool Message::valid_matching_command() const
{
    return (payload[0] < uint8_t(GameMode::NO_MODE));
}

template void Message::create_serialized<GoodAuthNotification>(GoodAuthNotification const&);

template void Message::create_serialized<QueueMatchRequest>(QueueMatchRequest const&);

template void Message::create_serialized<CancelMatchRequest>(CancelMatchRequest const&);

template void Message::create_serialized<MatchHistoryRequest>(MatchHistoryRequest const&);

template void Message::create_serialized<MatchStartNotification>(MatchStartNotification const&);

template void Message::create_serialized<PlayerView>(PlayerView const&);

template void Message::create_serialized<Command>(Command const&);

template void Message::create_serialized<BadRegNotification>(BadRegNotification const&);

template void Message::create_serialized<BadAuthNotification>(BadAuthNotification const&);

template void Message::create_serialized<LoginRequest>(LoginRequest const&);

template void Message::create_serialized<RegisterRequest>(RegisterRequest const&);

template void Message::create_serialized<BanMessage>(BanMessage const&);

template void Message::create_serialized<UserList>(UserList const&);

template void Message::create_serialized<FriendRequest>(FriendRequest const&);

template void Message::create_serialized<FriendDecision>(FriendDecision const&);

template void Message::create_serialized<UnfriendRequest>(UnfriendRequest const&);

template void Message::create_serialized<BlockRequest>(BlockRequest const&);

template void Message::create_serialized<UnblockRequest>(UnblockRequest const&);

template void Message::create_serialized<NotifyRelationUpdate>(NotifyRelationUpdate const&);

template void Message::create_serialized<TextMessage>(TextMessage const&);

template void Message::create_serialized<ExternalMatchMessage>(ExternalMatchMessage const&);

template void Message::create_serialized<MatchResultList>(MatchResultList const&);

template void Message::create_serialized<ReplayRequest>(ReplayRequest const&);

template void Message::create_serialized<MatchReplay>(MatchReplay const&);

template void Message::create_serialized<StaticMatchData>(StaticMatchData const&);

std::array<int, RANKED_MODES_COUNT> Message::to_elos()
{
    std::array<int, RANKED_MODES_COUNT> elos{};

    if (payload.size() != RANKED_MODES_COUNT * sizeof(uint32_t))
    {
        return {};
    }

    size_t offset = 0;

    // Go through each elo.
    for (size_t i = 0; i < RANKED_MODES_COUNT; i++)
    {
        // Copy 4 bytes and increase offset.
        uint32_t net_elo;
        std::memcpy(&net_elo,
                    payload.data() + offset,
                    sizeof(uint32_t));

        offset += sizeof(uint32_t);

        // Convert to host values.
        uint32_t host_elo = ntohl(net_elo);
        elos[i] = static_cast<int>(host_elo);
    }

    return elos;
}

LoginRequest Message::to_login_request() const
{
    LoginRequest request{};

    size_t idx = 0;

    // If there isn't at least HASH_LENGTH bytes for the password
    // plus one for the username then we have an invalid attempt.
    if (payload.size() < HASH_LENGTH + 1)
    {
        return {};
    }

    // If the username is too long, invalid.
    if (payload.size() > HASH_LENGTH + MAX_USERNAME_LENGTH)
    {
        return {};
    }

    // copy the hash bytes
    std::copy(payload.begin(), payload.begin() + HASH_LENGTH, request.hash.begin());

    idx += HASH_LENGTH;

    // Copy while ensuring valid username.
    std::string username;
    size_t username_len = (payload.size() - idx);
    username.reserve(username_len);

    for (size_t i = 0; i < username_len; i++)
    {
        unsigned char c = static_cast<unsigned char>(payload[idx + i]);
        if (!allowed_username_characters[c])
        {
            return {};
        }
        username.push_back(c);
    }

    request.username = std::move(username);

    return request;
}

// Convert the current message to a command to be used in a game instance.
Command Message::to_command()
{
    Command cmd;

    // Check that command is of valid size
    if (payload.size() < Command::COMMAND_SIZE)
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
    // This would be bytes payload[5], payload[6]
    uint16_t net_sequence;
    std::memcpy(&net_sequence, payload.data() + 5, sizeof(net_sequence));

    // Which we then convert with network to host short.
    cmd.sequence_number = ntohs(net_sequence);

    return cmd;

}

StaticMatchData Message::to_static_match_data(bool & op_status)
{
    // If there is not at least 2 UUIDs of data then stop.
    if (payload.size() < 2 * 16)
    {
        op_status = false;
        return StaticMatchData{};
    }

    StaticMatchData match_data;

    // Convert the list of users.
    uint8_t num_players = payload[0];

    // Ensure valid number of players.
    if (num_players > MAX_PLAYERS)
    {
        op_status = false;
        return StaticMatchData{};
    }

    size_t offset = 1;
    size_t total = payload.size();

    UserList user_list;

    // While we still have more users to go through.
    while (user_list.users.size() < num_players)
    {
        // If we can't read another 17 bytes, stop.
        if (offset + 16 + 1 > total)
        {
            op_status = false;
            return StaticMatchData{};
        }

        ExternalUser user;

        // Copy this user's uuid bytes from the buffer.
        std::memcpy(user.user_id.data,
                    payload.data() + offset,
                    16);

        // Update offset, get the username length. We store this
        // as a uint8_t since usernames lengths are less than 256.
        offset += 16;
        uint8_t username_len = payload[offset++];

        // If invalid username length.
        if (username_len > MAX_USERNAME_LENGTH)
        {
            op_status = false;
            return {};
        }

        // If not enough data.
        if (username_len + offset > total)
        {
            op_status = false;
            return {};
        }

        // Copy while ensuring valid username.
        std::string username;
        for (int i = 0; i < username_len; i++)
        {
            unsigned char c = static_cast<unsigned char>(payload[offset + i]);
            if (!allowed_username_characters[c])
            {
                op_status = false;
                return {};
            }
            username.push_back(c);
        }

        user.username = std::move(username);
        user_list.users.push_back(std::move(user));

        offset += username_len;
    }

    match_data.player_list = user_list;

    // Finally, grab the placement mask.
    std::vector<uint8_t> mask;

    mask.insert(
        mask.end(),
        reinterpret_cast<const uint8_t*>(payload.data()) + offset,
        reinterpret_cast<const uint8_t*>(payload.data()) + total
        );

    match_data.placement_mask = mask;

    op_status = true;
    return match_data;
}

// Modify view with success return type.
PlayerView Message::to_player_view(bool & op_status)
{
    constexpr size_t FIXED_BYTES_SIZE = 7;

    // If our payload is not able to be turned into
    // a view then return failure.
    if (payload.size() < FIXED_BYTES_SIZE)
    {
        op_status = false;
        return PlayerView{};
    }

    uint8_t n_tanks = payload[0];
    uint8_t current_player = payload[1];
    uint8_t width = payload[2];
    uint8_t height = payload[3];
    uint8_t current_fuel = payload[4];
    GameState current_state = static_cast<GameState>(payload[5]);
    uint8_t n_timers = payload[6];

    // Start at payload[7] since we already read the first 7 bytes
    size_t index = FIXED_BYTES_SIZE;

    // If we have malformed data, simply return false
    if (payload.size() - index < (size_t(width) * size_t(height) * 3))
    {
        op_status = false;
        return PlayerView{};
    }

    PlayerView view(width, height);

    view.current_player = current_player;
    view.current_fuel = current_fuel;
    view.current_state = current_state;

    // Otherwise, create the player view from the message data
    FlatArray<GridCell> & map_view = view.map_view;

    map_view.set_width(width);
    map_view.set_height(height);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // Read 3 bytes at a time, these are the cell values
            GridCell curr_cell;
            curr_cell.type_ = static_cast<CellType>(payload[index++]);
            curr_cell.occupant_ = payload[index++];
            curr_cell.visible_ = static_cast<bool>(payload[index++]);

            // Put the grid cell back into the map view
            map_view[map_view.idx(x,y)] = curr_cell;
        }
    }

    // If we can't fit the tank information.
    if (payload.size() - index < size_t(n_tanks) * 9)
    {
        op_status = false;
        return PlayerView{};
    }

    // Now add the tank information
    view.visible_tanks.clear();
    view.visible_tanks.reserve(n_tanks);

    for (uint8_t i = 0; i < n_tanks; i++)
    {
        Tank this_tank;

        this_tank.pos_.x_ = payload[index++];
        this_tank.pos_.y_ = payload[index++];
        this_tank.current_direction_ = payload[index++];
        this_tank.barrel_direction_ = payload[index++];
        this_tank.id_ = payload[index++];
        this_tank.health_ = payload[index++];
        this_tank.aim_focused_ = static_cast<bool>(payload[index++]);
        this_tank.loaded_ = static_cast<bool>(payload[index++]);
        this_tank.owner_ = payload[index++];

        view.visible_tanks.push_back(this_tank);
    }

    // If we don't have the correct data for timers.
    if (payload.size() - index < size_t(n_timers) * 8)
    {
        op_status = false;
        return PlayerView{};
    }

    for (uint8_t j = 0; j < n_timers; j++)
    {
        uint64_t raw;
        std::memcpy(&raw,
                    payload.data() + index,
                    sizeof(raw));

        int64_t ms_int = static_cast<int64_t>(htonll(raw));
        std::chrono::milliseconds ms{ms_int};

        view.timers.push_back(ms);
        index += sizeof(raw);
    }

    op_status = true;

    // Return our result
    return view;
}

BanMessage Message::to_ban_message()
{
    using system_clock = std::chrono::system_clock;

    BanMessage ban_message;

    // If message is too small, return a default. Called must check
    // for this after return.
    if (payload.size() < sizeof(int64_t))
    {
        ban_message.time_till_unban = system_clock::time_point::min();
        return ban_message;
    }

    // Grab message bytes. We know at least 8 bytes exist because
    // of our check.
    int64_t network_time;
    std::memcpy(&network_time, payload.data(), sizeof(int64_t));

    // Convert to host time.
    //
    // Since we used byteswap, we can just call the same function
    int64_t host_time = htonll(network_time);

    // Convert to time point for our internal use.
    std::time_t unban_time_t = static_cast<std::time_t>(host_time);
    ban_message.time_till_unban = system_clock::from_time_t(unban_time_t);

    // If a reason exists, convert it.
    if (payload.size() > sizeof(int64_t))
    {
        const uint8_t * reason_ptr = payload.data() + sizeof(int64_t);
        size_t len = payload.size() - sizeof(int64_t);

        // Construct string given the data pointer and length.
        ban_message.reason = std::string(
                                reinterpret_cast<const char *>(reason_ptr),
                                len);
    }

    return ban_message;
}

UserList Message::to_user_list(bool & op_status)
{
    size_t offset = 0;
    size_t total = payload.size();

    UserList user_list;

    // While we still have more users to go through.
    while (offset < total)
    {
        // If we can't read another 17 bytes, stop.
        if (offset + 16 + 1 > total)
        {
            op_status = false;
            return {};
        }

        ExternalUser user;

        // Copy this user's uuid bytes from the buffer.
        std::memcpy(user.user_id.data,
                    payload.data() + offset,
                    16);

        // Update offset, get the username length. We store this
        // as a uint8_t since usernames lengths are less than 256.
        offset += 16;
        uint8_t username_len = payload[offset++];

        // If invalid username length.
        if (username_len > MAX_USERNAME_LENGTH)
        {
            op_status = false;
            return {};
        }

        // If not enough data.
        if (username_len + offset > total)
        {
            op_status = false;
            return {};
        }

        // Copy while ensuring valid username.
        std::string username;
        for (int i = 0; i < username_len; i++)
        {
            unsigned char c = static_cast<unsigned char>(payload[offset + i]);
            if (!allowed_username_characters[c])
            {
                op_status = false;
                return {};
            }
            username.push_back(c);
        }

        user.username = std::move(username);
        user_list.users.push_back(std::move(user));

        offset += username_len;
    }

    op_status = true;
    return user_list;
}

std::string Message::to_username()
{
    // Create empty string on bad data. Should never happen if we
    // reject headers of incorrect size.
    if (payload.size() < 1 || payload.size() > MAX_USERNAME_LENGTH)
    {
        return {};
    }

    // Otherwise, the whole payload is the username. Check it for
    // valid characters and construct the string.
    std::string username;
    username.reserve(payload.size());

    for (uint8_t byte : payload)
    {
        unsigned char c = static_cast<unsigned char>(byte);
        if (!allowed_username_characters[c])
        {
            return {};
        }
        username.push_back(c);
    }

    return username;
}

boost::uuids::uuid Message::to_uuid()
{
    boost::uuids::uuid user_id{};

    // Return nil ID on bad message.
    if (payload.size() != 16)
    {
        return user_id;
    }

    // Otherwise, construct from bytes.
    std::copy(
        payload.begin(),
        payload.begin() + 16,
        user_id.begin()
    );

    return user_id;
}

FriendDecision Message::to_friend_decision()
{
    FriendDecision friend_decision{};

    // Return nil ID on bad message.
    if (payload.size() != 17)
    {
        return friend_decision;
    }

    // Copy uuid bytes.
    std::copy(
        payload.begin(),
        payload.begin() + 16,
        friend_decision.user_id.begin()
    );

    // Add the decision.
    friend_decision.decision = static_cast<bool>(payload[16]);

    return friend_decision;
}

ExternalUser Message::to_user()
{
    ExternalUser user{};
    if (payload.size() < 16 || payload.size() > 16 + MAX_USERNAME_LENGTH)
    {
        return {};
    }

    // Copy the UUID.
    std::copy(
        payload.begin(),
        payload.begin() + 16,
        user.user_id.begin()
    );

    // Copy username bytes, if any.
    size_t start = 16;
    size_t len = payload.size() - start;

    if (len > MAX_USERNAME_LENGTH)
    {
        return {};
    }

    user.username.assign(reinterpret_cast<const char*>(&payload[start]), len);

    return user;
}

TextMessage Message::to_text_message()
{
    TextMessage dm{};

    if (payload.size() < 17)
    {
        return dm;
    }

    // Copy the UUID.
    std::copy(
        payload.begin(),
        payload.begin() + 16,
        dm.user_id.begin()
    );

    // Construct the text.
    size_t start = 16;
    size_t len = payload.size() - start;

    dm.text.assign(reinterpret_cast<const char*>(&payload[start]), len);

    return dm;
}

// We can convert from the ExternalMatchMessage we got on the wire
// and an internal version (no username length stored) by simply
// parsing the external message and not saving the length.
InternalMatchMessage Message::to_match_message()
{
    InternalMatchMessage msg;

    size_t offset = 0;

    // If we are not holding a UUID, username size (1 byte),
    // username (at least 1 byte), and at least some text,
    // then stop now.
    if (payload.size() < 16 + 1 + 1 + 1)
    {
        return {};
    }

    std::copy(
        payload.begin(),
        payload.begin() + 16,
        msg.user_id.begin()
    );

    offset += 16;

    uint8_t username_len = payload[offset];

    offset += 1;

    // If not enough bytes, close now.
    if (payload.size() < offset + username_len + 1)
    {
        return {};
    }

    // Otherwise, grab the username.
    msg.sender_username.assign
                        (
                            reinterpret_cast<const char*>(&payload[offset]),
                            username_len
                        );

    offset += username_len;

    // Grab the text.
    msg.text.assign(reinterpret_cast<const char*>(&payload[offset]),
                    payload.size() - offset);

    return msg;
}

GameMode Message::to_gamemode()
{
    // Just in case, but header should be rejected anyways.
    if (payload.size() != 1)
    {
        return GameMode::NO_MODE;
    }

    // Clip mode values that are too high.
    if (payload[0] >= static_cast<uint8_t>(GameMode::NO_MODE))
    {
        return GameMode::NO_MODE;
    }

    return static_cast<GameMode>(payload[0]);
}

MatchResultList Message::to_results_list()
{
    MatchResultList results;

    if (payload.size() < 1)
    {
        return {};
    }

    // Fetch the first byte, will always exist from header check
    // and holds the mode.
    results.mode = static_cast<GameMode>(payload[0]);

    size_t index = 1;

    // While remaining bytes is at least one MatchResultRow in size.
    while (payload.size() - index >= MatchResultRow::DATA_SIZE)
    {
        MatchResultRow row{};

        // Copy and convert id from network.
        int64_t net_id;
        std::memcpy(&net_id, payload.data() + index, sizeof(net_id));
        index += sizeof(net_id);
        row.match_id = htonll(net_id);

        // Copy and convert network time.
        int64_t net_time;
        std::memcpy(&net_time, payload.data() + index, sizeof(net_time));
        index += sizeof(net_time);

        int64_t host_time = htonll(net_time);
        std::time_t finish_time = static_cast<std::time_t>(host_time);
        row.finished_at = std::chrono::system_clock::from_time_t(finish_time);

        // Copy and convert placement.
        uint16_t net_placement;
        std::memcpy(&net_placement, payload.data() + index, sizeof(net_placement));
        index += sizeof(net_placement);
        row.placement = ntohs(net_placement);

        // Copy and convert elo change.
        int32_t net_elo;
        std::memcpy(&net_elo, payload.data() + index, sizeof(net_elo));
        index += sizeof(net_elo);
        row.elo_change = ntohl(net_elo);

        results.match_results.push_back(row);
    }

    return results;
}

ReplayRequest Message::to_replay_request()
{
    ReplayRequest req(0);

    if (payload.size() < sizeof(uint64_t))
    {
        return req;
    }

    uint64_t net_match_id;
    std::memcpy(&net_match_id, payload.data(), sizeof(net_match_id));

    req.match_id = htonll(net_match_id);

    return req;
}

MatchReplay Message::to_match_replay(bool & op_status)
{
    MatchReplay replay{};

    size_t index = 0;

    uint32_t turn_count;

    if (payload.size() - index < sizeof(turn_count))
    {
        op_status = false;
        return replay;
    }

    std::memcpy(&turn_count,
                payload.data() + index,
                sizeof(turn_count));

    turn_count = ntohl(turn_count);
    index += sizeof(turn_count);

    uint16_t filename_length;

    if (payload.size() - index < sizeof(filename_length))
    {
        op_status = false;
        return replay;
    }

    std::memcpy(&filename_length,
                payload.data() + index,
                sizeof(filename_length));

    filename_length = ntohs(filename_length);
    index += sizeof(filename_length);

    uint64_t time_ms;

    if (payload.size() - index < sizeof(time_ms))
    {
        op_status = false;
        return replay;
    }

    std::memcpy(&time_ms,
                payload.data() + index,
                sizeof(time_ms));

    time_ms = htonll(time_ms);
    index += sizeof(time_ms);

    uint64_t increment_ms;

    if (payload.size() - index < sizeof(increment_ms))
    {
        op_status = false;
        return replay;
    }

    std::memcpy(&increment_ms,
                payload.data() + index,
                sizeof(increment_ms));

    increment_ms = htonll(increment_ms);
    index += sizeof(increment_ms);

    uint64_t match_id;

    if (payload.size() - index < sizeof(match_id))
    {
        op_status = false;
        return replay;
    }

    std::memcpy(&match_id,
                payload.data() + index,
                sizeof(match_id));

    match_id = htonll(match_id);
    index += sizeof(match_id);

    // Add the main metadata to the replay.
    replay.initial_time_ms = time_ms;
    replay.increment_ms = increment_ms;
    replay.match_id = match_id;

    // If not enough data for filename, exit.
    if (payload.size() - index < filename_length)
    {
        op_status = false;
        return replay;
    }

    // grab the filename
    std::string filename;
    filename.assign(reinterpret_cast<const char*>(&payload[index]),
                    filename_length);
    index += filename_length;

    // If not enough data for settings, exit.
    if (payload.size() - index < 5 * sizeof(uint8_t))
    {
        op_status = false;
        return replay;
    }

    replay.settings.filename = std::move(filename);
    replay.settings.width = payload[index];
    replay.settings.height = payload[index + 1];
    replay.settings.num_tanks = payload[index + 2];
    replay.settings.num_players = payload[index + 3];
    replay.settings.mode = payload[index + 4];
    index += 5;

    // Try to grab num_players users.
    for (int i = 0; i < replay.settings.num_players; i++)
    {
        // If we don't have 17 bytes for the UUID + username length, stop.
        if (payload.size() - index < 17)
        {
            op_status = false;
            return replay;
        }

        // Grab the UUID and then the username.
        ExternalUser user;

        std::memcpy(user.user_id.data,
                    payload.data() + index,
                    16);

        index += 16;

        uint8_t username_len = payload[index++];

        // If invalid username length.
        if (username_len > MAX_USERNAME_LENGTH)
        {
            op_status = false;
            return replay;
        }

        // If not enough data.
        if (username_len + index >= payload.size())
        {
            op_status = false;
            return replay;
        }

        // Copy while ensuring valid username.
        std::string username;
        for (int i = 0; i < username_len; i++)
        {
            unsigned char c = static_cast<unsigned char>(payload[index + i]);
            if (!allowed_username_characters[c])
            {
                op_status = false;
                return replay;
            }
            username.push_back(c);
        }

        user.username = std::move(username);
        replay.players.users.push_back(std::move(user));

        index += username_len;
    }

    // If not exactly turn_count commands of data are left, exit.
    if (payload.size() - index != (CommandHead::COMMAND_SIZE) * turn_count)
    {
        op_status = false;
        return replay;
    }

    // While there is still a command to process.
    while (payload.size() - index >= (CommandHead::COMMAND_SIZE))
    {
        CommandHead command;

        command.sender = payload[index];
        command.type = static_cast<CommandType>(payload[index + 1]);
        command.tank_id = payload[index + 2];
        command.payload_first = payload[index + 3];
        command.payload_second = payload[index + 4];

        replay.moves.push_back(command);
        index += CommandHead::COMMAND_SIZE;
    }

    op_status = true;
    return replay;
}

// Function to create a network serialized message for a message type.
template <typename mType>
void Message::create_serialized(const mType & req)
{
    std::vector<uint8_t> payload_buffer;

    // pack the payload based on the message type
    if constexpr (std::is_same_v<mType, GoodAuthNotification>)
    {
        header.type_ = HeaderType::GoodAuth;

        // Iterate through user elos, converting them to network ready form.
        for (int elo : req.elos)
        {
            uint32_t net_elo = htonl(static_cast<uint32_t>(elo));

            // cast to "array" and insert it.
            uint8_t * bytes = reinterpret_cast<uint8_t*>(&net_elo);
            payload_buffer.insert(payload_buffer.end(),
                                  bytes,
                                  bytes + sizeof(net_elo));
        }

    }
    else if constexpr (std::is_same_v<mType, QueueMatchRequest>)
    {
        header.type_ = HeaderType::QueueMatch;
        payload_buffer.push_back(static_cast<uint8_t>(req.mode));
    }
    else if constexpr (std::is_same_v<mType, CancelMatchRequest>)
    {
        header.type_ = HeaderType::CancelMatch;
        payload_buffer.push_back(static_cast<uint8_t>(req.mode));
    }
    else if constexpr (std::is_same_v<mType, MatchHistoryRequest>)
    {
        header.type_ = HeaderType::FetchMatchHistory;
        payload_buffer.push_back(static_cast<uint8_t>(req.mode));
    }
    else if constexpr (std::is_same_v<mType, MatchResultList>)
    {
        header.type_ = HeaderType::MatchHistory;
        payload_buffer.push_back(static_cast<uint8_t>(req.mode));

        // For each result in the result list, push back each data
        // element using network order.
        for (const MatchResultRow & result : req.match_results)
        {
            // Insert match ID.
            int64_t net_match_id = htonll(result.match_id);

            uint8_t* id_bytes = reinterpret_cast<uint8_t*>(&net_match_id);
            payload_buffer.insert(payload_buffer.end(),
                                  id_bytes,
                                  id_bytes + sizeof(net_match_id));

            // Insert finished at time.
            int64_t network_time = htonll(static_cast<int64_t>(
                                        std::chrono::system_clock::to_time_t(
                                            result.finished_at
                                        )));

            uint8_t* time_bytes = reinterpret_cast<uint8_t*>(&network_time);
            payload_buffer.insert(payload_buffer.end(),
                                  time_bytes,
                                  time_bytes + sizeof(network_time));

            // Insert placement.
            uint16_t net_placement = htons(result.placement);

            uint8_t* placement_bytes = reinterpret_cast<uint8_t*>(&net_placement);
            payload_buffer.insert(payload_buffer.end(),
                                  placement_bytes,
                                  placement_bytes + sizeof(net_placement));

            // Insert elo.
            uint32_t net_elo = htonl(static_cast<uint32_t>(result.elo_change));

            uint8_t* elo_bytes = reinterpret_cast<uint8_t*>(&net_elo);
            payload_buffer.insert(payload_buffer.end(),
                                  elo_bytes,
                                  elo_bytes + sizeof(net_elo));
        }
    }
    else if constexpr (std::is_same_v<mType, ReplayRequest>)
    {
        header.type_ = HeaderType::MatchReplayRequest;

        uint64_t net_match_id = htonll(req.match_id);
        uint8_t* id_bytes = reinterpret_cast<uint8_t*>(&net_match_id);
        payload_buffer.insert(payload_buffer.end(),
                              id_bytes,
                              id_bytes + sizeof(net_match_id));
    }
    else if constexpr (std::is_same_v<mType, MatchReplay>)
    {
        header.type_ = HeaderType::MatchReplay;

        // Serialize the number of entries as a uint32_t since
        // we may have many turns.
        uint32_t turn_count = req.moves.size();
        turn_count = htonl(turn_count);

        uint8_t* turn_count_bytes = reinterpret_cast<uint8_t*>(&turn_count);
        payload_buffer.insert(payload_buffer.end(),
                              turn_count_bytes,
                              turn_count_bytes + sizeof(turn_count));

        // Serialize the map name length.
        uint16_t filename_length = req.settings.filename.length();
        filename_length = htons(filename_length);

        uint8_t* name_length_bytes = reinterpret_cast<uint8_t*>(&filename_length);
        payload_buffer.insert(payload_buffer.end(),
                              name_length_bytes,
                              name_length_bytes + sizeof(filename_length));

        // Serialize the initial time and increment.
        uint64_t time_ms = req.initial_time_ms;
        time_ms = htonll(time_ms);

        uint8_t* initial_time_bytes = reinterpret_cast<uint8_t*>(&time_ms);
        payload_buffer.insert(payload_buffer.end(),
                              initial_time_bytes,
                              initial_time_bytes + sizeof(time_ms));

        uint64_t increment_ms = req.increment_ms;
        increment_ms = htonll(increment_ms);

        uint8_t* increment_bytes = reinterpret_cast<uint8_t*>(&increment_ms);
        payload_buffer.insert(payload_buffer.end(),
                              increment_bytes,
                              increment_bytes + sizeof(increment_ms));

        uint64_t match_id = req.match_id;
        match_id = htonll(match_id);

        uint8_t* match_id_bytes = reinterpret_cast<uint8_t*>(&match_id);
        payload_buffer.insert(payload_buffer.end(),
                              match_id_bytes,
                              match_id_bytes + sizeof(match_id));

        // Now push the match settings.
        payload_buffer.insert(payload_buffer.end(),
                              req.settings.filename.begin(),
                              req.settings.filename.end());

        payload_buffer.push_back(req.settings.width);
        payload_buffer.push_back(req.settings.height);
        payload_buffer.push_back(req.settings.num_tanks);
        payload_buffer.push_back(req.settings.num_players);
        payload_buffer.push_back(req.settings.mode);

        // Loop over the users list and add it to the buffer.
        for (const auto & user : req.players.users)
        {
            payload_buffer.insert(
                payload_buffer.end(),
                std::begin(user.user_id),
                std::end(user.user_id));

            // Usernames are not allowed to surpass uint8_t in length.
            //
            // We need to give this to the client so they know when the
            // username ends.
            uint8_t username_len = static_cast<uint8_t>(user.username.size());
            payload_buffer.push_back(username_len);

            // Copy the username.
            payload_buffer.insert(
                payload_buffer.end(),
                reinterpret_cast<const uint8_t*>(user.username.data()),
                reinterpret_cast<const uint8_t*>(user.username.data())
                                                 + username_len);
        }

        // Finally, go through the command list and push moves.
        for (const auto & move : req.moves)
        {
            payload_buffer.push_back(move.sender);
            payload_buffer.push_back(static_cast<uint8_t>(move.type));
            payload_buffer.push_back(move.tank_id);
            payload_buffer.push_back(move.payload_first);
            payload_buffer.push_back(move.payload_second);
        }

    }
    // Create a message to notify the player of their
    // player ID and match status.
    else if constexpr (std::is_same_v<mType, MatchStartNotification>)
    {
        header.type_ = HeaderType::MatchStarting;
        payload_buffer.push_back(static_cast<uint8_t>(req.player_id));
    }
    // Create message to notify the user of a bad signup request.
    else if constexpr (std::is_same_v<mType, BadRegNotification>)
    {
        header.type_ = HeaderType::BadRegistration;
        payload_buffer.push_back(static_cast<uint8_t>(req.reason));
    }
    else if constexpr (std::is_same_v<mType, BadAuthNotification>)
    {
        header.type_ = HeaderType::BadAuth;
        payload_buffer.push_back(static_cast<uint8_t>(req.reason));
    }
    else if constexpr (std::is_same_v<mType, LoginRequest>)
    {
        header.type_ = HeaderType::LoginRequest;

        payload_buffer.insert(payload_buffer.end(),
                              req.hash.begin(),
                              req.hash.end());

        payload_buffer.insert(payload_buffer.end(),
                              req.username.begin(),
                              req.username.end());
    }
    else if constexpr (std::is_same_v<mType, RegisterRequest>)
    {
        header.type_ = HeaderType::RegistrationRequest;

        payload_buffer.insert(payload_buffer.end(),
                              req.hash.begin(),
                              req.hash.end());

        payload_buffer.insert(payload_buffer.end(),
                              req.username.begin(),
                              req.username.end());
    }
    else if constexpr (std::is_same_v<mType, UserList>)
    {
        // Loop over the users list and add it to the buffer.
        for (const auto & user : req.users)
        {
            // UUIDs are 16 byte arrays, so we can just memcpy this.
            payload_buffer.insert(
                payload_buffer.end(),
                std::begin(user.user_id),
                std::end(user.user_id));

            // Usernames are not allowed to surpass uint8_t in length.
            //
            // We need to give this to the client so they know when the
            // username ends.
            uint8_t username_len = static_cast<uint8_t>(user.username.size());
            payload_buffer.push_back(username_len);

            // Copy the username.
            payload_buffer.insert(
                payload_buffer.end(),
                reinterpret_cast<const uint8_t*>(user.username.data()),
                reinterpret_cast<const uint8_t*>(user.username.data())
                                                 + username_len);
        }
    }
    else if constexpr (std::is_same_v<mType, FriendRequest>)
    {
        header.type_ = HeaderType::SendFriendRequest;

        payload_buffer.insert(payload_buffer.end(),
                              req.username.begin(),
                              req.username.end());
    }
    else if constexpr (std::is_same_v<mType, FriendDecision>)
    {
        header.type_ = HeaderType::RespondFriendRequest;

        // Insert UUID.
        payload_buffer.insert(payload_buffer.end(),
                              std::begin(req.user_id),
                              std::end(req.user_id));

        // Cast from bool to uint8_t.
        payload_buffer.push_back(static_cast<uint8_t>(req.decision));
    }
    else if constexpr (std::is_same_v<mType, UnfriendRequest>)
    {
        header.type_ = HeaderType::RemoveFriend;

        // Insert UUID.
        payload_buffer.insert(payload_buffer.end(),
                              std::begin(req.user_id),
                              std::end(req.user_id));
    }
    else if constexpr (std::is_same_v<mType, BlockRequest>)
    {
        header.type_ = HeaderType::BlockUser;

        payload_buffer.insert(payload_buffer.end(),
                              req.username.begin(),
                              req.username.end());
    }
    else if constexpr (std::is_same_v<mType, UnblockRequest>)
    {
        header.type_ = HeaderType::UnblockUser;

        // Insert UUID.
        payload_buffer.insert(payload_buffer.end(),
                              std::begin(req.user_id),
                              std::end(req.user_id));
    }
    else if constexpr (std::is_same_v<mType, NotifyRelationUpdate>)
    {
        // Insert UUID.
        payload_buffer.insert(payload_buffer.end(),
                              std::begin(req.user.user_id),
                              std::end(req.user.user_id));

        // Insert username.
        payload_buffer.insert(payload_buffer.end(),
                              req.user.username.begin(),
                              req.user.username.end());
    }
    else if constexpr (std::is_same_v<mType, TextMessage>)
    {
        // Insert UUID.
        payload_buffer.insert(payload_buffer.end(),
                              std::begin(req.user_id),
                              std::end(req.user_id));

        // Insert text.
        payload_buffer.insert(payload_buffer.end(),
                              req.text.begin(),
                              req.text.end());
    }
    else if constexpr (std::is_same_v<mType, ExternalMatchMessage>)
    {
        // Insert UUID.
        payload_buffer.insert(payload_buffer.end(),
                              std::begin(req.user_id),
                              std::end(req.user_id));

        // Insert the username with its length.
        payload_buffer.push_back(req.username_length);

        payload_buffer.insert(payload_buffer.end(),
                              req.sender_username.begin(),
                              req.sender_username.end());

        // Insert text.
        payload_buffer.insert(payload_buffer.end(),
                              req.text.begin(),
                              req.text.end());
    }
    // Create a ban message to send to a banned user/IP.
    else if constexpr (std::is_same_v<mType, BanMessage>)
    {
        header.type_ = HeaderType::Banned;

        // First, turn this into unix timestamp
        std::time_t unban_time = std::chrono::system_clock::to_time_t(req.time_till_unban);

        // Now, use a consistent sized type.
        int64_t network_time = htonll(static_cast<int64_t>(unban_time));

        // Insert the bytes into the buffer.
        uint8_t* time_bytes = reinterpret_cast<uint8_t*>(&network_time);
        payload_buffer.insert(payload_buffer.end(),
                              time_bytes,
                              time_bytes + sizeof(network_time));

        // Add the ban message to the buffer, if it exists.
        if (!req.reason.empty())
        {
            const char * reason = req.reason.data();
            payload_buffer.insert(
                payload_buffer.end(),
                reinterpret_cast<const uint8_t*>(reason),
                reinterpret_cast<const uint8_t*>(reason) + req.reason.size());
        }

    }
    // Send all data that remains constant.
    else if constexpr (std::is_same_v<mType, StaticMatchData>)
    {
        header.type_ = HeaderType::StaticMatchData;

        // Add player list size.
        payload_buffer.push_back(
            static_cast<uint8_t>(req.player_list.users.size()));

        // Loop over the users and add them.
        for (const auto & user : req.player_list.users)
        {
            // Copy the UUID.
            payload_buffer.insert(payload_buffer.end(),
                                  std::begin(user.user_id),
                                  std::end(user.user_id));

            // Give the username length.
            uint8_t username_len = static_cast<uint8_t>(user.username.size());
            payload_buffer.push_back(username_len);

            // Copy the username.
            payload_buffer.insert(
                payload_buffer.end(),
                reinterpret_cast<const uint8_t*>(user.username.data()),
                reinterpret_cast<const uint8_t*>(user.username.data())
                                                 + username_len);
        }

        // Copy the placement mask.
        payload_buffer.insert(
                payload_buffer.end(),
                reinterpret_cast<const uint8_t*>(req.placement_mask.data()),
                reinterpret_cast<const uint8_t*>(req.placement_mask.data())
                                                 + req.placement_mask.size());
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
        const FlatArray<GridCell> & map_view = req.map_view;

        // First, set the header to the correct type
        header.type_ = HeaderType::PlayerView;

        // Next add the number of visible tanks
        uint8_t n_tanks = static_cast<uint8_t>(req.visible_tanks.size());
        payload_buffer.push_back(n_tanks);

        // And the current player.
        payload_buffer.push_back(req.current_player);

        // Now serialize the environment data, starting with dimensions.
        payload_buffer.push_back(map_view.get_width());
        payload_buffer.push_back(map_view.get_height());

        // Push back the current fuel and state.
        payload_buffer.push_back(req.current_fuel);
        payload_buffer.push_back(static_cast<uint8_t>(req.current_state));

        // Push back the number of timers.
        uint8_t n_timers = static_cast<uint8_t>(req.timers.size());
        payload_buffer.push_back(n_timers);

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

        // Add the tank information
        for (int i = 0; i < n_tanks; i++)
        {
            const Tank & curr_tank = req.visible_tanks[i];

            payload_buffer.push_back(curr_tank.pos_.x_);
            payload_buffer.push_back(curr_tank.pos_.y_);
            payload_buffer.push_back(curr_tank.current_direction_);
            payload_buffer.push_back(curr_tank.barrel_direction_);
            payload_buffer.push_back(curr_tank.id_);
            payload_buffer.push_back(curr_tank.health_);
            payload_buffer.push_back(static_cast<uint8_t>(curr_tank.aim_focused_));
            payload_buffer.push_back(static_cast<uint8_t>(curr_tank.loaded_));
            payload_buffer.push_back(curr_tank.owner_);
        }

        // Finally add each player timer.
        for (size_t j = 0; j < req.timers.size(); j++)
        {
            int64_t ms = static_cast<int64_t>(req.timers[j].count());
            ms = htonll(ms);

            uint8_t* ms_bytes = reinterpret_cast<uint8_t*>(&ms);
            payload_buffer.insert(payload_buffer.end(),
                                  ms_bytes,
                                  ms_bytes + sizeof(ms));
        }

    }
    else if constexpr (std::is_same_v<mType, Command>)
    {
        header.type_ = HeaderType::SendCommand;

        payload_buffer.reserve(Command::COMMAND_SIZE);

        // Copy all the command data into the payload.
        //
        // Start with the single byte data members.
        payload_buffer.push_back(req.sender);
        payload_buffer.push_back(static_cast<uint8_t>(req.type));
        payload_buffer.push_back(req.tank_id);
        payload_buffer.push_back(req.payload_first);
        payload_buffer.push_back(req.payload_second);

        // Now load in the sequence number.
        uint16_t net_sequence = htons(req.sequence_number);

        // Copy into the vector, 1 byte at a time.
        //
        // If net_sequence = 0xABCD then we push back:
        //
        // 0xABCD >> 8 which is 0x00AB and thus is cast to 0xAB
        // 0xABCD & 0xFF which is 0x00CD and thus is cast to 0xCD
        payload_buffer.push_back(static_cast<uint8_t>(net_sequence >> 8));
        payload_buffer.push_back(static_cast<uint8_t>(net_sequence & 0xFF));

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
