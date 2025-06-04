#include "database.h"
#include "message.h"
#include <boost/uuid/uuid_io.hpp>

// One connection to the database per thread in the thread pool.
static thread_local std::unique_ptr<pqxx::connection> conn_;

// Utility to turn timepoint into a string for the database.
std::string timepoint_to_string(std::chrono::system_clock::time_point banned_until)
{
    // prepare time point for DB insertion
    auto tt = std::chrono::system_clock::to_time_t(banned_until);
    std::tm tm_utc;

    // We need different time conversion (thread safe)
    // functions for different platforms.
#if defined(_WIN32) || defined(_WIN64)
    gmtime_s(&tm_utc, &tt);
#else
    gmtime_r(&tt, &tm_utc);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%S");

    std::string banned_until_str = oss.str();

    banned_until_str += "Z";

    return banned_until_str;
}

// We expect that a file exists at ~/.pgpass
Database::Database(asio::io_context& io,
                   AuthCallback auth_callback,
                   BanCallback ban_callback)
: strand_(io.get_executor()),
thread_pool_(MAX_CONCURRENT_AUTHS),
auth_callback_(std::move(auth_callback)),
ban_callback_(std::move(ban_callback)),
// Isolate per username work on the same thread pool.
uuid_strands_(thread_pool_.get_executor())
{}

void Database::authenticate(Message msg,
                            std::shared_ptr<Session> session,
                            std::string client_ip)
{
    asio::post(strand_,
               [this,
                m = std::move(msg),
                s = std::move(session),
                ip = std::move(client_ip)]
    {

        // First, turn the message into an authentication attempt
        LoginRequest request = m.to_login_request();

        // If the username has length zero, we should stop now.
        if (request.username.length() == 0)
        {
            // Callback to server with null uuid
            UserData nil_data;
            if (auth_callback_)
            {
                auth_callback_(nil_data, s);
            }
            else
            {
                std::cerr << "Auth callback was not set!\n";
            }

            return;
        }

        // Otherwise we have a valid H(password) and some valid length characters
        // that might be a username. Attempt to authenticate the user.
        //
        // Post the database call to our thread pool.
        do_auth(std::move(request), std::move(s), std::move(ip));

    });
}

void Database::register_account(Message msg,
                                std::shared_ptr<Session> session,
                                std::string client_ip)
{
    asio::post(strand_,
               [this,
                m = std::move(msg),
                s = std::move(session),
                ip = std::move(client_ip)]{

        // First, turn the message into a registration attempt
        //
        // This is exactly the same as login attempt so we simply
        // treat it the same way.
        LoginRequest request = m.to_login_request();

        // If the username has length zero, we should stop now.
        if (request.username.length() == 0)
        {
            // Tell the client that the registration was bad.
            Message bad_reg;
            BadRegNotification r_notif(BadRegNotification::Reason::InvalidUsername);
            bad_reg.create_serialized(r_notif);

            s->deliver(bad_reg);

            return;
        }

        // Otherwise we have a valid registration attempt.
        //
        // Post the database call to our thread pool.
        do_register(std::move(request), std::move(s), std::move(ip));

    });

}

void Database::record_match(MatchResult result)
{
    asio::post(strand_, [this, result = std::move(result)] mutable
    {

        std::string moves_json = glz::write_json(result.move_history).value_or("error");
        std::string settings_json = glz::write_json(result.settings).value_or("error");

        // post database work to the database thread pool.
        do_record(result.user_ids,
                  result.elimination_order,
                  settings_json,
                  moves_json,
                  result.settings.mode);

    });
}

void Database::ban_ip(std::string ip,
            std::chrono::system_clock::time_point banned_until)
{
    do_ban_ip(std::move(ip), banned_until);
}

void Database::unban_ip(std::string ip)
{
    do_unban_ip(std::move(ip));
}

void Database::ban_user(std::string username,
              std::chrono::system_clock::time_point banned_until,
              std::string reason)
{
    do_ban_user(std::move(username),
                std::move(banned_until),
                std::move(reason));
}

