#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/functional/hash.hpp>
#include <unordered_map>

#include "user.h"

namespace asio = boost::asio;

class UserManager : public std::enable_shared_from_this<UserManager>
{
public:
    using UUIDHashSet = std::unordered_set<boost::uuids::uuid,
                                           boost::hash<boost::uuids::uuid>>;

    UserManager(boost::asio::io_context & cntx);

    // Adds a user on login
    //
    // This function is not responsible for handling logic related to user
    // authentication, only adding them to the list of logged in users.
    void on_login(UserData data,
                  UUIDHashSet friends,
                  UUIDHashSet blocked_users,
                  std::shared_ptr<Session> session);

    // Called from the server after a session disconnects.
    void disconnect(std::shared_ptr<Session> session);

    // Notify of forfeit or match end
    void notify_match_finished(boost::uuids::uuid user_id,
                               GameMode mode);

    // Notify match start.
    void notify_match_start(boost::uuids::uuid user_id,
                            std::shared_ptr<MatchInstance> inst);

    // Update the user data and session's user data on elo update.
    void notify_elo_update(boost::uuids::uuid user_id,
                           int new_elo,
                           GameMode mode);

    void on_ban_user(boost::uuids::uuid user_id,
                     std::chrono::system_clock::time_point banned_until,
                     std::string reason);

    // Update the user's block list for message passing.
    void on_block_user(boost::uuids::uuid blocker,
                       boost::uuids::uuid blocked);

    void on_unblock_user(boost::uuids::uuid blocker,
                         boost::uuids::uuid blocked);

    // Update the user's friend list for message passing.
    void on_friend_user(boost::uuids::uuid user_id,
                        std::string user_username,
                        boost::uuids::uuid friend_uuid);

    void on_unfriend_user(boost::uuids::uuid user_id,
                          boost::uuids::uuid friend_uuid);

    void on_friend_request(boost::uuids::uuid sender,
                           boost::uuids::uuid friend_id);

    void direct_message_user(boost::uuids::uuid sender,
                             TextMessage dm);

    void match_message_user(boost::uuids::uuid sender,
                            InternalMatchMessage msg);

private:

    asio::strand<asio::io_context::executor_type> strand_;
    std::unordered_map<boost::uuids::uuid,
                       std::shared_ptr<User>,
                       boost::hash<boost::uuids::uuid>> users_;
    std::unordered_map<uint64_t, boost::uuids::uuid> sid_to_uuid_;
};


