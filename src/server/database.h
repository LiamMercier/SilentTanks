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
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <glaze/glaze.hpp>
#include <argon2.h>
#include <sodium.h>

class UserManager;

// Limit how much memory we are consuming for logins.
constexpr size_t MAX_CONCURRENT_AUTHS = 3;

// Maximum number of requests to hold at one time, to prevent spam.
constexpr int MAX_FRIEND_REQUESTS = 50;

namespace asio = boost::asio;

// TODO: pipeline everything that can be pipelined.
// This is especially true for the match write function.
class Database
{
public:
    using UUIDHashSet = std::unordered_set<boost::uuids::uuid,
                                           boost::hash<boost::uuids::uuid>>;

    using AuthCallback = std::function<void
                            (UserData data,
                             UUIDHashSet friends,
                             UUIDHashSet blocked_users,
                             std::shared_ptr<Session> session)>;

    using BanCallback = std::function<void(boost::uuids::uuid,
                                        std::chrono::system_clock::time_point,
                                        std::string)>;

    using ShutdownCallback = std::function<void()>;

    Database(asio::io_context & io,
             AuthCallback auth_callback,
             BanCallback ban_callback,
             ShutdownCallback shutdown_callback,
             std::shared_ptr<UserManager> user_manager);

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
                             Message msg,
                             std::shared_ptr<Session> session);

    void respond_friend_request(boost::uuids::uuid user,
                                Message msg,
                                std::shared_ptr<Session> session);

    void block_user(boost::uuids::uuid blocker,
                    Message msg,
                    std::shared_ptr<Session> session);

    void unblock_user(boost::uuids::uuid blocker,
                      Message msg,
                      std::shared_ptr<Session> session);

    void remove_friend(boost::uuids::uuid user,
                       Message msg,
                       std::shared_ptr<Session> session);

    void fetch_blocks(boost::uuids::uuid user,
                      std::shared_ptr<Session> session);

    void fetch_friends(boost::uuids::uuid user,
                       std::shared_ptr<Session> session);

    void fetch_friend_requests(boost::uuids::uuid user,
                               std::shared_ptr<Session> session);

    void fetch_new_matches(boost::uuids::uuid user,
                           GameMode mode,
                           std::shared_ptr<Session> session);

    void fetch_replay(ReplayRequest req,
                      std::shared_ptr<Session> session);

    std::unordered_map<std::string, std::chrono::system_clock::time_point>
    load_bans();

    void async_shutdown();

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

    void do_fetch_new_matches(boost::uuids::uuid user,
                              GameMode mode,
                              std::shared_ptr<Session> session);

    void do_fetch_replay(ReplayRequest req,
                         std::shared_ptr<Session> session);

    void prepares();

    void try_finish_shutdown();

private:
    // Main strand to serialize requests.
    asio::strand<asio::io_context::executor_type> strand_;

    // Small number of threads for this pool to stop our main
    // threads from blocking.
    asio::thread_pool thread_pool_;

    // Callbacks
    AuthCallback auth_callback_;
    BanCallback ban_callback_;
    ShutdownCallback shutdown_callback_;

    std::shared_ptr<UserManager> user_manager_;

    // Thread safe map of mutex's for user keys.
    KeyedExecutor<boost::uuids::uuid,
                  boost::hash<boost::uuids::uuid>>
                  uuid_strands_;

    std::atomic<bool> shutting_down_{false};
    std::atomic<size_t> pending_writes_{0};
};
