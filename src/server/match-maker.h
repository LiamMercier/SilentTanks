#pragma once

#include "match-strategy.h"
#include "match-instance.h"

class MatchMaker
{
using ptr = std::shared_ptr<Session>;

public:
    MatchMaker(asio::io_context & cntx);

    void enqueue(const ptr & p, GameMode queued_mode);

    void cancel(const ptr & p, GameMode queued_mode);

    void tick_all();

    void route_to_match(const ptr & p, const Message & msg);

private:
    void make_match_on_strand(Session::ptr p1, Session::ptr p2);

    void route_impl(const Session::ptr & p, const Message & msg);

    void forfeit_impl(const Session::ptr & p);

public:
    std::vector<GameMap> available_maps_;

private:
    // strand to serialize shared state requests
    asio::strand<asio::io_context::executor_type> global_strand_;

    // Queues for each game mode offered by the server, accessed via an array
    std::array<std::unique_ptr<IMatchStrategy>, static_cast<size_t>(GameMode::NO_MODE)> matching_queues_;

    std::unordered_map<ptr, std::shared_ptr<MatchInstance>> session_to_match_;
    std::vector<std::shared_ptr<MatchInstance>> live_matches_;
};
