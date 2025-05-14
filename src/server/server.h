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
    // Strand to prevent race conditions on session removal and addition.
    asio::strand<asio::io_context::executor_type> server_strand_;

    // Acceptor bound to an endpoint.
    tcp::acceptor acceptor_;

    // Rare case where we don't want an array style structure.
    //
    // If many sessions are opened, it becomes hard to find sessions and
    // we can experience memory fragmentation if many sessions are opened
    // and closed between server reboots.
    //
    // By using an unordered map we end up preventing this, though, we
    // get worse cache locality of course.
    //
    // This trade off is perfectly reasonable for a server however.
    std::unordered_map<uint64_t, ptr> sessions_;

    // TODO: session_to_user_ taking session ID -> user ID. (need NO_USER on init)
    // TODO: implement users and user IDs.
    //
    //       We want the Server to be responsible for mapping sockets
    //       to more durable objects like users.

    // Class to handle matching players who want to play against one another
    MatchMaker matcher_;

    // TODO: login manager class
    //       We would prefer to hand off login requests to a login manager
    //       just like we did with match making requests.

    // Increasing counter for the next session ID.
    uint64_t next_session_id_{1};

};
