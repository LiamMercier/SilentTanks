#include "user-manager.h"

UserManager::UserManager(boost::asio::io_context & cntx)
:strand_(cntx.get_executor())
{
}

void UserManager::on_login(UserData data,
                           std::shared_ptr<Session> session)
{
    boost::asio::post(strand_, [self = shared_from_this(),
                                user_data = std::move(data),
                                session = session]{

        auto & user = (self->users_)[user_data.user_id];

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
        (self->sid_to_uuid_)[session->id()] = user_data.user_id;

        // Sync match if we currently are still in one.
        //
        // TODO: make this an async call to the session to get data
        auto inst = user->current_match.lock();
        if (inst)
        {
            // TODO: make this function
            //inst->request_view(session.id());
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
    [self = shared_from_this(), sid = session->id()]{
        auto s_itr = (self->sid_to_uuid_).find(sid);
        if (s_itr != (self->sid_to_uuid_).end())
        {
            auto uuid = s_itr->second;

            (self->sid_to_uuid_).erase(s_itr);

            auto u_itr = (self->users_).find(uuid);

            if (u_itr != (self->users_).end())
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
                    (self->users_).erase(u_itr);
                }
            }

        }

        // callback? might be necessary, need to figure this out
        // on_complete();

    });
}
