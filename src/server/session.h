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
#include <memory>
#include <deque>
#include <iostream>
#include <utility>
#include <chrono>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

#include "message.h"
#include "header.h"
#include "user-data.h"

// Seconds to wait before closing session when data is not sent.
static constexpr uint64_t READ_TIMEOUT = 10;
static constexpr uint64_t PING_TIMEOUT = READ_TIMEOUT;
static constexpr uint64_t WRITE_TIMEOUT = READ_TIMEOUT;

// Seconds between pings to the client.
static constexpr uint64_t PING_INTERVAL = 90;

// Token refill rate per second.
static constexpr uint64_t TOKENS_REFILL_RATE = 10;

// Allow bursts of up to 10x the refill rate.
static constexpr uint64_t MAX_TOKENS = 10 * TOKENS_REFILL_RATE;

// Upper bound on messages waiting to be written.
static constexpt size_t MAX_MESSAGE_BACKLOG = 50;

// Command to get the cost in tokens of a command.
constexpr uint64_t weight_of_cmd(Header h)
{
    switch (h.type_)
    {
        // Value for fetch requests, should be rarely called.
        case HeaderType::FetchFriends: return 20;
        case HeaderType::FetchFriendRequests: return 20;
        case HeaderType::FetchBlocks: return 20;

        // Requests that involve other users (and thus the database).
        case HeaderType::SendFriendRequest: return 20;
        case HeaderType::RespondFriendRequest: return 5;
        case HeaderType::RemoveFriend: return 20;
        case HeaderType::BlockUser: return 20;
        case HeaderType::UnblockUser: return 20;

        // Match result requests (involves heavy database calls).
        case HeaderType::FetchMatchHistory: return 20;
        case HeaderType::MatchReplayRequest: return 20;

        // Message passing variable string for text.
        case HeaderType::DirectTextMessage: return 2 + (h.payload_len / 100);
        case HeaderType::MatchTextMessage: return 2 + (h.payload_len / 100);

        // Game related requests.
        case HeaderType::QueueMatch: return 2;
        case HeaderType::CancelMatch: return 1;
        case HeaderType::SendCommand: return 4;
        case HeaderType::ForfeitMatch: return 1;

        default: return 0;
    }
}

namespace asio = boost::asio;

class Session : public std::enable_shared_from_this<Session>
{
public:
    using ptr = std::shared_ptr<Session>;
    using tcp = asio::ip::tcp;

    // callback function to relay messages through
    using MessageHandler = std::function<void(const ptr& session, Message msg)>;
    using DisconnectHandler = std::function<void(const ptr& session)>;

    Session(asio::io_context & cntx,
            uint64_t session_id,
            asio::ssl::context & ssl_cntx);

    void set_message_handler(MessageHandler handler, DisconnectHandler d_handler);

    void start();

    // disable copy
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    // disable move
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;

    // Send a message to the client
    //
    // If the message does not need to be preserved, one can
    // use deliver(std::move(msg)) to avoid copies.
    void deliver(Message msg);

    void close_session();

    void set_session_data(UserData user_data);

    bool spend_tokens(Header header);

    inline tcp::socket& socket();

    inline uint64_t id() const;

    inline bool is_authenticated() const;

    inline bool has_registered() const;

    inline void set_registered();

    inline bool is_live() const;

    inline UserData get_user_data() const;

    inline void update_elo(int new_elo, uint8_t mode_idx);

    inline bool has_matches(GameMode mode);

    inline void set_has_matches(bool value, GameMode mode);

private:
    void start_ping();

    void do_read_header();

    void do_read_body();

    void do_write();

    void handle_message();

    void handle_read_error(boost::system::error_code ec);

    void handle_write_error(boost::system::error_code ec);

    void force_close_session();

public:

private:
    asio::ssl::stream<tcp::socket> ssl_socket_;
    asio::strand<asio::io_context::executor_type> strand_;

    // We need to ensure clients are not connecting and
    // pretending to send data (slowloris style DoS), so we set this
    // timer and disconnect the session if they do not deliver
    // their data in a reasonable manner.
    asio::steady_timer read_timer_;

    // Same case, but for when we try to write data.
    asio::steady_timer write_timer_;

    // We need to ensure sessions are still active and remove
    // them if we see no activity for awhile.
    asio::steady_timer ping_timer_;
    asio::steady_timer pong_timer_;

    // Tokens related members for rate limiting.
    mutable std::mutex tokens_mutex_;
    uint64_t tokens;
    std::chrono::steady_clock::time_point last_update;

    // Unique ID for this session. Session IDs are used internally
    // so we can simply use uint64_t for our implementation.
    //
    // This value is never changed.
    const uint64_t session_id_;

    // Remains false until a login, then remains true until
    // object is destroyed.
    //
    // We want to use this to prevent calls to our database
    // when we already are authenticated.
    std::atomic<bool> authenticated_;

    // Prevent clients from trying to register many accounts.
    //
    // We may also need to monitor clients that reset connections to
    // create more accounts in our database calls.
    std::atomic<bool> called_register_;

    // For managing game queues.
    std::atomic<bool> live_;

    // For preventing unnecessary history fetching.
    //
    // Storing as bit flags could be useful, potential optimization.
    mutable std::mutex has_new_matches_mutex_;
    std::array<bool, NUMBER_OF_MODES> has_new_matches_{};

    bool awaiting_pong_;

    // Mutex to prevent needing a strand call for this data
    // since it will probably not be accessed by multiple people.
    mutable std::mutex user_data_mutex_;
    UserData user_data_;

    // Data related members.
    Header incoming_header_;
    std::vector<uint8_t> incoming_body_;
    std::deque<Message> write_queue_;

    // Callbacks.
    MessageHandler on_message_relay_;
    DisconnectHandler on_disconnect_relay_;
};

inline asio::ip::tcp::socket& Session::socket()
{
    return ssl_socket_.next_layer();
}

inline uint64_t Session::id() const
{
    return session_id_;
}

inline bool Session::is_authenticated() const
{
    return authenticated_.load(std::memory_order_acquire);
}

inline bool Session::has_registered() const
{
    return called_register_.load(std::memory_order_acquire);
}

inline void Session::set_registered()
{
    called_register_.store(true, std::memory_order_release);
    return;
}

inline bool Session::is_live() const
{
    return live_.load(std::memory_order_acquire);
}

inline UserData Session::get_user_data() const
{
    std::lock_guard lock(user_data_mutex_);
    return user_data_;
}

inline void Session::update_elo(int new_elo, uint8_t mode_idx)
{
    std::lock_guard<std::mutex> lock(user_data_mutex_);
    user_data_.matching_elos[mode_idx] = new_elo;
    return;
}

inline bool Session::has_matches(GameMode mode)
{
    std::lock_guard<std::mutex> lock(has_new_matches_mutex_);
    return has_new_matches_[static_cast<uint8_t>(mode)];
}

inline void Session::set_has_matches(bool value, GameMode mode)
{
    std::lock_guard<std::mutex> lock(has_new_matches_mutex_);
    has_new_matches_[static_cast<uint8_t>(mode)] = value;
    return;
}
