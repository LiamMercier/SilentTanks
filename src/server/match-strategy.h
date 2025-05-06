#pragma once

#include <memory>
#include <unordered_set>

#include <asio.hpp>

#include "session.h"
#include "message.h"

class IMatchStrategy
{
public:
    using MakeMatchCallback = std::function<void(Session::ptr, Session::ptr)>;

    virtual ~IMatchStrategy() = default;

    // enqueue player for this particular game mode
    virtual void enqueue(Session::ptr p) = 0;

    // cancel or remove a queued player
    virtual void cancel(Session::ptr p) = 0;

    // tick strategies to change elo windows
    virtual void tick() = 0;
};

// Classic 1 vs 1 queue
class CasualTwoPlayerStrategy : public IMatchStrategy
{
public:

    CasualTwoPlayerStrategy(asio::io_context & cntx, MakeMatchCallback on_match_ready);

    void enqueue(Session::ptr p) override;

    void cancel(Session::ptr p) override;

    // do nothing, casual uses a deque to automatically match
    //
    // strategies must use their strand if necessary
    void tick() override
    {

    }
private:
    void try_form_match();

private:
    // strand for this game mode to manage async queue/cancel attempts
    asio::strand<asio::io_context::executor_type> strand_;

    MakeMatchCallback on_match_ready_;

    // simple FIFO queue for casual matching
    std::deque<Session::ptr> queue_;

    // lookup to see if player is queued
    std::unordered_set<Session::ptr> lookup_;

};
