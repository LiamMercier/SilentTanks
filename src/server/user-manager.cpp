#include "user-manager.h"
#include "match-instance.h"

UserManager::UserManager(boost::asio::io_context & cntx)
:strand_(cntx.get_executor())
{
}

void UserManager::on_login(UserData data,
                           std::shared_ptr<Session> session)
{
    boost::asio::post(strand_, [this,
                                user_data = std::move(data),
                                session = session]{

        auto & user = (this->users_)[user_data.user_id];

        // Handle new user.
        if (!user)
        {
            // Construct from the user data.
            user = std::make_shared<User>(user_data);
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

void UserManager::notify_match_start(boost::uuids::uuid user_id, std::shared_ptr<MatchInstance> inst){

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
