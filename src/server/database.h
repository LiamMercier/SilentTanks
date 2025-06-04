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

constexpr bool ACCEPT_REQUEST = true;

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

    void unban_user(std::string username, uint64_t ban_id);

    void send_friend_request(boost::uuids::uuid user,
                             std::string friend_username,
                             std::shared_ptr<Session> session);

    // TODO: get friend requests

    void respond_friend_request(boost::uuids::uuid user,
                                boost::uuids::uuid sender,
                                bool decision,
                                std::shared_ptr<Session> session);

    void block_user(boost::uuids::uuid blocker,
                    std::string blocked,
                    std::shared_ptr<Session> session);

    // TODO: get blocked users

    void unblock_user(boost::uuids::uuid blocker,
                      boost::uuids::uuid blocked_id,
                      std::shared_ptr<Session> session);

    void remove_friend(boost::uuids::uuid user,
                       boost::uuids::uuid friend_id,
                       std::shared_ptr<Session> session);

    void fetch_blocks(boost::uuids::uuid user,
                      std::shared_ptr<Session> session);

    void fetch_friends(boost::uuids::uuid user,
                       std::shared_ptr<Session> session);

    void fetch_friend_requests(boost::uuids::uuid user,
                               std::shared_ptr<Session> session);

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

    void do_unban_user(std::string username, uint64_t ban_id);

    void do_send_friend_request(boost::uuids::uuid user,
                                std::string friend_username,
                                std::shared_ptr<Session> session);

    void do_respond_friend_request(boost::uuids::uuid user,
                                   boost::uuids::uuid sender,
                                   bool decision,
                                   std::shared_ptr<Session> session);

    void do_block_user(boost::uuids::uuid blocker,
                       std::string blocked,
                       std::shared_ptr<Session> session);

    void do_unblock_user(boost::uuids::uuid blocker,
                         boost::uuids::uuid blocked_id,
                         std::shared_ptr<Session> session);

    void do_remove_friend(boost::uuids::uuid user,
                          boost::uuids::uuid friend_id,
                          std::shared_ptr<Session> session);

    void do_fetch_blocks(boost::uuids::uuid user,
                         std::shared_ptr<Session> session);

    void do_fetch_friends(boost::uuids::uuid user,
                          std::shared_ptr<Session> session);

    void do_fetch_friend_requests(boost::uuids::uuid user,
                                  std::shared_ptr<Session> session);

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
