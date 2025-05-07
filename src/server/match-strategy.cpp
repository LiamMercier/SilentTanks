#include "match-strategy.h"

CasualTwoPlayerStrategy::CasualTwoPlayerStrategy(asio::io_context & cntx,
                                                 MakeMatchCallback on_match_ready,
                                                 const MapRepository & map_repo)
: strand_(cntx.get_executor()),
  on_match_ready_(std::move(on_match_ready)),
  map_repo_(map_repo)
{
}

// push the session into the deque
void CasualTwoPlayerStrategy::enqueue(Session::ptr p)
{
    asio::post(strand_,
               [this, p] {

            // if the lookup does not find this session enqueued already
            if (lookup_.find(p) == lookup_.end())
            {
                queue_.push_back(p);
                try_form_match();
            }

            });
}

void CasualTwoPlayerStrategy::cancel(Session::ptr p)
{
    asio::post(strand_, [this, p]{
        if (lookup_.erase(p))
        {
            // remove if lookup found a queued player
            //
            // this might be from the middle of the deque
            queue_.erase(std::remove(queue_.begin(), queue_.end(), p), queue_.end());
        }

    });
}

void CasualTwoPlayerStrategy::try_form_match()
{
    while (queue_.size() >= 2)
    {
        // pop player 1
        Session::ptr p1 = queue_.front();
        queue_.pop_front();

        // pop player 2
        Session::ptr p2 = queue_.front();
        queue_.pop_front();

        // erase both from lookups
        lookup_.erase(p1);
        lookup_.erase(p2);

        // get match settings
        // TODO: evaluate how to handle this properly
        GameMap map = (map_repo_.get_available_maps()[0]);
        MatchSettings settings(map, initial_time_ms, increment_ms);

        // spawn a new match instance
        on_match_ready_(std::vector<Session::ptr>{p1, p2}, settings);
    }
}