void Database::unban_user(std::string username, uint64_t ban_id)
{
    do_unban_user(std::move(username), ban_id);
}

void Database::send_friend_request(boost::uuids::uuid user,
                                   std::string friend_username,
                                   std::shared_ptr<Session> session)
{
    do_send_friend_request(std::move(user),
                           std::move(friend_username),
                           std::move(session));
}

void Database::respond_friend_request(boost::uuids::uuid user,
                                      boost::uuids::uuid sender,
                                      bool decision,
                                      std::shared_ptr<Session> session)
{
    do_respond_friend_request(std::move(user),
                              std::move(sender),
                              decision,
                              std::move(session));
}

void Database::block_user(boost::uuids::uuid blocker,
                          std::string blocked,
                          std::shared_ptr<Session> session)
{
    do_block_user(std::move(blocker),
                  std::move(blocked),
                  std::move(session));
}

void Database::unblock_user(boost::uuids::uuid blocker,
                            boost::uuids::uuid blocked,
                            std::shared_ptr<Session> session)
{
    do_unblock_user(std::move(blocker),
                    std::move(blocked),
                    std::move(session));
}

void Database::remove_friend(boost::uuids::uuid user,
                             boost::uuids::uuid friend_id,
                             std::shared_ptr<Session> session)
{
    do_remove_friend(std::move(user),
                     std::move(friend_id),
                     std::move(session));
}

void Database::fetch_blocks(boost::uuids::uuid user,
                            std::shared_ptr<Session> session)
{
    do_fetch_blocks(std::move(user), std::move(session));
}

void Database::fetch_friends(boost::uuids::uuid user,
                             std::shared_ptr<Session> session)
{
    do_fetch_friends(std::move(user), std::move(session));
}

void Database::fetch_friend_requests(boost::uuids::uuid user,
                                     std::shared_ptr<Session> session)
{
    do_fetch_friend_requests(std::move(user), std::move(session));
}

std::unordered_map<std::string, std::chrono::system_clock::time_point>
Database::load_bans()
{
    std::unordered_map<std::string, std::chrono::system_clock::time_point>
    map;

    std::unique_ptr<pqxx::connection> temp_conn_ = std::make_unique<pqxx::connection>(
            "host=127.0.0.1 "
            "port=5432 "
            "dbname=SilentTanksDB "
            "user=SilentTanksOperator");

    pqxx::work txn{*temp_conn_};
    // Get the time in terms of the unix epoch
    auto res = txn.exec_params(
        "SELECT ip, (extract(epoch FROM banned_until)*1000)::bigint AS banned_ms "
        "FROM BannedIPs");

    // For row in rows essentially.
    for (const auto & row : res)
    {
        std::string ip = row["ip"].c_str();

        auto ms = row["banned_ms"].as<long long>();

        auto timepoint = std::chrono::system_clock::time_point{
                                std::chrono::milliseconds(ms)
                                };
        map.emplace(std::move(ip), timepoint);
    }

    return map;
}

