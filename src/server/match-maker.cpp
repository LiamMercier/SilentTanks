#include "match-maker.h"
#include "user-manager.h"
#include "console.h"

MatchMaker::MatchMaker(asio::io_context & cntx,
                       std::string map_file_name,
                       SendCallback send_callback,
                       ResultsCallback recorder_callback,
                       std::shared_ptr<UserManager> user_manager)
:global_strand_(cntx.get_executor()),
 all_maps_(std::make_unique<MapRepository>()),
 io_cntx_(cntx),
 send_callback_(std::move(send_callback)),
 recorder_callback_(std::move(recorder_callback)),
 user_manager_(user_manager),
 tick_timer_(cntx)
{
    all_maps_->load_map_file(map_file_name);

    // Create a callback to make a match when the match strategy
    // finds valid players.
    auto match_call_back = [this](std::vector<Session::ptr> players, MatchSettings settings)
    {
        make_match_on_strand(std::move(players), settings);
    };

    // Setup match making strategies.
    matching_queues_[static_cast<size_t>(GameMode::ClassicTwoPlayer)] = std::make_unique<CasualTwoPlayerStrategy>(cntx, match_call_back, all_maps_);

    matching_queues_[static_cast<size_t>(GameMode::ClassicThreePlayer)] = std::make_unique<CasualThreePlayerStrategy>(cntx, match_call_back, all_maps_);

    matching_queues_[static_cast<size_t>(GameMode::ClassicFivePlayer)] = std::make_unique<CasualFivePlayerStrategy>(cntx, match_call_back, all_maps_);

    matching_queues_[static_cast<size_t>(GameMode::RankedTwoPlayer)] = std::make_unique<RankedTwoPlayerStrategy>(cntx, match_call_back, all_maps_);

    // Start tick timer.
    start_tick_loop();
}

