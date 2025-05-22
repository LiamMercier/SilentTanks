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
    // TODO: determine if these are necessary
    using LoginCallback = std::function<void(std::shared_ptr<User>)>;
    using DisconnectHandler = std::function<void()>;

    UserManager(boost::asio::io_context & cntx);

    // Adds a user on login
    //
    // This function is not responsible for handling logic related to user
    // authentication, only adding them to the list of logged in users.
    void on_login(UserData data,
                  std::shared_ptr<Session> session);

    // Called from the server after a session disconnects.
    void disconnect(std::shared_ptr<Session> session);

    // TODO: on match end for when a match finishes

private:

    asio::strand<asio::io_context::executor_type> strand_;
    std::unordered_map<boost::uuids::uuid,
                       std::shared_ptr<User>,
                       boost::hash<boost::uuids::uuid>> users_;
    std::unordered_map<uint64_t, boost::uuids::uuid> sid_to_uuid_;
    // LoginCallback
};


