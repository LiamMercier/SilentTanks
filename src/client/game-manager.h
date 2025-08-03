#pragma once

#include <utility>
#include <boost/asio.hpp>

#include "player-view.h"

namespace asio = boost::asio;

class GameManager
{
public:
    GameManager(asio::io_context & cntx);

    void update_view(PlayerView new_view);

    void clear_view();

private:
    asio::strand<asio::io_context::executor_type> strand_;
    PlayerView current_view_;
};