void Database::do_auth(LoginRequest request,
                       std::shared_ptr<Session> session,
                       std::string client_ip)
{
    asio::post(thread_pool_,
               [this,
               req=std::move(request),
               s = std::move(session),
               ip = std::move(client_ip)]() mutable{

        try
        {
            prepares();

            pqxx::work txn{*conn_};
            auto res = txn.exec_prepared("auth", req.username);

            if (!res.empty())
            {
                auto row = res[0];

                pqxx::binarystring hb{row["hash"]};
                pqxx::binarystring sb{row["salt"]};

                std::string hash_bytes = hb.str();
                std::string salt_bytes = sb.str();

                boost::uuids::string_generator gen;
                boost::uuids::uuid user_id = gen(row["user_id"].as<std::string>());

                uuid_strands_.post(user_id,
                    [this,
                     req = std::move(req),
                     user_id,
                     hash_bytes,
                     salt_bytes,
                     s,
                     ip
                     ]() mutable {
// START LAMBDA FOR UUID BASED STRAND
    UserData data;
    try {
            prepares();

            pqxx::work txn{*conn_};
            pqxx::result ban_res = txn.exec_prepared
                                    (
                                        "check_bans",
                                        boost::uuids::to_string(user_id)
                                    );

            // If user is banned, reject and send them a banned message.
            if(!ban_res.empty())
            {
                auto ban_row = ban_res[0];

                // Convert the ban time, same as load_bans
                auto ms = ban_row["banned_ms"].as<long long>();

                auto timepoint = std::chrono::system_clock::time_point
                                    {
                                        std::chrono::milliseconds(ms)
                                    };

                // Convert to message and deliver to the session.
                BanMessage ban_msg;
                ban_msg.time_till_unban = timepoint;
                ban_msg.reason = ban_row["reason"].as<std::string>();

                Message banned;
                banned.create_serialized(ban_msg);
                s->deliver(banned);

                txn.commit();

                if(auth_callback_)
                {
                    auth_callback_(data, s);
                }
                else
                {
                    std::cerr << "Auth callback was not set!\n";
                }

                // Close the client connection, they are banned.
                s->close_session();

                return;
            }

            // If we get here, the user is not currently banned.

            if (hash_bytes.size() != HASH_LENGTH ||
                salt_bytes.size() != SALT_LENGTH)
            {
                std::cerr << "DB call had wrong hashing lengths!\n";
                std::cerr << hash_bytes.size() << " " << salt_bytes.size() << "\n";
                return;
            }

            std::array<uint8_t, HASH_LENGTH> hash;
            std::array<uint8_t, SALT_LENGTH> salt;

            std::copy
            (
                reinterpret_cast<uint8_t *>(hash_bytes.data()),
                reinterpret_cast<uint8_t *>(hash_bytes.data() + hash_bytes.size()),
                hash.begin()
            );

            std::copy
            (
                reinterpret_cast<uint8_t *>(salt_bytes.data()),
                reinterpret_cast<uint8_t *>(salt_bytes.data() + salt_bytes.size()),
                salt.begin()
            );

            // First, take our 32 byte H(password) from the user
            // and compute H(H(password), salt) using libargon2
            std::array<uint8_t, HASH_LENGTH> computed_hash{};
            int result = argon2id_hash_raw(
                ARGON2_TIME,
                ARGON2_MEMORY,
                ARGON2_PARALLEL,
                reinterpret_cast<const uint8_t*>(req.hash.data()),
                req.hash.size(),
                salt.data(),
                salt.size(),
                computed_hash.data(),
                computed_hash.size());

            // If function hashed successfully, compare hashes.
            if (result == ARGON2_OK)
            {
                // If correct, password matched and we can authenticate.
                if (computed_hash == hash)
                {
                    data.user_id = user_id;
                    data.username = req.username;

                    // Update the user's login history
                    txn.exec_prepared("update_user_logins",
                                      ip,
                                      boost::uuids::to_string(user_id));
                }
            }
            // Handle bad function calls.
            else
            {
                std::cerr << "Argon2 hashing failed " << argon2_error_message(result) << "\n";
            }

        txn.commit();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Unexpected exception occured during auth " << e.what() << "\n";
    }

    // Send callback to server once we've done our work.
    if(auth_callback_)
    {
        auth_callback_(data, s);
    }
    else
    {
        std::cerr << "Auth callback was not set!\n";
    }

            });
// END LAMBDA FOR UUID BASED STRAND
            }
            else
            {
                // Empty, failed to find user.
            }

            txn.commit();
            return;
        }

        catch(const pqxx::sql_error& e)
        {
            std::cerr << "Database error: " << e.what() << "\n";
        }
        catch(const std::exception& e)
        {
            std::cerr << "Unexpected exception occured during auth " << e.what() << "\n";
        }
    });
}

