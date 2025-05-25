#pragma once

#include <memory>
#include <unordered_set>
#include <utility>

#include <boost/asio.hpp>

#include "session.h"
#include "message.h"
#include "maps.h"
#include "match-settings.h"

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

// Classic 1 vs 1 queue
class CasualTwoPlayerStrategy : public IMatchStrategy
{
public:

    CasualTwoPlayerStrategy(boost::asio::io_context & cntx,
                            MakeMatchCallback on_match_ready,
                            const MapRepository & map_repo);

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
    std::unordered_set<Session::ptr> lookup_;

    // map repository
    const MapRepository & map_repo_;
};
