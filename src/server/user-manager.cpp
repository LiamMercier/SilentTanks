#include "user-manager.h"
#include "match-instance.h"

UserManager::UserManager(boost::asio::io_context & cntx)
:strand_(cntx.get_executor())
{
}

void UserManager::on_login(UserData data,
                           UUIDHashSet friends,
                           UUIDHashSet blocked_users,
                           std::shared_ptr<Session> session)
{
    boost::asio::post(strand_, [this,
                                user_data = std::move(data),
                                friends = std::move(friends),
                                blocked = std::move(blocked_users),
                                session = session]{

        auto & user = (this->users_)[user_data.user_id];

        // Handle new user.
        if (!user)
        {
            // Construct from the user data.
            user = std::make_shared<User>(std::move(user_data),
                                          std::move(friends),
                                          std::move(blocked));
        }
        else
        {
            // Put the most current blocks/friends into the user data.
            user->friends = std::move(friends);
            user->blocked_users = std::move(blocked);
        }

        // Handle double sessions by closing the oldest
        // one and setting the new one as the current session.
        if (user->current_session)
        {
            (user->current_session)->close_session();
        }

        // Set the user session and set login to true.
        user->current_session = session;
        session->set_session_data(user_data);

        // Map the session ID to this uuid.
        (this->sid_to_uuid_)[session->id()] = user_data.user_id;

        // Send the user the current match state if they are
        // still inside of a match.
        auto inst = user->current_match.lock();
        if (inst)
        {
            // Notify the client that a game exists.
            Message match_found;
            match_found.create_serialized(HeaderType::MatchInProgress);
            session->deliver(match_found);

            // Sync the player, send them their current view.
            inst->sync_player(session->id(), user_data.user_id);
        }

        });
}

// Called from the server after a session disconnects.
//
// If the user is still in a game, do not remove them from
// the user list. Otherwise, drop this data.
void UserManager::disconnect(std::shared_ptr<Session> session)
{
    boost::asio::post(strand_,
    [this, sid = session->id()]{
        auto s_itr = (this->sid_to_uuid_).find(sid);
        if (s_itr != (this->sid_to_uuid_).end())
        {
            auto uuid = s_itr->second;

            (this->sid_to_uuid_).erase(s_itr);

            auto u_itr = (this->users_).find(uuid);

            if (u_itr != (this->users_).end())
            {
                auto & user = u_itr->second;

                // If we are in this function because of a new
                // session replacing an old one, stop now.
                if (!user->current_session || (user->current_session)->id() != sid)
                {
                    return;
                }

                // Drop the shared session pointer
                user->current_session.reset();

                // If the match instance is still valid, don't drop the user.
                if (!user->current_match.expired())
                {

                }
                // Otherwise we need to drop this user's data.
                else
                {
                    (this->users_).erase(u_itr);
                }
            }

        }

    });
}

void UserManager::notify_match_finished(boost::uuids::uuid user_id)
{
    boost::asio::post(strand_,
    [this, user_id]{

        // Find the User and void its weak pointer.
        auto user_itr = users_.find(user_id);
        if (user_itr == users_.end())
        {
            return;
        }

        auto & user = user_itr->second;

        // Reset the weak pointer, the game is already over.
        user->current_match.reset();

        // If user is not connected, remove their data.
        //
        // This might happen if the user logs out before the
        // match ends and thus disconnect does not clean up the user.
        if (!user->current_session || !(user->current_session->is_live()))
        {
            users_.erase(user_itr);
        }

    });

}

void UserManager::notify_match_start(boost::uuids::uuid user_id,
                                     std::shared_ptr<MatchInstance> inst){

    boost::asio::post(strand_,
    [this, user_id, inst]{

        // Find the User for this uuid.
        auto user_itr = users_.find(user_id);
        if (user_itr == users_.end())
        {
            return;
        }

        auto & user = user_itr->second;

        // Update the weak pointer to point at this new
        // match instance.
        //
        // We can do this assuming we only change users
        // inside of the user strand, which is true.
        user->current_match = inst;

    });

}

void UserManager::notify_elo_update(boost::uuids::uuid user_id,
                                    int new_elo,
                                    GameMode mode)
{
    boost::asio::post(strand_,
        [this,
         uuid = std::move(user_id),
         new_elo,
         mode]{

        auto & user = (this->users_)[uuid];

        uint8_t mode_idx = elo_ranked_index(mode);

        user->user_data.matching_elos[mode_idx] = new_elo;

        // Now, update the session's data.
        (user->current_session)->update_elo(new_elo, mode_idx);

    });
}