void MatchMaker::enqueue(const Session::ptr & p, GameMode queued_mode)
{
    // Check that the user is not currently in a game
    auto user_id = (p->get_user_data()).user_id;
    auto match = uuid_to_match_.find(user_id);

    // If in a match already, do not queue.
    if (match != uuid_to_match_.end())
    {
        Message bad_queue;
        bad_queue.create_serialized(HeaderType::BadQueue);
        p->deliver(bad_queue);
        return;
    }

    // Now check if the player is already queued for a game.
    auto mode = uuid_to_gamemode_.find(user_id);

    // Reject if currently in queue.
    if (mode != uuid_to_gamemode_.end())
    {
        Message bad_queue;
        bad_queue.create_serialized(HeaderType::BadQueue);
        p->deliver(bad_queue);
        return;
    }

    // Otherwise, if we are going to queue we should add this to the data
    // structure for future lookups.
    uuid_to_gamemode_[user_id] = queued_mode;

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
void MatchMaker::cancel(const Session::ptr & p,
                        GameMode queued_mode,
                        bool called_by_user)
{
    // Remove the user from the map of queued users.
    auto user_id = (p->get_user_data()).user_id;
    uuid_to_gamemode_.erase(user_id);

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
    Console::instance().log("Ticking all matchers!", LogLevel::INFO);

    // tick all game modes
    for (int i = 0; i < static_cast<int>(GameMode::NO_MODE); i++)
    {
      matching_queues_[i]->tick();
    }
}

// Public facing function to keep strand logic and routing logic separate
void MatchMaker::route_to_match(const Session::ptr & p, Message msg)
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

void MatchMaker::send_match_message(const Session::ptr & p,
                                    boost::uuids::uuid user_id,
                                    InternalMatchMessage msg)
{
    asio::post(global_strand_,
            [this,
            session = p,
            u_id = user_id,
            msg = std::move(msg)]{

        // Find the sender's match.
        auto match = uuid_to_match_.find(u_id);

        if (match != uuid_to_match_.end())
        {
            // Route to found match instance
            match->second->match_message(u_id, std::move(msg));
        }
        else
        {
            // Can't find match to send message to, notify user.
            Message match_over;
            match_over.create_serialized(HeaderType::NoMatchFound);

            session->deliver(match_over);
        }

    });
}

void MatchMaker::async_shutdown(std::function<void()> on_done)
{
    asio::post(global_strand_,
            [this,
            on_done = std::move(on_done)]{

            // Only call once.
            if (shutting_down_)
            {
                return;
            }

            shutting_down_ = true;

            // Since strand calls are ordered, we can shutdown all
            // the instances and then call back.
            for (auto & key : live_matches_)
            {
                key.second->async_shutdown();
            }

            tick_timer_.cancel();

            on_done();
    });
}

void MatchMaker::start_tick_loop()
{
    tick_timer_.expires_after(std::chrono::seconds(TICK_INTERVAL_SECONDS));

    tick_timer_.async_wait(
        boost::asio::bind_executor(
            global_strand_,
            [this](const boost::system::error_code & ec)
            {
                // Stop ticking on server shutdown.
                if (ec == boost::asio::error::operation_aborted)
                {
                    std::string lmsg = "MatchMaker tick aborted, server "
                                       "is likely shutting down.";
                    Console::instance().log(lmsg, LogLevel::WARN);
                    return;
                }
                // Log other errors.
                if (ec)
                {
                    std::string lmsg = "Error in MatchMaker tick timer. "
                                       "Server will be unable to match "
                                       "any new players.";
                    Console::instance().log(lmsg, LogLevel::WARN);
                    return;
                }

                // Call tick_all and restart the timer.
                tick_all();
                start_tick_loop();
            }));
}

void MatchMaker::make_match_on_strand(std::vector<Session::ptr> players,
                                      MatchSettings settings)
{
    asio::post(global_strand_, [this, players, settings]{

        std::string templog = "Creating match with session IDs: "
                              + std::to_string(players[0]->id())
                              + " "
                              + std::to_string(players[1]->id());

        Console::instance().log(templog, LogLevel::INFO);

        std::vector<Session::ptr> live_players;
        live_players.reserve(players.size());

        // Ensure a valid game is about to commence.
        for (uint8_t i = 0; i < players.size(); i++)
        {
            Session::ptr p = players[i];

            // Remove this UUID from the queue
            auto uuid = (p->get_user_data()).user_id;
            uuid_to_gamemode_.erase(uuid);

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
            const UserData & data = players[p_id]->get_user_data();
            player_list.emplace_back
            (
                PlayerInfo
                (
                    p_id,
                    players[p_id]->id(),
                    data.user_id,
                    data.username
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

        std::shared_ptr<MatchInstance> new_inst;

    // Try to create the instance. If this fails, abort, we are likely
    // out of memory and need to stop making matches.
    try
    {
        new_inst = std::make_shared<MatchInstance>(io_cntx_,
                                                   std::move(settings),
                                                   player_list,
                                                   send_callback_,
                                                   game_message_cb);
    }
    catch (...)
    {
        // Notify players that the server had an error creating the match.
        for (uint8_t p_id = 0; p_id < players.size(); p_id++)
        {
            Message notify_cancel;
            notify_cancel.create_serialized(HeaderType::MatchCreationError);
            send_callback_(player_list[p_id].session_id, notify_cancel);
        }

        return;
    }
        // Proceed with match creation if successful.

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
                    user_manager_->notify_match_finished
                                        (
                                            u_id,
                                            static_cast<GameMode>(result.settings.mode)
                                        );
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
    auto user_id = (p->get_user_data()).user_id;
    auto match = uuid_to_match_.find(user_id);

    if (match != uuid_to_match_.end())
    {
        // Convert message to command
        Command cmd_to_send = msg.to_command();

        // Route to found match instance
        match->second->receive_command(user_id, std::move(cmd_to_send));
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

    // Tell the match instance that we are forfeiting if one exists.
    auto inst = uuid_to_match_.find(u_id);

    if (inst == uuid_to_match_.end())
    {
        return;
    }

    inst->second->forfeit(u_id);

    // Remove from the map and notify the user manager.
    uuid_to_match_.erase(u_id);

    // No need to update the mode at this time, a match never got recorded.
    user_manager_->notify_match_finished(u_id, GameMode::NO_MODE);
}
