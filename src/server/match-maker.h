#pragma once

#include "match-strategy.h"
#include "match-instance.h"
#include "map-repository.h"

#include <boost/functional/hash.hpp>

class UserManager;

class MatchMaker
{
using ptr = std::shared_ptr<Session>;
using SendCallback = std::function<void(uint64_t s_id, Message msg)>;
using ResultsCallback = std::function<void(MatchResult result)>;

public:
    MatchMaker(asio::io_context & cntx,
               std::string map_file_name,
               SendCallback send_callback,
               ResultsCallback recorder_callback,
               std::shared_ptr<UserManager> user_manager);

    void enqueue(const ptr & p, GameMode queued_mode);

    void cancel(const ptr & p, GameMode queued_mode, bool called_by_user);

    void tick_all();

    void route_to_match(const ptr & u_id, Message msg);

    void forfeit(const Session::ptr & p);

    void send_match_message(const Session::ptr & p,
                            boost::uuids::uuid user_id,
                            InternalMatchMessage msg);

private:
    void make_match_on_strand(std::vector<Session::ptr> players,
                              MatchSettings settings);

    void route_impl(const Session::ptr & p, Message msg);

    void forfeit_impl(const Session::ptr & p);

private:
    // strand to serialize shared state requests
    asio::strand<asio::io_context::executor_type> global_strand_;

    // Queues for each game mode offered by the server, accessed via an array
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

    // Temporary match ID to instance mapping
    // TODO: check this doesn't exist before allowing to enqueue
    std::unordered_map<uint64_t, std::shared_ptr<MatchInstance>> live_matches_;

    // TODO: track queued user IDs.

    // Map repository.
    std::shared_ptr<MapRepository> all_maps_;

    // Necessary to create match instances with a different strand
    // to prevent having to use global_strand_ for all calls.
    asio::io_context & io_cntx_;

    // Note: this is only an internal value, the database match
    // ID is managed by postgreSQL and will differ.
    uint64_t next_match_id_{0};

    // Callback to server to send a message to a session
    SendCallback send_callback_;

    ResultsCallback recorder_callback_;

    std::shared_ptr<UserManager> user_manager_;
};
