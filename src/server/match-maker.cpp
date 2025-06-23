#include "match-maker.h"
#include "user-manager.h"

MatchMaker::MatchMaker(asio::io_context & cntx,
                       SendCallback send_callback,
                       ResultsCallback recorder_callback,
                       std::shared_ptr<UserManager> user_manager)
:global_strand_(cntx.get_executor()),
 all_maps_(std::vector<GameMap>
        {
        GameMap(std::string("envs/test2.txt"), 12, 8, 2)
        }),
 io_cntx_(cntx),
 send_callback_(std::move(send_callback)),
 recorder_callback_(std::move(recorder_callback)),
 user_manager_(user_manager)
{
    // Create a callback to make a match when the match strategy
    // finds valid players.
    auto match_call_back = [this](std::vector<Session::ptr> players, MatchSettings settings)
    {
        make_match_on_strand(std::move(players), settings);
    };

    matching_queues_[static_cast<size_t>(GameMode::ClassicTwoPlayer)] = std::make_unique<CasualTwoPlayerStrategy>(cntx, match_call_back, all_maps_);

    // other matchmaking options go here
    //
    // TODO: change this to the proper option when implemented
    matching_queues_[static_cast<size_t>(GameMode::RankedTwoPlayer)] = std::make_unique<CasualTwoPlayerStrategy>(cntx, match_call_back, all_maps_);

}

void MatchMaker::enqueue(const ptr & p, GameMode queued_mode)
{
    // queue up player for the given game mode
    //
    // matching_queues_[queued_mode] handles concurrency on its strand
    matching_queues_[static_cast<size_t>(queued_mode)]->enqueue(p);
}


// The user may request to cancel before the game starts, but
// before we get the request the match might begin.
//
// This should be treated as the user having a request to
// cancel, instead of blindly cancelling. If the request
// is not able to be followed through, we tell the client
// that their game has started and they must play or forfeit.
void MatchMaker::cancel(const ptr & p, GameMode queued_mode, bool called_by_user)
{
    // call matching cancel function for this game mode
    //
    // matching_queues_[queued_mode] handles concurrency on its strand
    matching_queues_[static_cast<size_t>(queued_mode)]->cancel(p);

    // Handle match cancels if a match is currently ongoing.
    asio::post(global_strand_, [this, p, queued_mode, called_by_user]{

        // Handle user cancels if the match is already ongoing.
        if (called_by_user == true)
        {
            auto it = uuid_to_match_.find((p->get_user_data()).user_id);
            // If a match exists, tell them they must forfeit
            // since the game started already.
            if (it != uuid_to_match_.end())
            {
                // Notify of bad cancel.
                Message bad_cancel;
                bad_cancel.create_serialized(HeaderType::BadCancel);
                send_callback_(p->id(), bad_cancel);
            }

            return;
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

void MatchMaker::forfeit(const Session::ptr & p)
{
    asio::post(global_strand_, [this, p]
    {
       forfeit_impl(p);
    });
}

void MatchMaker::send_match_message(boost::uuids::uuid sender,
                                    InternalMatchMessage msg)
{
    asio::post(global_strand_,
            [this,
            sender,
            msg = std::move(msg)]{

        // Find the sender's match.
        auto match = uuid_to_match_.find(sender);

        if (match != uuid_to_match_.end())
        {
            // Route to found match instance
            match->second->match_message(sender, std::move(msg));
        }
        else
        {
            // TODO: Can't find match to send message to, notify user.
        }

    });
}

void MatchMaker::make_match_on_strand(std::vector<Session::ptr> players,
                                      MatchSettings settings)
{
    asio::post(global_strand_, [this, players, settings]{

        std::cout << "Creating match with session IDs: "
        << players[0]->id() << " " << players[1]->id() << "\n";

        std::vector<Session::ptr> live_players;
        live_players.reserve(players.size());

        // Ensure a valid game is about to commence.
        for (uint8_t i = 0; i < players.size(); i++)
        {
            Session::ptr p = players[i];

            // If still live, add to the match
            if(p->is_live())
            {
                live_players.push_back(p);
            }

            // Else, cancel the match since a user disconnected
            // before the match started.
            else
            {
                break;
            }
        }

        // Match cancel logic
        if (live_players.size() < players.size())
        {
            // iterate through the vector
            for (uint8_t j = 0; j < live_players.size(); j++)
            {
                Message queue_dropped;
                queue_dropped.create_serialized(HeaderType::QueueDropped);
                send_callback_(live_players[j]->id(), queue_dropped);
            }

            return;
        }

        // Setup PlayerInfo structs.
        std::vector<PlayerInfo> player_list;
        player_list.reserve(players.size());

        // Setup players list from our session data.
        for (uint8_t p_id = 0; p_id < players.size(); p_id++)
        {
            player_list.emplace_back
            (
                PlayerInfo
                (
                    p_id,
                    players[p_id]->id(),
                    (players[p_id]->get_user_data()).user_id
                )
             );

            // Tell the player that the game is starting.
            Message notify_match;
            MatchStartNotification notification;
            notification.player_id = p_id;
            notify_match.create_serialized(notification);
            send_callback_(player_list[p_id].session_id, notify_match);
        }

        // Setup message callback.
        auto game_message_cb = [this](boost::uuids::uuid user_id,
                                      InternalMatchMessage msg)
        {
            this->user_manager_->match_message_user(user_id, msg);
        };

        auto new_inst = std::make_shared<MatchInstance>(io_cntx_,
                                                        settings,
                                                        player_list,
                                                        player_list.size(),
                                                        send_callback_,
                                                        game_message_cb);
        next_match_id_++;

        // Setup the results callback to handle deletion.
        auto results_cb = [this, m_id = next_match_id_](MatchResult result) mutable
        {
           asio::post(global_strand_,
            [this, result = std::move(result), m_id]{

                // Remove UUID to match mapping and notify
                // the user manager.
                for (size_t i = 0; i < result.user_ids.size(); i++)
                {
                    boost::uuids::uuid u_id = result.user_ids[i];
                    uuid_to_match_.erase(u_id);
                    user_manager_->notify_match_finished(u_id);
                }

                // Remove from live matches
                live_matches_.erase(m_id);

                // Push off the rest of the work to the match recorder
                recorder_callback_(std::move(result));

            });
        };
        // End results callback lambda

        // Add players to uuid_to_match_ mapping and
        // inform the user manager.
        for (uint8_t i = 0; i < player_list.size(); i++)
        {
            boost::uuids::uuid uuid = player_list[i].user_id;
            uuid_to_match_[uuid] = new_inst;
            user_manager_->notify_match_start(uuid, new_inst);
        }

        // set the instance's results callback using the weak_ptr
        // to allow us to quickly remove it when the game ends.
        new_inst->set_results_callback(std::move(results_cb));

        live_matches_[next_match_id_] = new_inst;

        // start the instance, it is now async.
        new_inst->start();
    });

}

// Decode into a command and send to the server
void MatchMaker::route_impl(const Session::ptr & p, Message msg)
{
    auto match = uuid_to_match_.find((p->get_user_data()).user_id);

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
    const auto & u_id = (p->get_user_data()).user_id;

    // tell the match instance that we are forfeiting
    auto inst = uuid_to_match_[u_id];

    inst->forfeit(u_id);

    // Remove from the map and notify the user manager.
    uuid_to_match_.erase(u_id);

    user_manager_->notify_match_finished(u_id);
}