void UserManager::on_ban_user(boost::uuids::uuid user_id,
                        std::chrono::system_clock::time_point banned_until,
                        std::string reason)
{
    boost::asio::post(strand_,
        [this,
         uuid = std::move(user_id),
         banned_until = std::move(banned_until),
         reason = std::move(reason)]{

            // Find the user data.
            auto user_itr = users_.find(uuid);
            if (user_itr == users_.end())
            {
                // If there is no data, the user is not currently
                // logged in. We can simply stop here.
                return;
            }

            // Otherwise, grab the user.
            auto user = user_itr->second;

            // Delete the UUID to user mapping.
            users_.erase(user_itr);

            // Now call the session to close.
            if ((user->current_session))
            {
                // Send a banned message.
                BanMessage ban_msg;
                ban_msg.time_till_unban = banned_until;
                ban_msg.reason = reason;

                Message banned;
                banned.create_serialized(ban_msg);
                (user->current_session)->deliver(banned);

                // The server will get a callback from the
                // session after it closes the socket, which
                // will call the user manager to disconnect
                // the mapping from sesssion id -> uuid for us.
                (user->current_session)->close_session();
            }

        });
}

void UserManager::on_block_user(boost::uuids::uuid blocker,
                                boost::uuids::uuid blocked)
{
    boost::asio::post(strand_,
        [this,
        blocker = std::move(blocker),
        blocked = std::move(blocked)]{

        auto user_it = this->users_.find(blocker);

        // Do work if the user exists.
        if (user_it != this->users_.end() && user_it->second)
        {
            // If friends, remove friendship.
            user_it->second->friends.erase(blocked);

            // Add the blocked user to the list.
            user_it->second->blocked_users.emplace(std::move(blocked));

        }

        // Do the update for the other user as well.
        auto blocked_it = this->users_.find(blocked);

        if (blocked_it != this->users_.end() && blocked_it->second)
        {
            // If friends, remove friendship.
            blocked_it->second->friends.erase(blocker);

            // Notify this session if they are online.
            if (blocked_it->second->current_session)
            {
                NotifyRelationUpdate notification;
                notification.user.user_id = std::move(blocker);
                // Not needed.
                notification.user.username.clear();

                Message notify_unfriend;
                notify_unfriend.header.type_ = HeaderType::NotifyFriendRemoved;
                notify_unfriend.create_serialized(notification);

                blocked_it->second->current_session->deliver(notify_unfriend);
            }
        }

    });
}

void UserManager::on_unblock_user(boost::uuids::uuid blocker,
                                  boost::uuids::uuid blocked)
{
    boost::asio::post(strand_,
        [this,
         blocker = std::move(blocker),
         blocked = std::move(blocked)]{

        auto user_it = this->users_.find(blocker);

        // Do work if the user exists.
        if (user_it != this->users_.end() && user_it->second)
        {
            // Remove the blocked user from the list.
            user_it->second->blocked_users.erase(std::move(blocked));
        }

    });
}

void UserManager::on_friend_user(boost::uuids::uuid user_id,
                                 std::string user_username,
                                 boost::uuids::uuid friend_uuid)
{
    boost::asio::post(strand_,
        [this,
        user_id = std::move(user_id),
        user_username = std::move(user_username),
        friend_uuid = std::move(friend_uuid)]{

        auto user_it = this->users_.find(user_id);

        // Do work if the user exists.
        if (user_it != this->users_.end() && user_it->second)
        {
            // Add the friend to user's friends list.
            user_it->second->friends.emplace(friend_uuid);
        }

        // Notify other user of friend added and update the list.
        auto friend_it = this->users_.find(friend_uuid);

        if (friend_it != this->users_.end() &&
            friend_it->second &&
            friend_it->second->current_session)
        {
            friend_it->second->friends.emplace(user_id);

            NotifyRelationUpdate notification;
            notification.user.user_id = std::move(user_id);
            notification.user.username = std::move(user_username);

            Message notify_friend;
            notify_friend.header.type_ = HeaderType::NotifyFriendAdded;
            notify_friend.create_serialized(notification);

            friend_it->second->current_session->deliver(notify_friend);

        }

    });
}

