#pragma once

#include <utility>
#include <chrono>

#include "match-maker.h"
#include "user-manager.h"
#include "database.h"

static constexpr int SHUTDOWN_COMPONENTS_COUNT = 3;

// TODO <security>: profile how many sessions can be a reasonable default.
static constexpr int DEFAULT_MAX_SESSIONS = 1600;

class Server
{
    using tcp = asio::ip::tcp;
    using ptr = std::shared_ptr<Session>;

public:
    // construct the server given a context and an endpoint
    Server(asio::io_context & cntx,
           tcp::endpoint endpoint);

    void CONSOLE_ban_user(std::string username,
                          std::chrono::system_clock::time_point banned_until,
                          std::string reason);

    void CONSOLE_ban_ip(std::string username,
                        std::chrono::system_clock::time_point banned_until);

    void shutdown();

    void do_accept();
private:
    void handle_accept(const boost::system::error_code & ec,
                       tcp::socket socket);

    void remove_session(const ptr & session);

    void on_message(const ptr & session, Message msg);

    void on_auth(UserData data,
                 UserManager::UUIDHashSet friends,
                 UserManager::UUIDHashSet blocked_users,
                 std::shared_ptr<Session> session);

    void on_ban_user(boost::uuids::uuid user_id,
                     std::chrono::system_clock::time_point banned_until,
                     std::string reason);

    void send_banned(std::chrono::system_clock::time_point banned_until,
                     tcp::socket socket);

    void send_reject(tcp::socket socket);

    void notify_subsystem_shutdown();

private:
    // Spawning IO context reference.
    asio::io_context & calling_context_;

    // Strand to prevent race conditions on session removal and addition.
    asio::strand<asio::io_context::executor_type> server_strand_;

    // Acceptor bound to an endpoint.
    tcp::acceptor acceptor_;

    // Rare case where we don't want an array style structure.
    //
    // If many sessions are opened, it becomes hard to find sessions and
    // we can experience memory fragmentation if many sessions are opened
    // and closed between server reboots.
    //
    // By using an unordered map we end up preventing this, though, we
    // get worse cache locality of course.
    //
    // This trade off is perfectly reasonable for a server however.
    //
    // We map each session ID to a session with this structure.
    std::unordered_map<uint64_t, ptr> sessions_;

    // We would prefer to hand off login requests to a login manager
    // just like we do with match making requests.
    std::shared_ptr<UserManager> user_manager_;

    // Class to handle matching players who want to play against one another.
    MatchMaker matcher_;

    // Class to post calls to the postgreSQL database.
    Database db_;

    // Hash map of banned IPs and their unban times.
    std::unordered_map<std::string,
                        std::chrono::system_clock::time_point> bans_;

    std::mutex bans_mutex_;

    // Increasing counter for the next session ID.
    uint64_t next_session_id_{1};

    std::atomic<std::size_t> max_sessions_{DEFAULT_MAX_SESSIONS};

    std::atomic<bool> shutting_down_{false};

    std::mutex shutdown_mutex_;
    std::condition_variable shutdown_cv_;
    size_t remaining_shutdown_tasks_ = 0;

};
