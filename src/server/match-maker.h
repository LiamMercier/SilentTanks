#pragma once

#include "match-strategy.h"
#include "match-instance.h"

class MatchMaker
{
using ptr = std::shared_ptr<Session>;
using SendCallback = std::function<void(uint64_t s_id, Message msg)>;

public:
    MatchMaker(asio::io_context & cntx, SendCallback send_callback);

    void enqueue(const ptr & p, GameMode queued_mode);

    void cancel(const ptr & p, GameMode queued_mode);

    void tick_all();

    void route_to_match(const ptr & p, Message msg);

private:
    void make_match_on_strand(std::vector<Session::ptr> players,
                              MatchSettings settings);

    void route_impl(const Session::ptr & p, Message msg);

    void forfeit_impl(const Session::ptr & p);

private:
    // strand to serialize shared state requests
    asio::strand<asio::io_context::executor_type> global_strand_;

    // Queues for each game mode offered by the server, accessed via an array
    std::array<std::unique_ptr<IMatchStrategy>, static_cast<size_t>(GameMode::NO_MODE)> matching_queues_;

    // TODO: change this to user IDs or similar.
    std::unordered_map<ptr, std::shared_ptr<MatchInstance>> session_to_match_;

    // TODO: find a better data structure for this, matches will end at different
    // times and thus fragment the memory.
    std::vector<std::shared_ptr<MatchInstance>> live_matches_;

    MapRepository all_maps_;

    // Necessary to create match instances with a different strand
    // to prevent having to use global_strand_ for all calls.
    asio::io_context & io_cntx_;

    // Callback to server to send a message to a session
    SendCallback send_callback_;
};