void UserManager::on_unfriend_user(boost::uuids::uuid user_id,
                                   boost::uuids::uuid friend_uuid)
{
    boost::asio::post(strand_,
        [this,
         user_id = std::move(user_id),
         friend_uuid = std::move(friend_uuid)]{

        auto user_it = this->users_.find(user_id);

        // Do work if the user exists.
        if (user_it != this->users_.end() && user_it->second)
        {
            // Remove the friend from user's friends list.
            user_it->second->friends.erase(friend_uuid);
        }

        // Notify other user of friend removed.
        auto friend_it = this->users_.find(friend_uuid);

        if (friend_it != this->users_.end() &&
            friend_it->second &&
            friend_it->second->current_session)
        {
            friend_it->second->friends.erase(user_id);

            NotifyRelationUpdate notification;
            notification.user.user_id = std::move(user_id);

            // Username is not required.
            notification.user.username.clear();

            Message notify_friend;
            notify_friend.header.type_ = HeaderType::NotifyFriendRemoved;
            notify_friend.create_serialized(notification);

            friend_it->second->current_session->deliver(notify_friend);

        }

    });
}

void UserManager::on_friend_request(boost::uuids::uuid sender,
                                    boost::uuids::uuid friend_id)
{
    boost::asio::post(strand_,
        [this,
         sender = std::move(sender),
         friend_id = std::move(friend_id)]{

        // Find the person who is getting the friend request.
        auto user_it = this->users_.find(friend_id);

        // Do work if the user exists.
        if (user_it != this->users_.end() && user_it->second)
        {
            NotifyRelationUpdate notification;

            // Find the sender's username.
            auto sender_it = this->users_.find(sender);

            if (sender_it != this->users_.end() && sender_it->second)
            {
                notification.user.username = sender_it
                                             ->second
                                             ->user_data.username;
            }

            notification.user.user_id = std::move(sender);

            Message request_notification;
            request_notification.create_serialized(notification);
            request_notification.header.type_ = HeaderType::NotifyFriendRequest;

            // Send them the other person's info if possible.
            if ((user_it->second)->current_session)
            {
                 (user_it->second)
                 ->current_session
                 ->deliver(request_notification);
            }

        }

    });
}

// TODO: notify user not online.
void UserManager::direct_message_user(boost::uuids::uuid sender,
                                      TextMessage dm)
{
    boost::asio::post(strand_,
        [this,
         sender = std::move(sender),
         dm = std::move(dm)]{

        // If the user isn't online, drop the message.
        auto user_it = this->users_.find(dm.user_id);
        if (user_it == this->users_.end() || !user_it->second)
        {
            return;
        }

        // Check if the receiver is friends with the sender.
        // If not, drop the message.
        auto user = (user_it->second);

        if (user->friends.find(sender) == user->friends.end())
        {
            return;
        }

        // Otherwise, relay the message.
        if (user->current_session)
        {
            TextMessage client_dm;
            client_dm.user_id = sender;
            client_dm.text = std::move(dm.text);

            Message dm_msg;
            dm_msg.header.type_ = HeaderType::DirectTextMessage;
            dm_msg.create_serialized(client_dm);
            (user->current_session)->deliver(dm_msg);
        }

    });
}

void UserManager::match_message_user(boost::uuids::uuid sender,
                                     InternalMatchMessage msg)
{
    boost::asio::post(strand_,
        [this,
         sender = std::move(sender),
         msg = std::move(msg)]{

        // If the user isn't online, drop the message.
        auto user_it = this->users_.find(msg.user_id);
        if (user_it == this->users_.end() || !user_it->second)
        {
            return;
        }

        // Check if the receiver has blocked the sender.
        // If so, drop the message.
        auto user = (user_it->second);

        if (user->blocked_users.find(sender) != user->blocked_users.end())
        {
            return;
        }

        // Otherwise, relay the message.
        if (user->current_session)
        {
            ExternalMatchMessage client_text;
            // Tell the client who is sending the message.
            client_text.user_id = sender;
            client_text.username_length = static_cast<uint8_t>
                                        (
                                            msg.sender_username.length()
                                        );
            client_text.sender_username = std::move(msg.sender_username);
            client_text.text = std::move(msg.text);

            Message match_msg;
            match_msg.header.type_ = HeaderType::MatchTextMessage;
            match_msg.create_serialized(client_text);
            (user->current_session)->deliver(match_msg);
        }

    });
}

