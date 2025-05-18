#pragma once

#include "message.h"
#include "user.h"

#include <array>
#include <pqxx/pqxx>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <argon2.h>

constexpr SALT_LENGTH = 16;

constexpr ARGON2_TIME = 4;
constexpr ARGON2_MEMORY = 65536;
constexpr ARGON2_PARALLEL = 1;

// Limit how much memory we are consuming for logins.
constexpr MAX_CONCURRENT_AUTHS = 3;

namespace asio = boost::asio;

class Database
{
public:
    // TODO: user information instead of just uuid
    using AuthCallback = std::function<void>(UserData data, std::shared_ptr<Session> session)>;

    Database(asio::io_context& io,
             const std::string & conn_str,
             AuthCallback auth_callback);

    void authenticate(Message msg, std::shared_ptr<Session> session);

private:
    void do_auth(LoginRequest request, std::shared_ptr<Session> session);

private:
    // Main strand to serialize requests.
    asio::strand<asio::io_context::executor_type> strand_

    // Small number of threads for this pool to stop our main
    // threads from blocking.
    asio::thread_pool thread_pool_;
    pqxx::connection conn_;
    AuthCallback auth_callback_;

};
