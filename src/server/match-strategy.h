#pragma once

#include <memory>
#include <unordered_set>
#include <utility>

#include <boost/asio.hpp>
#include <boost/functional/hash.hpp>

#include "session.h"
#include "message.h"
#include "maps.h"
#include "map-repository.h"
#include "match-settings.h"
#include "elo-updates.h"

class IMatchStrategy
{
public:
    using MakeMatchCallback = std::function<void(std::vector<Session::ptr>,
                                                 MatchSettings settings)>;

    virtual ~IMatchStrategy() = default;

    // Enqueue player for this particular game mode.
    //
    // Must be idempotent.
    virtual void enqueue(Session::ptr p) = 0;

    // Cancel or remove a queued player
    //
    // Must be idempotent.
    virtual void cancel(Session::ptr p) = 0;

    // tick strategies to change elo windows
    virtual void tick() = 0;
};

// TODO: <refactoring>
// It would probably be better to make a CasualStrategy and give it
// an abstract method for matching players if the number of game modes
// were to quickly expand in the future.

// Classic 1 vs 1 queue
class CasualTwoPlayerStrategy : public IMatchStrategy
{
public:

    CasualTwoPlayerStrategy(boost::asio::io_context & cntx,
                            MakeMatchCallback on_match_ready,
                            const std::shared_ptr<MapRepository> & map_repo);

    void enqueue(Session::ptr p) override;

    void cancel(Session::ptr p) override;

    // Do nothing, casual uses a deque to automatically match.
    void tick() override
    {

    }
private:
    void try_form_match();

public:
    // 20 minute clock
    static constexpr uint64_t initial_time_ms = 1200000;
    // 1 second increment
    static constexpr uint64_t increment_ms = 1000;

private:
    // strand for this game mode to manage async queue/cancel attempts
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    MakeMatchCallback on_match_ready_;

    // simple FIFO queue for casual matching
    std::deque<Session::ptr> queue_;

    // lookup to see if player is queued
    std::unordered_set<boost::uuids::uuid,
                       boost::hash<boost::uuids::uuid>> lookup_;

    // map repository
    const std::shared_ptr<MapRepository> & map_repo_;
};

// Classic 3 player FFA queue
class CasualThreePlayerStrategy : public IMatchStrategy
{
public:

    CasualThreePlayerStrategy(boost::asio::io_context & cntx,
                              MakeMatchCallback on_match_ready,
                              const std::shared_ptr<MapRepository> & map_repo);

    void enqueue(Session::ptr p) override;

    void cancel(Session::ptr p) override;

    // Do nothing, casual uses a deque to automatically match.
    void tick() override
    {

    }
private:
    void try_form_match();

public:
    // 20 minute clock
    static constexpr uint64_t initial_time_ms = 1200000;
    // 1 second increment
    static constexpr uint64_t increment_ms = 1000;

private:
    // strand for this game mode to manage async queue/cancel attempts
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    MakeMatchCallback on_match_ready_;

    // simple FIFO queue for casual matching
    std::deque<Session::ptr> queue_;

    // lookup to see if player is queued
    std::unordered_set<boost::uuids::uuid,
                       boost::hash<boost::uuids::uuid>> lookup_;

    // map repository
    const std::shared_ptr<MapRepository> & map_repo_;
};

// Classic 3 player FFA queue
class CasualFivePlayerStrategy : public IMatchStrategy
{
public:

    CasualFivePlayerStrategy(boost::asio::io_context & cntx,
                             MakeMatchCallback on_match_ready,
                             const std::shared_ptr<MapRepository> & map_repo);

    void enqueue(Session::ptr p) override;

    void cancel(Session::ptr p) override;

    // Do nothing, casual uses a deque to automatically match.
    void tick() override
    {

    }
private:
    void try_form_match();

public:
    // 20 minute clock
    static constexpr uint64_t initial_time_ms = 1200000;
    // 1 second increment
    static constexpr uint64_t increment_ms = 1000;

private:
    // strand for this game mode to manage async queue/cancel attempts
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    MakeMatchCallback on_match_ready_;