// TODO: avoid try, catch for uniqueness violations
void Database::do_register(LoginRequest request,
                           std::shared_ptr<Session> session,
                           std::string client_ip)
{
    asio::post(thread_pool_,
               [this,
               req=std::move(request),
               s = std::move(session),
               ip = std::move(client_ip)]() mutable{

        std::array<uint8_t, SALT_LENGTH> salt;

        // Call libsodium to fill the salt with cryptography ready
        // random bytes.
        randombytes_buf(salt.data(), salt.size());

        // Hash with libargon2
        std::array<uint8_t, HASH_LENGTH> computed_hash{};

        int result = argon2id_hash_raw(
                    ARGON2_TIME,
                    ARGON2_MEMORY,
                    ARGON2_PARALLEL,
                    reinterpret_cast<const uint8_t*>(req.hash.data()),
                    req.hash.size(),
                    salt.data(),
                    salt.size(),
                    computed_hash.data(),
                    computed_hash.size());

        if (result != ARGON2_OK)
        {
            std::cerr << "Argon2id failed: " << argon2_error_message(result) << "\n";

            Message bad_reg;
            BadRegNotification r_notif(BadRegNotification::Reason::ServerError);
            bad_reg.create_serialized(r_notif);

            s->deliver(bad_reg);
            return;
        }

        // Now, try to insert into the database
        try
        {
            prepares();

            pqxx::work txn{*conn_};

            // Create a random UUID for database insert
            boost::uuids::uuid user_id = boost::uuids::random_generator()();

            txn.exec_prepared(
                "reg",
                boost::uuids::to_string(user_id),
                req.username,
                pqxx::binarystring(reinterpret_cast<const char*>(computed_hash.data()), computed_hash.size()),
                pqxx::binarystring(reinterpret_cast<const char*>(salt.data()), salt.size()),
                ip);

            // Create initial elo entries for the user.
            for (uint8_t mode = RANKED_MODES_START;
                 mode < static_cast<uint8_t>(GameMode::NO_MODE);
                 mode++)
            {
                txn.exec_prepared("add_user_elos",
                              boost::uuids::to_string(user_id),
                              static_cast<int16_t>(mode),
                              DEFAULT_ELO);
            }

            txn.commit();

            // Tell client that registration was successful
            Message good_reg;
            good_reg.create_serialized(HeaderType::GoodRegistration);
            s->deliver(good_reg);
            return;
        }

        catch (const pqxx::unique_violation & e)
        {
            std::cerr << "Exception in registration: user not unique" << "\n";

            // Tell client that their username is not unique
            Message bad_reg;
            BadRegNotification r_notif(BadRegNotification::Reason::NotUnique);
            bad_reg.create_serialized(r_notif);

            s->deliver(bad_reg);
        }

        catch (const std::exception & e)
        {
            std::cerr << "Exception in registration: " << e.what() << "\n";

            Message bad_reg;
            BadRegNotification r_notif(BadRegNotification::Reason::ServerError);
            bad_reg.create_serialized(r_notif);

            s->deliver(bad_reg);
        }


    });

}

void Database::do_record(std::vector<boost::uuids::uuid> user_ids,
                   std::vector<uint8_t> elimination_order,
                   std::string settings_json,
                   std::string moves_json,
                   GameMode mode)
{
    asio::post(thread_pool_,
        [this,
        user_ids,
        elimination_order,
        settings_json,
        moves_json,
        mode]() mutable{

        try
        {
            prepares();

            // start transaction
            pqxx::work txn{*conn_};

            // Insert the match
            auto res_m_id = txn.exec_params(
                "INSERT INTO Matches(game_mode, settings, move_history) \
                VALUES ($1, $2::jsonb, $3::jsonb) \
                RETURNING match_id",
                static_cast<short>(mode),
                settings_json,
                moves_json
            );

            long match_id = res_m_id[0][0].as<long>();

            // Now we insert each player for the game into the
            // MatchPlayers table. Loop through the players.
            for (size_t i = 0; i < user_ids.size(); i++)
            {
                txn.exec_params(
                    "INSERT INTO MatchPlayers(match_id, user_id, \
                    player_id, placement) VALUES ($1, $2, $3, $4)",
                    match_id,
                    boost::uuids::to_string(user_ids[i]),
                    static_cast<short>(i),
                    static_cast<short>(elimination_order[i])
                );
            }

            // Next, we decide if we need to change the player
            // elo and if so we fetch the player's current ranks.
            if (static_cast<uint8_t>(mode) >= RANKED_MODES_START)
            {
                // TODO: ranked updates
            }

            // Commit the transaction.
            txn.commit();

        }
        catch (const std::exception & e)
        {
            std::cerr << "standard exception in db match write: " << e.what() << "\n";
        }

    });
}

