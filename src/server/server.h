#pragma once

#include "match-maker.h"

class Server
{
    using tcp = asio::ip::tcp;
    using ptr = std::shared_ptr<Session>;

public:
    // construct the server given a context and an endpoint
    Server(asio::io_context & cntx, tcp::endpoint endpoint);

    void do_accept();

private:
    void remove_session(const ptr & session);

    void on_message(const ptr & session, Message msg);

private:
    // strand to prevent race conditions on session removal and addition.
    asio::strand<asio::io_context::executor_type> server_strand_;

    tcp::acceptor acceptor_;
    // Rare case where we don't want an array style structure.
    //
    // If many sessions are opened, it becomes hard to find sessions and
    // we can experience memory fragmentation if many sessions are opened
    // and closed between server reboots.
    //
    // By using an unordered set we end up preventing this, though, we
    // get worse cache locality of course.
    //
    // This trade off is perfectly reasonable for a server however.
    std::unordered_set<ptr> sessions_;

    // class to handle matching players who want to play against one another
    MatchMaker matcher_;

};
