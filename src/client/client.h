#pragma once

#include "client-session.h"
#include "client-state.h"

class Client
{
public:
    using ptr = ClientSession::ptr;
public:
    Client(asio::io_context & cntx);

    void connect(std::string endpoint);

private:
    void on_message(const ptr & session, Message msg);

    void on_disconnect();

private:
    asio::io_context & io_context_;
    asio::strand<asio::io_context::executor_type> client_strand_;

    ClientState state_;

    ClientSession::ptr current_session_;

    // TODO: client data.

    // TODO: friends list

    // TODO: game manager
};
