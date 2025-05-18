#include "match-maker.h"

MatchMaker::MatchMaker(asio::io_context & cntx,
                       SendCallback send_callback,
                       ResultsCallback results_callback)
:global_strand_(cntx.get_executor()),
 all_maps_(std::vector<GameMap>
        {
        GameMap(std::string("envs/test2.txt"), 12, 8, 2)
        }),
 io_cntx_(cntx),
 send_callback_(std::move(send_callback)),
 results_callback_(std::move(results_callback))
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

        auto it = uuid_to_match_.find((p->get_user_data()).user_id);
        if (it != session_to_match_.end())
        {
            // TODO: bool Called_By_User or something
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
void MatchMaker::route_to_match(const ptr & p, Message msg)
{
    asio::post(global_strand_, [this, p, m = std::move(msg)]
    {
       route_impl(p, m);
    });
}

void MatchMaker::make_match_on_strand(std::vector<Session::ptr> players,
                                      MatchSettings settings)
{
    asio::post(global_strand_, [this, players, settings]{

        std::cout << "Creating match with session IDs: "
        << players[0]->id() << " " << players[1]->id() << "\n";

        // setup PlayerInfo structs
        std::vector<PlayerInfo> player_list;
        player_list.reserve(players.size());

        // TODO uuid's instead
        for (uint8_t p_id = 0; p_id < players.size(); p_id++)
        {
            player_list.emplace_back(PlayerInfo(p_id, players[p_id]->id()));

            Message notify_match;
            MatchStartNotification notification;
            notification.player_id = p_id;

            notify_match.create_serialized(notification);

            // also tell the player that the game is starting
            send_callback_(players[p_id]->id(), notify_match);
        }

        // setup instance
        auto new_inst = std::make_shared<MatchInstance>(io_cntx_,
                                                        settings,
                                                        player_list,
                                                        player_list.size(),
                                                        send_callback_,
                                                        results_callback_);

        // Add players to session_to_match_
        //
        // TODO: route this to the player manager, possibly through session?
        for (uint8_t i = 0; i < player_list.size(); i++)
        {
            uuid_to_match_[players[i]] = new_inst;
        }

        live_matches_.push_back(new_inst);

        // start the instance
        new_inst->start();
    });

}

// Decode into a command and send to the server
void MatchMaker::route_impl(const Session::ptr & p, Message msg)
{
    auto match = uuid_to_match_.find((p->user_data).user_id);

    if (match != uuid_to_match_.end())
    {
        // Convert message to command
        Command cmd_to_send = msg.to_command();

        // Route to found match instance
        match->second->receive_command(p->id(), std::move(cmd_to_send));
    }
    else
    {
        // Cant find match to cancel, notify user
        Message no_match_msg;
        no_match_msg.create_serialized(HeaderType::NoMatchFound);
        p->deliver(std::move(no_match_msg));
    }
}

void MatchMaker::forfeit_impl(const Session::ptr & p)
{
    auto & u_id = (p->user_data).user_id

    // tell the match instance that we are forfeiting
    auto inst = uuid_to_match_[u_id];

    // TODO: make a forfeit function for MatchInstance
    //inst->forfeit(p->uuid);

    // remove from map
    uuid_to_match_.erase(u_id);
}
