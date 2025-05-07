#include "match-maker.h"

MatchMaker::MatchMaker(asio::io_context & cntx)
:global_strand_(cntx.get_executor()),
 all_maps_(std::vector<GameMap>
        {
        GameMap(std::string("envs/test2.txt"), 12, 8, 2)
        })
{
    auto match_call_back = [this](std::vector<Session::ptr> players, MatchSettings settings)
    {
        make_match_on_strand(std::move(players), settings);
    };

    matching_queues_[static_cast<size_t>(GameMode::ClassicTwoPlayer)] = std::make_unique<CasualTwoPlayerStrategy>(cntx, match_call_back, all_maps_);

    // other matchmaking options go here

}

void MatchMaker::enqueue(const ptr & p, GameMode queued_mode)
{
    // queue up player for the given game mode
    //
    // matching_queues_[queued_mode] handles concurrency on its strand
    matching_queues_[static_cast<size_t>(queued_mode)]->enqueue(p);
}

void MatchMaker::cancel(const ptr & p, GameMode queued_mode)
{
    // call matching cancel function for this game mode
    //
    // matching_queues_[queued_mode] handles concurrency on its strand
    matching_queues_[static_cast<size_t>(queued_mode)]->cancel(p);

    // handle match cancels if a match is currently ongoing.
    asio::post(global_strand_, [this, p]{

        auto it = session_to_match_.find(p);
        if (it != session_to_match_.end())
        {
            forfeit_impl(p);
        }

    });
}

void MatchMaker::tick_all()
{
    // tick all game modes
    for (int i = 0; i < static_cast<int>(GameMode::NO_MODE); i++)
    {
      matching_queues_[i]->tick();
    }
}

// Public facing function to keep strand logic and routing logic separate
void MatchMaker::route_to_match(const ptr & p, const Message & msg)
{
    asio::post(global_strand_, [this, p, msg]
    {
       route_impl(p, msg);
    });
}

// TODO: coherent instance creation
void MatchMaker::make_match_on_strand(std::vector<Session::ptr> players,
                                      MatchSettings settings)
{
    asio::post(global_strand_, [this, players, settings]{

        // setup PlayerInfo structs
        std::vector<PlayerInfo> player_list;
        player_list.reserve(players.size());

        for (uint8_t id = 0; id < players.size(); id++)
        {
            player_list.emplace_back(PlayerInfo(id));
        }

        // setup instance
        auto new_inst = std::make_shared<MatchInstance>(settings,
                                                        player_list,
                                                        player_list.size());

        // Add players to session_to_match_
        for (uint8_t i = 0; i < player_list.size(); i++)
        {
            session_to_match_[players[i]] = new_inst;
        }

        live_matches_.push_back(new_inst);

        // start the instance
        // new_inst->start
    });

}

void MatchMaker::route_impl(const Session::ptr & p, const Message & msg)
{
    auto it = session_to_match_.find(p);

    if (it != session_to_match_.end())
    {
        // TODO: link this to the MatchInstance queue command
        //it->second->enqueue_command(p, msg);
    }
    else
    {
        // cant find match
        // TODO: cannot find match callback to session
    }
}

void MatchMaker::forfeit_impl(const Session::ptr & p)
{
    // tell the match instance that we are forfeiting
    auto inst = session_to_match_[p];

    // TODO: make a forfeit function for MatchInstance
    //inst->forfeit(p);

    // remove from map
    session_to_match_.erase(p);

    // TODO: notify player's session and possibly opponent
}
