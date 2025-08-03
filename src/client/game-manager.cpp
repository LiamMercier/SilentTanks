#include "game-manager.h"

GameManager::GameManager(asio::io_context & cntx)
:strand_(cntx.get_executor())
{

}

void GameManager::update_view(PlayerView new_view)
{
    asio::post(strand_,
        [this,
        new_view = std::move(new_view)]{
        current_view_ = new_view;
    });
}

void GameManager::clear_view()
{
    asio::post(strand_,
        [this]{
        current_view_ = PlayerView();
    });
}
