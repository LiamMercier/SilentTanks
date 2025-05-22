#pragma once

#include "match-strategy.h"
#include "match-instance.h"

#include <boost/functional/hash.hpp>

class MatchMaker
{
using ptr = std::shared_ptr<Session>;
using SendCallback = std::function<void(uint64_t s_id, Message msg)>;
using ResultsCallback = std::function<void(MatchResult result)>;

public:
    MatchMaker(asio::io_context & cntx,
               SendCallback send_callback,
               ResultsCallback results_callback);

    void enqueue(const ptr & p, GameMode queued_mode);

    void cancel(const ptr & p, GameMode queued_mode, bool called_by_user);

    void tick_all();

    void route_to_match(const ptr & u_id, Message msg);

private:
    void make_match_on_strand(std::vector<Session::ptr> players,
                              MatchSettings settings);

    void route_impl(const Session::ptr & p, Message msg);

    void forfeit_impl(const Session::ptr & p, bool called_by_user);

private:
    // strand to serialize shared state requests
    asio::strand<asio::io_context::executor_type> global_strand_;

    // Queues for each game mode offered by the server, accessed via an array
    //
    // TODO: look at this, does it need to be uuid?
    //
    // We can probably keep these as session pointers since when we close
    // the client connection it would allow us to call cancel on the queue
    // and this would remove the player from the queue. Shared ptr means we will
    // still have the UUID for forfeit/cancel if necessary.
    std::array<std::unique_ptr<IMatchStrategy>, static_cast<size_t>(GameMode::NO_MODE)> matching_queues_;

    // Mapping from unique users to their matches
    std::unordered_map<boost::uuids::uuid,
                       std::shared_ptr<MatchInstance>,
                       boost::hash<boost::uuids::uuid>> uuid_to_match_;

    // TODO: find a better data structure for this, matches will end at different
    // times and thus fragment the memory.
    std::vector<std::shared_ptr<MatchInstance>> live_matches_;

    MapRepository all_maps_;

    // Necessary to create match instances with a different strand
    // to prevent having to use global_strand_ for all calls.
    asio::io_context & io_cntx_;

    // Callback to server to send a message to a session
    SendCallback send_callback_;

    ResultsCallback results_callback_;
};