void Database::do_ban_ip(std::string ip,
               std::chrono::system_clock::time_point banned_until)
{
    asio::post(thread_pool_, [this, ip, banned_until]{
        try
        {
            prepares();

            pqxx::work txn{*conn_};

            std::string banned_until_str = timepoint_to_string(banned_until);

            txn.exec_prepared("ban_ip", ip, banned_until_str);
            txn.commit();
        }
        catch (const std::exception & e)
        {
            std::cerr << "Exception in do_ban_ip: " << e.what() << "\n";
        }

    });
}

void Database::do_unban_ip(std::string ip)
{
    asio::post(thread_pool_, [this, ip]{
        try
        {
            prepares();

            pqxx::work txn{*conn_};

            txn.exec_prepared("unban_ip", ip);
            txn.commit();
        }
        catch (const std::exception & e)
        {
            std::cerr << "Exception in do_unban_ip: " << e.what() << "\n";
        }

    });
}

void Database::do_ban_user(std::string username,
                 std::chrono::system_clock::time_point banned_until,
                 std::string reason)
{
    asio::post(thread_pool_,
        [this,
        username = std::move(username),
        banned_until = std::move(banned_until),
        reason = std::move(reason)]{

        try
        {
            prepares();

            pqxx::work txn{*conn_};

            // Find the uuid for this username

            auto user_res = txn.exec_prepared("find_uuid", username);

            if(!user_res.empty())
            {
                auto user_row = user_res[0];

                // Turn this into a boost UUID.
                std::string uuid_str = user_row["user_id"].as<std::string>();

                boost::uuids::string_generator gen;
                boost::uuids::uuid user_id = gen(uuid_str);

                uuid_strands_.post(user_id,
                    [this,
                     user_id = std::move(user_id),
                     banned_until = std::move(banned_until),
                     reason = std::move(reason)
                     ]() mutable {
// START LAMBDA FOR UUID BASED STRAND
                prepares();

                pqxx::work txn{*conn_};

                // Make the time string for the database
                std::string banned_until_str = timepoint_to_string(banned_until);

                // Now insert the ban based on the UUID.
                txn.exec_prepared("ban_user",
                                  boost::uuids::to_string(user_id),
                                  banned_until_str,
                                  reason);

                txn.commit();

                // Callback with the UUID for the server to remove
                // this user's data from memory.
                ban_callback_(user_id, banned_until, reason);

                });
// END LAMBDA FOR UUID BASED STRAND
            }
            // No user found, exit.
            else
            {
                // TODO: log this to console
                std::cerr << "no user in do ban user\n";
            }
            txn.commit();
        }
        catch (const std::exception & e)
        {
            std::cerr << "Exception in do_ban_user: " << e.what() << "\n";
        }

    });
}

void Database::do_unban_user(std::string username, uint64_t ban_id)
{
    asio::post(thread_pool_,
        [this,
        username = std::move(username),
        ban_id]{

        try
        {
            prepares();

            pqxx::work txn{*conn_};

            // Find the uuid for this username

            auto user_res = txn.exec_prepared("find_uuid", username);

            if(!user_res.empty())
            {
                auto user_row = user_res[0];

                // Turn this into a boost UUID.
                std::string uuid_str = user_row["user_id"].as<std::string>();

                boost::uuids::string_generator gen;
                boost::uuids::uuid user_id = gen(uuid_str);

                uuid_strands_.post(user_id,
                    [this,
                     ban_id]() mutable {
// START LAMBDA FOR UUID BASED STRAND
                prepares();

                pqxx::work txn{*conn_};

                txn.exec_prepared("unban_user", ban_id);

                txn.commit();

                });
// END LAMBDA FOR UUID BASED STRAND
            }
            // No user found, exit.
            else
            {
                // TODO: log to console
                std::cerr << "no user in do unban user\n";
            }
            txn.commit();
        }
        catch (const std::exception & e)
        {
            std::cerr << "Exception in do_unban_user: " << e.what() << "\n";
        }

    });
}

