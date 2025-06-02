#pragma once

#include "message.h"
#include "user.h"
#include "match-result.h"
#include "keyed-executor.h"

#include <array>
#include <cstdint>
#include <chrono>
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

// Default elo on table insert
constexpr int DEFAULT_ELO = 1000;

namespace asio = boost::asio;

class Database
{
public:
    using AuthCallback = std::function<void(UserData data,
                                            std::shared_ptr<Session> session)>;

    using BanCallback = std::function<void(boost::uuids::uuid,
                                        std::chrono::system_clock::time_point,
                                        std::string)>;

    Database(asio::io_context & io,
             AuthCallback auth_callback,
             BanCallback ban_callback);

    void authenticate(Message msg,
                      std::shared_ptr<Session> session,
                      std::string client_ip);

    void register_account(Message msg,
                          std::shared_ptr<Session> session,
                          std::string client_ip);

    void record_match(MatchResult result);

    void ban_ip(std::string ip,
                std::chrono::system_clock::time_point banned_until);

    void unban_ip(std::string ip);

    void ban_user(std::string username,
                  std::chrono::system_clock::time_point banned_until,
                  std::string reason);

    std::unordered_map<std::string, std::chrono::system_clock::time_point>
    load_bans();

private:
    void do_auth(LoginRequest request,
                 std::shared_ptr<Session> session,
                 std::string client_ip);

    void do_register(LoginRequest request,
                     std::shared_ptr<Session> session,
                     std::string client_ip);

    void do_record(std::vector<boost::uuids::uuid> user_ids,
                   std::vector<uint8_t> elimination_order,
                   std::string settings_json,
                   std::string moves_json,
                   GameMode mode);

    void do_ban_ip(std::string ip,
                std::chrono::system_clock::time_point banned_until);

    void do_unban_ip(std::string ip);

    void do_ban_user(std::string username,
                     std::chrono::system_clock::time_point banned_until,
                     std::string reason);

    void prepares();

private:
    // Main strand to serialize requests.
    asio::strand<asio::io_context::executor_type> strand_;

    // Small number of threads for this pool to stop our main
    // threads from blocking.
    asio::thread_pool thread_pool_;

    // Callbacks
    AuthCallback auth_callback_;
    BanCallback ban_callback_;

    // Thread safe map of mutex's for user keys.
    KeyedExecutor<boost::uuids::uuid,
                  boost::hash<boost::uuids::uuid>>
                  uuid_strands_;

};