    // simple FIFO queue for casual matching
    std::deque<Session::ptr> queue_;

    // lookup to see if player is queued
    std::unordered_set<boost::uuids::uuid,
                       boost::hash<boost::uuids::uuid>> lookup_;

    // map repository
    const std::shared_ptr<MapRepository> & map_repo_;
};

// Ranked 1 vs 1 queue.
//
// We use bucket based match making for ranked queues. The elo range
// [ELO_FLOOR, MAX_ELO_BUCKET) is sliced up into buckets, with an extra bucket
// for everyone with elo at or equal to MAX_ELO_BUCKET.
//
// Players are matched up within their bucket whenever possible. Otherwise,
// if a bucket B_i has only one player, we scan the buckets B_(i-delta) through
// B_(i+delta) where delta depends on the time a player spends in the queue.
//
// So with min = 500, bucket size 25, the bucket B_0 has range [500, 525),
// bucket B_1 has range [525, 550), etc.
//
// To compute the index given elo, we use (elo - MIN_ELO) / (BUCKET_SIZE)
// and take the floor.
class RankedTwoPlayerStrategy : public IMatchStrategy
{
public:
    struct QueueEntry
    {
        Session::ptr session;
        std::chrono::steady_clock::time_point enqueued_at;
    };

    struct LookupData
    {
        size_t bucket_index;
        std::list<QueueEntry>::iterator iter;
    };

    // Set the cutoff for when elo  gets put into the last bucket.
    inline static constexpr int MAX_ELO_BUCKET = 3000;

    // Compute ranges, get bucket size.
    inline static constexpr int ELO_RANGE = MAX_ELO_BUCKET - ELO_FLOOR;
    inline static constexpr int NUM_BUCKETS = 100;
    inline static constexpr int BUCKET_SIZE = ELO_RANGE / NUM_BUCKETS;

    // Enforce bounds on bucket searching.
    inline static constexpr int MAX_ELO_DIFF = 400;
    inline static constexpr int MAX_BUCKETS_DIFF = MAX_ELO_DIFF / BUCKET_SIZE;

    // Update bounds every 30 seconds.
    inline static constexpr std::chrono::seconds BUCKET_INCREMENT_TIME{15};

    inline static constexpr GameMode curr_gamemode = GameMode::RankedTwoPlayer;

    // Ensure divisibility.
    static_assert(ELO_RANGE % NUM_BUCKETS == 0);
    static_assert(MAX_ELO_DIFF % BUCKET_SIZE == 0);

public:
    RankedTwoPlayerStrategy(boost::asio::io_context & cntx,
                            MakeMatchCallback on_match_ready,
                            const std::shared_ptr<MapRepository> & map_repo);

    void enqueue(Session::ptr p) override;

    void cancel(Session::ptr p) override;

    // Handle logic on each matching tick.
    void tick() override;

private:
    void try_form_match();

    void match_and_remove(Session::ptr p1, Session::ptr p2);

    size_t elo_to_bucket_index(int elo);

public:
    // 20 minute clock
    inline static constexpr uint64_t initial_time_ms = 1200000;
    // 1 second increment
    inline static constexpr uint64_t increment_ms = 1000;

private:
    // Strand for this game mode to manage async queue/cancel attempts
    boost::asio::strand<boost::asio::io_context::executor_type> strand_;

    // Callback to MatchMaker's make_match_on_strand function.
    MakeMatchCallback on_match_ready_;

    const std::shared_ptr<MapRepository> & map_repo_;

    // Members for matching players.
    std::vector<std::list<QueueEntry>> elo_buckets_;

    // Map from UUID to the respective bucket index and list iterator.
    std::unordered_map<boost::uuids::uuid,
                       LookupData,
                       boost::hash<boost::uuids::uuid>> lookup_;
};