void Database::do_send_friend_request(boost::uuids::uuid user,
                                      std::string friend_username,
                                      std::shared_ptr<Session> session)
{
    asio::post(thread_pool_,
        [this,
        user,
        friend_username = std::move(friend_username),
        s = std::move(session)]{

        try
        {
            prepares();

            pqxx::work txn{*conn_};

            // Find the uuid for this username

            auto user_res = txn.exec_prepared("find_uuid", friend_username);

            if(!user_res.empty())
            {
                auto user_row = user_res[0];

                // Turn this into a boost UUID.
                std::string uuid_str = user_row["user_id"].as<std::string>();

                boost::uuids::string_generator gen;
                boost::uuids::uuid friend_id = gen(uuid_str);

                uuid_strands_.post(friend_id,
                    [this,
                    user = std::move(user),
                    friend_id = std::move(friend_id),
                    s = std::move(s)]() mutable {
// START LAMBDA FOR UUID BASED STRAND
                try
                {
                    prepares();

                    pqxx::work txn{*conn_};

                    // Check if the sender is blocked
                    // by the receiver.
                    auto blocked_res = txn.exec_prepared("check_blocked",
                                      boost::uuids::to_string(user),
                                      boost::uuids::to_string(friend_id));

                    // If blocked, finish transaction and close.
                    if (!blocked_res.empty())
                    {
                        txn.commit();
                        return;
                    }

                    txn.exec_prepared("friend_request",
                                      boost::uuids::to_string(user),
                                      boost::uuids::to_string(friend_id));

                    txn.commit();
                }
                catch (const std::exception & e)
                {
                    std::cerr << "Exception in do_send_friend_request: " << e.what() << "\n";
                }
                });
// END LAMBDA FOR UUID BASED STRAND
            }
            // No user found, exit.
            else
            {
                // TODO: log to console
                std::cerr << "no user in do_send_friend_request\n";
            }
            txn.commit();
        }
        catch (const std::exception & e)
        {
            std::cerr << "Exception in do_send_friend_request: " << e.what() << "\n";
        }

    });
}

void Database::do_respond_friend_request(boost::uuids::uuid user,
                                         boost::uuids::uuid sender,
                                         bool decision,
                                         std::shared_ptr<Session> session)
{

    uuid_strands_.post(user,
        [this,
        user = std::move(user),
        sender = std::move(sender),
        decision,
        s = std::move(session)]() mutable {
// START LAMBDA FOR UUID BASED STRAND
    try
    {
        prepares();

        pqxx::work txn{*conn_};

        if (decision == ACCEPT_REQUEST)
        {
            // Check if this uuid maps to a valid user.
            auto req_res = txn.exec_prepared(
                                    "find_friend_request",
                                    boost::uuids::to_string(user),
                                    boost::uuids::to_string(sender));

            if(!req_res.empty())
            {
                txn.exec_prepared("accept_friend",
                              boost::uuids::to_string(user),
                              boost::uuids::to_string(sender));

                txn.exec_prepared("delete_friend_request",
                              boost::uuids::to_string(sender),
                              boost::uuids::to_string(user));
            }
            else
            {
                // Request never existed!
            }
        }
        else
        {
            txn.exec_prepared("delete_friend_request",
                              boost::uuids::to_string(sender),
                              boost::uuids::to_string(user));
        }

        txn.commit();
    }
    catch (const std::exception & e)
    {
        std::cerr << "Exception in do_respond_friend_request: " << e.what() << "\n";
    }
    });
// END LAMBDA FOR UUID BASED STRAND
}

