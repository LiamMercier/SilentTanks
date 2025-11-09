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

#include "client-constants.h"
#include "client-session.h"
#include "client-state.h"
#include "client-data.h"
#include "player-view.h"
#include "popup.h"

class Client
{
public:
    using ptr = ClientSession::ptr;

    using LoginCallback = std::function<void(std::string username)>;

    using StateChangeCallback = std::function<void(ClientState state)>;

    using PopupCallback = std::function<void(Popup p, bool urgent)>;

    using UsersUpdatedCallback = std::function<void(const UserMap & map,
                                                    UserListType type)>;

    using QueueUpdateCallback = std::function<void(GameMode mode)>;

    using DisplayMessageCallback = std::function<void(std::string message)>;

    using MatchHistoryCallback = std::function<void(MatchResultList results)>;

    using ViewUpdateCallback = std::function<void(PlayerView new_view)>;

    using MatchDataCallback = std::function<void(StaticMatchData data)>;

    using MatchReplayCallback = std::function<void(MatchReplay replay)>;

    using ShutdownCallback = std::function<void()>;
public:
    Client(asio::io_context & cntx,
           LoginCallback login_callback,
           StateChangeCallback state_change_callback,
           PopupCallback popup_callback,
           UsersUpdatedCallback users_updated_callback,
           QueueUpdateCallback queue_update_callback,
           DisplayMessageCallback display_message_callback,
           MatchHistoryCallback match_history_callback,
           ViewUpdateCallback view_callback,
           MatchDataCallback match_data_callback,
           MatchReplayCallback match_replay_callback,
           ShutdownCallback shutdown_callback);

    inline ClientState get_state() const;

    inline void change_state(ClientState new_state);

    inline int get_elo(GameMode mode);

    void connect(ServerIdentity identity);

    void login(std::string username, std::string password);

    void register_account(std::string username, std::string password);

    void request_user_list(UserListType list_type);

    void send_friend_request(std::string username);

    void respond_friend_request(boost::uuids::uuid user_id,
                                bool decision);

    void send_unfriend_request(boost::uuids::uuid user_id);

    void send_block_request(std::string username);

    void send_unblock_request(boost::uuids::uuid user_id);

    void queue_request(GameMode mode);

    void cancel_request(GameMode mode);

    void send_command(Command cmd);

    void forfeit_request();

    void interpret_message(std::string message);

    void fetch_match_history(GameMode mode);

    void request_match_replay(uint64_t match_id);

    void shutdown();

private:
    void send_direct_message(std::string text,
                             boost::uuids::uuid receiver);

    void send_match_message(std::string text);

    void on_message(const ptr & session, Message msg);

    void on_connection();

    void on_disconnect();

private:
    asio::io_context & io_context_;
    asio::strand<asio::io_context::executor_type> client_strand_;

    mutable std::mutex state_mutex_;
    ClientState state_;

    std::atomic<bool> playing_{true};

    mutable std::mutex data_mutex_;
    ClientData client_data_;
    GameMode last_queued_mode_{GameMode::NO_MODE};

    ClientSession::ptr current_session_;

    std::atomic<bool> shutting_down_{false};

    LoginCallback login_callback_;
    StateChangeCallback state_change_callback_;
    PopupCallback popup_callback_;
    UsersUpdatedCallback users_updated_callback_;
    QueueUpdateCallback queue_update_callback_;
    DisplayMessageCallback display_message_callback_;
    MatchHistoryCallback match_history_callback_;
    ViewUpdateCallback view_callback_;
    MatchDataCallback match_data_callback_;
    MatchReplayCallback match_replay_callback_;
    ShutdownCallback shutdown_callback_;
};

inline ClientState Client::get_state() const
{
    std::lock_guard lock(state_mutex_);
    return state_;
}

inline void Client::change_state(ClientState new_state)
{
    {
        std::lock_guard lock(state_mutex_);
        state_ = new_state;
    }

    state_change_callback_(new_state);
}

inline int Client::get_elo(GameMode mode)
{
    int elo = 0;
    if (static_cast<uint8_t>(mode) < RANKED_MODES_START)
    {
        return elo;
    }
    else
    {
        uint8_t indx = elo_ranked_index(mode);
        {
            std::lock_guard lock(data_mutex_);
            elo = client_data_.display_elos[indx];
        }
        return elo;
    }
}
