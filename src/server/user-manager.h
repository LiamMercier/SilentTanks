#pragma once

#include <boost/uuid/uuid.hpp>
#include <unordered_map>

#include "user.h"

class UserManager : public std::enable_shared_from_this<UserManager>
{
public:
    // TODO: determine if these are necessary
    using LoginCallback = std::function<void>(std::shared_ptr<User>)>;
    using DisconnectHandler = std::function<void()>;

    // Adds a user on login
    //
    // This function is not responsible for handling logic related to user
    // authentication, only adding them to the list of logged in users.
    void on_login(const UserData & data,
                  std::shared_ptr<Session> session);

    // Called from the server after a session disconnects.
    void disconnect(std::shared_ptr<Session> session);

    // TODO: on match end for when a match finishes

private:

    boost::asio::io_context::strand strand_;
    std::unordered_map<boost::uuids::uuid, std::shared_ptr<User>> users_;
    std::unordered_map<uint64_t, boost::uuids::uuid> sid_to_uuid_;
    // LoginCallback
};