void Database::do_block_user(boost::uuids::uuid blocker,
                             std::string blocked,
                             std::shared_ptr<Session> session)
{
    asio::post(thread_pool_,
        [this,
        blocker,
        blocked = std::move(blocked),
        s = std::move(session)]{

        try
        {
            prepares();

            pqxx::work txn{*conn_};

            // Find the uuid for this username

            auto user_res = txn.exec_prepared("find_uuid", blocked);

            if(!user_res.empty())
            {
                auto user_row = user_res[0];

                // Turn this into a boost UUID.
                std::string uuid_str = user_row["user_id"].as<std::string>();

                boost::uuids::string_generator gen;
                boost::uuids::uuid blocked_id = gen(uuid_str);

                uuid_strands_.post(blocked_id,
                    [this,
                    blocker = std::move(blocker),
                    blocked_id = std::move(blocked_id),
                    s = std::move(s)]() mutable {
// START LAMBDA FOR UUID BASED STRAND
            try
            {
                prepares();

                pqxx::work txn{*conn_};

                txn.exec_prepared("block_user",
                                  boost::uuids::to_string(blocker),
                                  boost::uuids::to_string(blocked_id));

                // Remove any friend requests that might exist.
                txn.exec_prepared("delete_friend_request",
                              boost::uuids::to_string(blocked_id),
                              boost::uuids::to_string(blocker));

                txn.exec_prepared("delete_friend_request",
                              boost::uuids::to_string(blocker),
                              boost::uuids::to_string(blocked_id));

                // Remove any friendships that exist between
                // the two users.
                txn.exec_prepared("remove_friend",
                                  boost::uuids::to_string(blocker),
                                  boost::uuids::to_string(blocked_id));

                txn.commit();
            }
            catch (const std::exception & e)
            {
                std::cerr << "Exception in do_block_user: " << e.what() << "\n";
            }
                });
// END LAMBDA FOR UUID BASED STRAND
            }
            // No user found, exit.
            else
            {
                // TODO: log to console
                std::cerr << "no user in do_block_user\n";
            }
            txn.commit();
        }
        catch (const std::exception & e)
        {
            std::cerr << "Exception in do_block_user: " << e.what() << "\n";
        }

    });
}

void Database::do_unblock_user(boost::uuids::uuid blocker,
                               boost::uuids::uuid blocked_id,
                               std::shared_ptr<Session> session)
{
    uuid_strands_.post(blocked_id,
        [this,
        blocker = std::move(blocker),
        blocked_id = std::move(blocked_id),
        s = std::move(s)]() mutable {
// START LAMBDA FOR UUID BASED STRAND
    try
    {
        prepares();

        pqxx::work txn{*conn_};

        txn.exec_prepared("unblock_user",
                          boost::uuids::to_string(blocker),
                          boost::uuids::to_string(blocked_id));

        txn.commit();
    }
    catch (const std::exception & e)
    {
        std::cerr << "Exception in do_unblock_user: " << e.what() << "\n";
            }
    });
// END LAMBDA FOR UUID BASED STRAND
}

void Database::do_remove_friend(boost::uuids::uuid user,
                                boost::uuids::uuid friend_id,
                                std::shared_ptr<Session> session)
{
    uuid_strands_.post(friend_id,
        [this,
        user = std::move(user),
        friend_id = std::move(friend_id),
        s = std::move(session)]() mutable {
// START LAMBDA FOR UUID BASED STRAND
    try
    {
        prepares();

        pqxx::work txn{*conn_};

        txn.exec_prepared("remove_friend",
                          boost::uuids::to_string(user),
                          boost::uuids::to_string(friend_id));

        txn.commit();
    }
    catch (const std::exception & e)
    {
        std::cerr << "Exception in do_remove_friend: " << e.what() << "\n";
    }
    });
// END LAMBDA FOR UUID BASED STRAND
}

void Database::do_fetch_blocks(boost::uuids::uuid user,
                               std::shared_ptr<Session> session)
{

}

void Database::do_fetch_friends(boost::uuids::uuid user,
                                std::shared_ptr<Session> session)
{

}

void Database::do_fetch_friend_requests(boost::uuids::uuid user,
                                        std::shared_ptr<Session> session)
{

}

