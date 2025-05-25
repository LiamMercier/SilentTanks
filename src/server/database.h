#pragma once

#include "message.h"
#include "user.h"
#include "match-result.h"

#include <array>
#include <cstdint>
#include <pqxx/pqxx>
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <glaze/glaze.hpp>
#include <argon2.h>
#include <sodium.h>

constexpr size_t SALT_LENGTH = 16;

constexpr uint32_t ARGON2_TIME = 4;
constexpr uint32_t ARGON2_MEMORY = 65536;
constexpr uint32_t ARGON2_PARALLEL = 1;

// Limit how much memory we are consuming for logins.
constexpr size_t MAX_CONCURRENT_AUTHS = 3;

namespace asio = boost::asio;

class Database
{
public:
    using AuthCallback = std::function<void(UserData data,
                                            std::shared_ptr<Session> session)>;

    Database(asio::io_context & io,
             AuthCallback auth_callback);

    void authenticate(Message msg, std::shared_ptr<Session> session);

    void register_account(Message msg, std::shared_ptr<Session> session);

    void record_match(MatchResult result);

private:
    void do_auth(LoginRequest request, std::shared_ptr<Session> session);

    void do_register(LoginRequest request, std::shared_ptr<Session> session);

    void do_record(std::vector<boost::uuids::uuid> user_ids,
                   std::vector<uint8_t> elimination_order,
                   std::string settings_json,
                   std::string moves_json,
                   GameMode mode);

private:
    // Main strand to serialize requests.
    asio::strand<asio::io_context::executor_type> strand_;

    // Small number of threads for this pool to stop our main
    // threads from blocking.
    asio::thread_pool thread_pool_;
    AuthCallback auth_callback_;

};