void Database::prepares()
{
    if (!conn_)
    {
        conn_ = std::make_unique<pqxx::connection>(
            "host=127.0.0.1 "
            "port=5432 "
            "dbname=SilentTanksDB "
            "user=SilentTanksOperator");
        conn_->prepare("auth",
            "SELECT user_id, hash, salt "
            "FROM Users "
            "WHERE username = $1");
        conn_->prepare("reg",
            "INSERT INTO Users (user_id, username, hash, salt, last_ip) "
            "VALUES ($1, $2, $3, $4, $5)");
        conn_->prepare("ban_ip",
            "INSERT INTO BannedIPs (ip, banned_until, original_expiration) "
            "VALUES ($1, $2, $2) "
            "ON CONFLICT (ip) DO UPDATE "
            "  SET banned_until = EXCLUDED.banned_until, "
            "  original_expiration = EXCLUDED.original_expiration"
            );
        conn_->prepare("unban_ip",
            "UPDATE BannedIPs "
            "SET banned_until = now() "
            "WHERE ip = $1"
            );
        conn_->prepare("update_user_logins",
            "UPDATE Users "
            "SET last_login = now(), "
            "last_ip = $1 "
            "WHERE user_id = $2");
        conn_->prepare("check_bans",
            "SELECT banned_at, (extract(epoch FROM banned_until)*1000)::bigint AS banned_ms, reason "
            "FROM UserBans "
            "WHERE user_id = $1 "
            " AND banned_until > now() "
            "ORDER BY banned_at DESC "
            "LIMIT 1");
        conn_->prepare("add_user_elos",
            "INSERT INTO UserElos (user_id, game_mode, current_elo) "
            "VALUES ($1, $2, $3)");
        conn_->prepare("find_uuid",
            "SELECT user_id "
            "FROM Users "
            "WHERE username = $1");
        conn_->prepare("ban_user",
            "INSERT INTO UserBans (user_id, banned_until, original_expiration, reason) "
            "VALUES ($1, $2, $2, $3)");
        conn_->prepare("unban_user",
            "UPDATE UserBans "
            "SET banned_until = now() "
            "WHERE ban_id = $1");
        conn_->prepare("friend_request",
            "INSERT INTO FriendRequests (sender, receiver) "
            "VALUES ($1, $2) "
            "ON CONFLICT DO NOTHING");
        conn_->prepare("delete_friend_request",
            "DELETE FROM FriendRequests "
            "WHERE sender = $1 "
            " AND receiver = $2");
        conn_->prepare("accept_friend",
            "INSERT INTO Friends (user_a, user_b) "
            "VALUES (LEAST($1, $2), GREATEST($1, $2)) "
            "ON CONFLICT DO NOTHING");
        conn_->prepare("find_friend_request",
            "SELECT 1 "
            "FROM FriendRequests "
            "WHERE receiver = $1 AND sender = $2 "
            "LIMIT 1");
        conn_->prepare("block_user",
            "INSERT INTO BlockedUsers (blocker, blocked) "
            "VALUES ($1, $2)");
        conn_->prepare("check_blocked",
            "SELECT 1 "
            "FROM BlockedUsers "
            "WHERE blocker = $1 AND blocked = $2 "
            "LIMIT 1");
        conn_->prepare("remove_friend",
            "DELETE FROM Friends "
            "WHERE (user_a = LEAST($1, $2) AND user_b = GREATEST($1, $2))");
        conn_->prepare("unblock_user",
            "DELETE FROM BlockedUsers "
            "WHERE blocker = $1 AND blocked = $2");
        conn_->prepare("fetch_blocks",
            "SELECT blocked "
            "FROM BlockedUsers "
            "WHERE blocker = $1");
        conn_->prepare("fetch_friends",
            "SELECT CASE "
            "WHEN user_a = $1 THEN user_b "
            "ELSE user a "
            "END AS friend_id "
            "FROM Friends "
            "WHERE user_a = $1 OR user_b = $1");
        conn_->prepare("fetch_friend_requests",
            "SELECT sender "
            "FROM FriendRequests "
            "WHERE receiver = $1");
    }
}
