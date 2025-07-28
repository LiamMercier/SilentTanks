#include "database.h"
#include "message.h"
#include "user-manager.h"
#include "elo-updates.h"
#include "console.h"

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

// Manually convert a vector of UUID strings to data for postgre to use.
//
// This could be replaced if using a new version of libpqxx that supports
// passing in std::vector<std::string> but otherwise this is necessary.
//
// This function should not be called for any arbitrary data, only UUIDs.
std::string uuid_to_pq_array(const std::vector<std::string> & user_id_strs)
{
    std::string result = "{";
    for (size_t i = 0; i < user_id_strs.size(); i++)
    {
        result += "\"" + user_id_strs[i] + "\"";
        if (i + 1 < user_id_strs.size())
        {
            result += ",";
        }
    }

    result += "}";
    return result;
}

// We expect that a file exists at ~/.pgpass
Database::Database(asio::io_context & io,
                   AuthCallback auth_callback,
                   BanCallback ban_callback,
                   std::shared_ptr<UserManager> user_manager)
: strand_(io.get_executor()),
thread_pool_(MAX_CONCURRENT_AUTHS),
auth_callback_(std::move(auth_callback)),
ban_callback_(std::move(ban_callback)),
user_manager_(user_manager),
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

        // First, turn the message into an authentication attempt.
        LoginRequest request = m.to_login_request();

        // If the username has length zero, we should stop now.
        if (request.username.length() == 0)
        {
            // Callback to server with null uuid.
            UserData nil_data;
            UUIDHashSet friends;
            UUIDHashSet blocked_users;

            if (auth_callback_)
            {
                auth_callback_(nil_data,
                               friends,
                               blocked_users,
                               s);
            }
            else
            {
                Console::instance().log("Auth callback was not set!",
                                        LogLevel::ERROR);
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

        std::string moves_json;
        auto ec = glz::write_json(result.move_history, moves_json);

        // Handle bad write.
        if (ec)
        {
            moves_json = "Error parsing moves.";
        }

        std::string settings_json;
        ec = glz::write_json(result, settings_json);

        // Handle bad write.
        if (ec)
        {
            moves_json = "Error parsing settings.";
        }

        // post database work to the database thread pool.
        do_record(result.user_ids,
                  result.elimination_order,
                  settings_json,
                  moves_json,
                  static_cast<GameMode>(result.settings.mode));

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
                                   Message msg,
                                   std::shared_ptr<Session> session)
{

    std::string friend_username = msg.to_username();

    if (friend_username.size() < 1)
    {
        return;
    }

    do_send_friend_request(std::move(user),
                           std::move(friend_username),
                           std::move(session));
}

void Database::respond_friend_request(boost::uuids::uuid user,
                                      Message msg,
                                      std::shared_ptr<Session> session)
{
    FriendDecision friend_decision = msg.to_friend_decision();

    if (friend_decision.user_id == boost::uuids::nil_uuid())
    {
        return;
    }

    do_respond_friend_request(std::move(user),
                              std::move(friend_decision.user_id),
                              std::move(friend_decision.decision),
                              std::move(session));
}

void Database::block_user(boost::uuids::uuid blocker,
                          Message msg,
                          std::shared_ptr<Session> session)
{
    std::string blocked = msg.to_username();

    if (blocked.size() < 1)
    {
        return;
    }

    do_block_user(std::move(blocker),
                  std::move(blocked),
                  std::move(session));
}

void Database::unblock_user(boost::uuids::uuid blocker,
                            Message msg,
                            std::shared_ptr<Session> session)
{
    boost::uuids::uuid unblocked_uuid = msg.to_uuid();

    if (unblocked_uuid == boost::uuids::nil_uuid())
    {
        return;
    }

    do_unblock_user(std::move(blocker),
                    std::move(unblocked_uuid),
                    std::move(session));
}

void Database::remove_friend(boost::uuids::uuid user,
                             Message msg,
                             std::shared_ptr<Session> session)
{
    boost::uuids::uuid friend_uuid = msg.to_uuid();

    if (friend_uuid == boost::uuids::nil_uuid())
    {
        return;
    }

    do_remove_friend(std::move(user),
                     std::move(friend_uuid),
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
    UUIDHashSet friends;
    UUIDHashSet blocks;
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
                    auth_callback_(data, friends, blocks, s);
                }
                else
                {
                    Console::instance().log("Auth callback was not set!",
                                            LogLevel::ERROR);
                }

                // Close the client connection, they are banned.
                s->close_session();

                return;
            }

            // If we get here, the user is not currently banned.

            if (hash_bytes.size() != HASH_LENGTH ||
                salt_bytes.size() != SALT_LENGTH)
            {
                std::string lmsg = "DB call had wrong hashing lengths! "
                                    + std::to_string(hash_bytes.size())
                                    + " "
                                    + std::to_string(salt_bytes.size());
                Console::instance().log(std::move(lmsg),
                                        LogLevel::ERROR);
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

                    // Update the user's login history.
                    txn.exec_prepared("update_user_logins",
                                      ip,
                                      boost::uuids::to_string(user_id));

                    // Grab user relations.
                    boost::uuids::string_generator gen;

                    auto f_res = txn.exec_prepared("INTERNAL_fetch_friends",
                                      boost::uuids::to_string(user_id));

                    friends.clear();
                    friends.reserve(f_res.size());

                    for (const auto & row : f_res)
                    {
                        friends.insert(
                            gen(row["friend_id"].as<std::string>()));
                    }

                    auto b_res = txn.exec_prepared("INTERNAL_fetch_blocks",
                                      boost::uuids::to_string(user_id));

                    blocks.clear();
                    blocks.reserve(b_res.size());

                    for (const auto & row : b_res)
                    {
                        blocks.insert(
                            gen(row["blocked"].as<std::string>()));
                    }

                    // Fetch elo's for this user.
                    auto mode_elos = txn.exec_prepared(
                                            "get_mode_elos",
                                            boost::uuids::to_string(user_id));

                    for (const auto & row : mode_elos)
                    {
                        uint8_t mode = static_cast<uint8_t>
                                        (
                                            row["game_mode"].as<int>()
                                        );

                        if (mode < RANKED_MODES_START)
                        {
                            continue;
                        }

                        uint8_t idx = elo_ranked_index(mode);

                        data.matching_elos[idx] =  row["current_elo"]
                                                    .as<int>();

                        // TODO: remove
                        std::string lmsg = "Loaded elo "
                                            + std::to_string(
                                                data.matching_elos[idx])
                                            + " for game mode "
                                            + std::to_string(mode);

                        Console::instance().log(lmsg, LogLevel::INFO);
                    }

                }
                // Inform bad login.
                else
                {
                    // Tell client that their password is bad.
                    Message bad_auth;
                    BadAuthNotification a_notif(BadAuthNotification::Reason::BadHash);
                    bad_auth.create_serialized(a_notif);

                    s->deliver(bad_auth);
                }
            }
            // Handle bad function calls.
            else
            {
                std::string lmsg = "Argon2 hashing failed "
                                    + std::string(argon2_error_message(result));
                Console::instance().log(std::move(lmsg),
                                        LogLevel::ERROR);
            }

        txn.commit();
    }
    catch(const std::exception & e)
    {
        std::string lmsg = "Unexpected exception occured during auth "
                            + std::string(e.what());
        Console::instance().log(std::move(lmsg),
                                LogLevel::ERROR);
    }
    // Send callback to server once we've done our work.
    if(auth_callback_)
    {
        auth_callback_(data, friends, blocks, s);
    }
    else
    {
        Console::instance().log("Auth callback was not set!",
                                LogLevel::ERROR);
    }

            });
// END LAMBDA FOR UUID BASED STRAND
            }
            else
            {
                // Empty, failed to find user.
                Message bad_auth;
                BadAuthNotification a_notif(BadAuthNotification::Reason::InvalidUsername);
                bad_auth.create_serialized(a_notif);

                s->deliver(bad_auth);
            }

            txn.commit();
            return;
        }
        catch(const pqxx::sql_error& e)
        {
            std::string lmsg = "Database error: "
                                + std::string(e.what());
            Console::instance().log(std::move(lmsg),
                                    LogLevel::WARN);

            Message bad_auth;
            BadAuthNotification a_notif(BadAuthNotification::Reason::ServerError);
            bad_auth.create_serialized(a_notif);

            s->deliver(bad_auth);
        }
        catch(const std::exception& e)
        {
            std::string lmsg = "Unexpected exception occured during auth "
                                + std::string(e.what());
            Console::instance().log(std::move(lmsg),
                                    LogLevel::ERROR);

            Message bad_auth;
            BadAuthNotification a_notif(BadAuthNotification::Reason::ServerError);
            bad_auth.create_serialized(a_notif);

            s->deliver(bad_auth);
        }
    });
}

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
            std::string lmsg = "Argon2id failed: "
                                + std::string(argon2_error_message(result));
            Console::instance().log(std::move(lmsg),
                                    LogLevel::ERROR);

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
            std::string lmsg = "Exception in registration: user not unique";
            Console::instance().log(std::move(lmsg),
                                    LogLevel::INFO);


            // Tell client that their username is not unique
            Message bad_reg;
            BadRegNotification r_notif(BadRegNotification::Reason::NotUnique);
            bad_reg.create_serialized(r_notif);

            s->deliver(bad_reg);
        }

        catch (const std::exception & e)
        {
            std::string lmsg = "Exception in registration: "
                                + std::string(e.what());
            Console::instance().log(std::move(lmsg),
                                    LogLevel::ERROR);

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
        user_ids = std::move(user_ids),
        elimination_order = std::move(elimination_order),
        settings_json = std::move(settings_json),
        moves_json = std::move(moves_json),
        mode]() mutable{

        try
        {
            prepares();

            size_t N_players = user_ids.size();

            if (N_players <= 1)
            {
                Console::instance().log("Too few players in match log!",
                                        LogLevel::ERROR);
                return;
            }

            if (elimination_order.size() != N_players)
            {
                Console::instance().log(
                    "Size mismatch between elimination "
                    "order and number of players.",
                    LogLevel::ERROR);

                return;
            }

            // start main transaction
            pqxx::work match_txn{*conn_};

            // Insert the match
            auto res_m_id = match_txn.exec_prepared(
                "insert_matches",
                static_cast<int16_t>(mode),
                settings_json,
                moves_json
            );

            long match_id = res_m_id[0][0].as<long>();

            // Now we insert each player for the game into the
            // MatchPlayers table. Loop through the players.
            //
            // This is a candidate for pipelining.
            for (size_t i = 0; i < N_players; i++)
            {
                match_txn.exec_prepared("insert_player",
                                  match_id,
                                  boost::uuids::to_string(user_ids[i]),
                                  static_cast<int16_t>(i),
                                  static_cast<int16_t>(elimination_order[i]));
            }

            match_txn.commit();

            // Next, we decide if we need to change the player
            // elos and if so we fetch the player's current ratings.
            if (static_cast<uint8_t>(mode) >= RANKED_MODES_START)
            {
                std::vector<std::string> user_id_strs;
                user_id_strs.reserve(N_players);

                // Go through the array (type is boost uuid) and
                // create a string from the user IDs.
                for (const auto & uuid : user_ids)
                {
                    user_id_strs.push_back(boost::uuids::to_string(uuid));
                }

                // Create a transaction to read the user elos.
                pqxx::work elo_txn{*conn_};

                // Manually turn this into an array of values.
                std::string user_id_array = uuid_to_pq_array(user_id_strs);

                auto read_res = elo_txn.exec_prepared(
                            "get_elos",
                            user_id_array,
                            static_cast<int16_t>(mode));

                // Get elo values out of our result.
                //
                // We map UUID to index and then make the elo vector.
                std::unordered_map<boost::uuids::uuid, size_t> player_idx;
                player_idx.reserve(N_players);

                for (size_t i = 0; i < N_players; i++)
                {
                    player_idx.emplace(user_ids[i], i);
                }

                boost::uuids::string_generator gen;
                int elo_sum = 0;

                std::vector<int> elos(N_players, 0);

                for (const auto & row : read_res)
                {
                    boost::uuids::uuid id = gen
                                    (
                                        row["user_id"].as<std::string>()
                                    );

                    int elo = row["current_elo"].as<int>();
                    auto idx_it = player_idx.find(id);

                    // If we find the user ID then place in correct index.
                    //
                    // This should never fail, because we only found
                    // rows based on our input UUIDs and so there
                    // should be no way for an external UUID to get in.
                    if (idx_it != player_idx.end())
                    {
                        elos[idx_it->second] = elo;
                        elo_sum += elo;
                    }
                }

                // If a player was missing (not found in table) then
                // set their elo to the average of the lobby for fairness.
                //
                // We do not expect this to occur under normal operation, but
                // it is good to guard against bad elo updates.
                if (read_res.size() < N_players)
                {
                    std::string lmsg = "Found "
                                        + std::to_string(read_res.size())
                                        + " of "
                                        + std::to_string(N_players)
                                        + " players in recording for match "
                                        + std::to_string(match_id)
                                        + ".";
                    Console::instance().log(std::move(lmsg),
                                            LogLevel::WARN);

                    int average_elo = std::round(
                                            static_cast<double>(elo_sum)
                                            /
                                            static_cast<double>(N_players));

                    for (size_t i = 0; i < N_players; i++)
                    {
                        if (elos[i] == 0)
                        {
                            elos[i] = average_elo;
                        }
                    }
                }

                // Compute the output elo's for database updates.
                std::vector<int> new_elos = elo_updates(elos, elimination_order);

                // Update user elo's.
                //
                // This is a candidate for pipelining.
                for (size_t i = 0; i < N_players; i++)
                {
                    elo_txn.exec_prepared("record_elo_history",
                                          boost::uuids::to_string(user_ids[i]),
                                          match_id,
                                          static_cast<int16_t>(mode),
                                          elos[i],
                                          new_elos[i]);

                    elo_txn.exec_prepared("update_elo",
                                          boost::uuids::to_string(user_ids[i]),
                                          static_cast<int16_t>(mode),
                                          new_elos[i]);

                    // Inform the user manager of elo updates.
                    user_manager_->notify_elo_update(user_ids[i],
                                                     new_elos[i],
                                                     static_cast<GameMode>(static_cast<uint8_t>(mode)));
                }

                elo_txn.commit();

            }

        }
        catch (const std::exception & e)
        {
            std::string lmsg = "Standard exception in db match write: "
                                + std::string(e.what());
            Console::instance().log(std::move(lmsg),
                                    LogLevel::ERROR);
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
            std::string lmsg = "Exception in do_ban_ip: "
                                + std::string(e.what());
            Console::instance().log(std::move(lmsg),
                                    LogLevel::CONSOLE);
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
            std::string lmsg = "Exception in do_unban_ip: "
                                + std::string(e.what());
            Console::instance().log(std::move(lmsg),
                                    LogLevel::CONSOLE);
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
                Console::instance().log("No user in do_ban_user.",
                                        LogLevel::CONSOLE);
            }
            txn.commit();
        }
        catch (const std::exception & e)
        {
            std::string lmsg = "Exception in do_ban_user: "
                                + std::string(e.what());
            Console::instance().log(std::move(lmsg),
                                    LogLevel::ERROR);
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
                Console::instance().log("No user in do_unban_user",
                                        LogLevel::CONSOLE);
            }
            txn.commit();
        }
        catch (const std::exception & e)
        {
            std::string lmsg = "Exception in do_unban_user: "
                                + std::string(e.what());
            Console::instance().log(std::move(lmsg),
                                    LogLevel::ERROR);
        }

    });
}

void Database::do_send_friend_request(boost::uuids::uuid user,
                                      std::string friend_username,
                                      std::shared_ptr<Session> session)
{
    asio::post(thread_pool_,
        [this,
        user = std::move(user),
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

                // Drop this work if user is trying to friend themselves.
                if (user == friend_id)
                {
                    txn.commit();
                    return;
                }

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

                    // Check if they are already friends.
                    auto friends_res = txn.exec_prepared("check_friends",
                                        boost::uuids::to_string(user),
                                        boost::uuids::to_string(friend_id));

                    // If already friends, stop now.
                    if (!friends_res.empty())
                    {
                        txn.commit();
                        return;
                    }

                    // If a friend request already exists, this will
                    // simply do nothing.
                    txn.exec_prepared("friend_request",
                                      boost::uuids::to_string(user),
                                      boost::uuids::to_string(friend_id),
                                      MAX_FRIEND_REQUESTS);

                    txn.commit();
                }
                catch (const std::exception & e)
                {
                    std::string lmsg = "Exception in do_send_friend_request: "
                                        + std::string(e.what());
                    Console::instance().log(std::move(lmsg),
                                            LogLevel::ERROR);
                }
                });
// END LAMBDA FOR UUID BASED STRAND
            }
            // No user found, exit.
            else
            {
                Console::instance().log("No user in do_send_friend_request",
                                        LogLevel::INFO);
            }
            txn.commit();
        }
        catch (const std::exception & e)
        {
            std::string lmsg = "Exception in do_send_friend_request: "
                                + std::string(e.what());
            Console::instance().log(std::move(lmsg),
                                    LogLevel::ERROR);
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
            // Check if this friend request exists.
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

                auto f_res = txn.exec_prepared("find_username",
                                            boost::uuids::to_string(sender));

                // Send notification to caller on friend accepted.
                NotifyRelationUpdate notification;
                notification.user.user_id = sender;

                if (!f_res.empty())
                {
                    notification.user.username = f_res[0]["username"].
                                                    as<std::string>();
                }
                else
                {
                    notification.user.username.clear();
                }

                Message notify_friend;
                notify_friend.header.type_ = HeaderType::NotifyFriendAdded;
                notify_friend.create_serialized(notification);

                s->deliver(notify_friend);

                // Call user manager to update blocks.
                std::string user_username = s->get_user_data().username;

                user_manager_->on_friend_user(user,
                                              std::move(user_username),
                                              sender);
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
        std::string lmsg = "Exception in do_respond_friend_request: "
                            + std::string(e.what());
        Console::instance().log(std::move(lmsg),
                                LogLevel::ERROR);
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
                    blocked = std::move(blocked),
                    blocked_id = std::move(blocked_id),
                    s = std::move(s)]() mutable {
// START LAMBDA FOR UUID BASED STRAND
            try
            {
                prepares();

                pqxx::work txn{*conn_};

                // Block the user, idempotent.
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

                // Send notification to caller that user is blocked.
                NotifyRelationUpdate notification;
                notification.user.user_id = blocked_id;
                notification.user.username = blocked;

                Message notify_block;
                notify_block.header.type_ = HeaderType::NotifyBlocked;
                notify_block.create_serialized(notification);

                s->deliver(notify_block);

                // Call user manager to update blocks.
                user_manager_->on_block_user(blocker, blocked_id);
            }
            catch (const std::exception & e)
            {
                std::string lmsg = "Exception in do_block_user: "
                                    + std::string(e.what());
                Console::instance().log(std::move(lmsg),
                                        LogLevel::ERROR);
            }
                });
// END LAMBDA FOR UUID BASED STRAND
            }
            // No user found, exit.
            else
            {
                Console::instance().log("No user in do_block_user",
                                        LogLevel::INFO);
            }
            txn.commit();
        }
        catch (const std::exception & e)
        {
            std::string lmsg = "Exception in do_block_user: "
                                + std::string(e.what());
            Console::instance().log(std::move(lmsg),
                                    LogLevel::ERROR);
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
        s = std::move(session)]() mutable {
// START LAMBDA FOR UUID BASED STRAND
    try
    {
        prepares();

        pqxx::work txn{*conn_};

        txn.exec_prepared("unblock_user",
                          boost::uuids::to_string(blocker),
                          boost::uuids::to_string(blocked_id));

        txn.commit();

        // Send notification to caller that user is unblocked.
        NotifyRelationUpdate notification;
        notification.user.user_id = blocked_id;

        // Username is unnecessary
        notification.user.username.clear();

        Message notify_unblock;
        notify_unblock.header.type_ = HeaderType::NotifyUnblocked;
        notify_unblock.create_serialized(notification);

        s->deliver(notify_unblock);

        // Call user manager to update blocks.
        user_manager_->on_unblock_user(blocker, blocked_id);
    }
    catch (const std::exception & e)
    {
        std::string lmsg = "Exception in do_unblock_user: "
                            + std::string(e.what());
        Console::instance().log(std::move(lmsg),
                                LogLevel::ERROR);
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

        // Send notification to caller that user is unblocked.
        NotifyRelationUpdate notification;
        notification.user.user_id = friend_id;

        // Username is unnecessary
        notification.user.username.clear();

        Message notify_unfriend;
        notify_unfriend.header.type_ = HeaderType::NotifyFriendRemoved;
        notify_unfriend.create_serialized(notification);

        s->deliver(notify_unfriend);

        // Call user manager to update blocks.
        user_manager_->on_unfriend_user(user, friend_id);
    }
    catch (const std::exception & e)
    {
        std::string lmsg = "Exception in do_remove_friend: "
                            + std::string(e.what());
        Console::instance().log(std::move(lmsg),
                                LogLevel::ERROR);
    }
    });
// END LAMBDA FOR UUID BASED STRAND
}

void Database::do_fetch_blocks(boost::uuids::uuid user,
                               std::shared_ptr<Session> session)
{
    uuid_strands_.post(user,
        [this,
        user = std::move(user),
        s = std::move(session)]() mutable {

    try
    {
        prepares();

        pqxx::work txn{*conn_};

        auto blocks_res = txn.exec_prepared("fetch_blocks",
                                            boost::uuids::to_string(user));

        // This transaction is read only. Commit now.
        txn.commit();

        UserList blocked_users;

        // Go through the blocked user list if it exists.
        if(!blocks_res.empty())
        {
            boost::uuids::string_generator gen;

            for (const auto & row : blocks_res)
            {
                ExternalUser blocked_user;

                blocked_user.user_id = gen(row["user_id"].c_str());
                blocked_user.username = row["username"].as<std::string>();

                blocked_users.users.push_back(blocked_user);
            }
        }

        // Turn the data into a message and send it to the client.
        Message blocks_msg;
        blocks_msg.header.type_ = HeaderType::BlockList;
        blocks_msg.create_serialized(blocked_users);
        s->deliver(blocks_msg);

    }
    catch (const std::exception & e)
    {
        std::string lmsg = "Exception in do_fetch_blocks: "
                            + std::string(e.what());
        Console::instance().log(std::move(lmsg),
                                LogLevel::ERROR);
    }
    });
}

void Database::do_fetch_friends(boost::uuids::uuid user,
                                std::shared_ptr<Session> session)
{
    uuid_strands_.post(user,
        [this,
        user = std::move(user),
        s = std::move(session)]() mutable {

    try
    {
        prepares();

        pqxx::work txn{*conn_};

        auto friends_res = txn.exec_prepared("fetch_friends",
                                             boost::uuids::to_string(user));

        // This transaction is read only. Commit now.
        txn.commit();

        UserList friends;

        // Go through the friends list if it exists.
        if(!friends_res.empty())
        {
            boost::uuids::string_generator gen;

            for (const auto & row : friends_res)
            {
                ExternalUser curr_friend;

                curr_friend.user_id = gen(row["user_id"].c_str());
                curr_friend.username = row["username"].as<std::string>();

                friends.users.push_back(curr_friend);
            }
        }

        // Turn the data into a message and send it to the client.
        Message friends_msg;
        friends_msg.header.type_ = HeaderType::FriendList;
        friends_msg.create_serialized(friends);
        s->deliver(friends_msg);

    }
    catch (const std::exception & e)
    {
        std::string lmsg = "Exception in do_fetch_friends: "
                            + std::string(e.what());
        Console::instance().log(std::move(lmsg),
                                LogLevel::ERROR);
    }
    });
}

void Database::do_fetch_friend_requests(boost::uuids::uuid user,
                                        std::shared_ptr<Session> session)
{
    uuid_strands_.post(user,
        [this,
        user = std::move(user),
        s = std::move(session)]() mutable {

    try
    {
        prepares();

        pqxx::work txn{*conn_};

        auto friends_req_res = txn.exec_prepared("fetch_friend_requests",
                                             boost::uuids::to_string(user));

        // This transaction is read only. Commit now.
        txn.commit();

        UserList friend_requests;

        // Go through the friends list if it exists.
        if(!friends_req_res.empty())
        {
            boost::uuids::string_generator gen;

            for (const auto & row : friends_req_res)
            {
                ExternalUser curr_req;

                curr_req.user_id = gen(row["user_id"].c_str());
                curr_req.username = row["username"].as<std::string>();

                friend_requests.users.push_back(curr_req);
            }
        }

        // Turn the data into a message and send it to the client.
        Message friends_req_msg;
        friends_req_msg.header.type_ = HeaderType::FriendRequestList;
        friends_req_msg.create_serialized(friend_requests);
        s->deliver(friends_req_msg);

    }
    catch (const std::exception & e)
    {
        std::string lmsg = "Exception in do_fetch_friend_requests: "
                            + std::string(e.what());
        Console::instance().log(std::move(lmsg),
                                LogLevel::ERROR);
    }
    });
}

void Database::prepares()
{
    if (!conn_)
    {
        conn_ = std::make_unique<pqxx::connection>(
            "host=127.0.0.1 "
            "port=5432 "
            "dbname=SilentTanksDB "
            "user=SilentTanksOperator"
            );
        conn_->prepare("auth",
            "SELECT user_id, hash, salt "
            "FROM Users "
            "WHERE LOWER(username) = LOWER($1)"
            );
        conn_->prepare("reg",
            "INSERT INTO Users (user_id, username, hash, salt, last_ip) "
            "VALUES ($1, $2, $3, $4, $5)"
            );
        conn_->prepare("insert_matches",
            "INSERT INTO Matches(game_mode, settings, move_history) "
            "VALUES ($1, $2::jsonb, $3::jsonb) "
            "RETURNING match_id"
            );
        conn_->prepare("insert_player",
            "INSERT INTO MatchPlayers(match_id, user_id, "
            "player_id, placement) VALUES ($1, $2, $3, $4)"
            );
        conn_->prepare("get_elos",
            "SELECT user_id, current_elo "
            "FROM UserElos "
            "WHERE user_id = ANY($1::uuid[]) AND game_mode = $2 "
            );
        conn_->prepare("get_mode_elos",
            "SELECT user_id, current_elo, game_mode "
            "FROM UserElos "
            "WHERE user_id = $1"
            );
        conn_->prepare("update_elo",
            "INSERT INTO UserElos (user_id, game_mode, current_elo) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT (user_id, game_mode) "
            "DO UPDATE SET current_elo = EXCLUDED.current_elo"
            );
        conn_->prepare("record_elo_history",
            "INSERT INTO EloHistory (user_id, match_id, game_mode, "
                                    "old_elo, new_elo) "
            "VALUES ($1, $2, $3, $4, $5)"
            );
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
            "WHERE user_id = $2"
            );
        conn_->prepare("check_bans",
            "SELECT banned_at, (extract(epoch FROM banned_until)*1000)::bigint AS banned_ms, reason "
            "FROM UserBans "
            "WHERE user_id = $1 "
            " AND banned_until > now() "
            "ORDER BY banned_at DESC "
            "LIMIT 1"
            );
        conn_->prepare("add_user_elos",
            "INSERT INTO UserElos (user_id, game_mode, current_elo) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT DO NOTHING"
            );
        conn_->prepare("find_uuid",
            "SELECT user_id "
            "FROM Users "
            "WHERE LOWER(username) = LOWER($1)"
            );
        conn_->prepare("find_username",
            "SELECT username "
            "FROM Users "
            "WHERE user_id = $1"
            );
        conn_->prepare("ban_user",
            "INSERT INTO UserBans (user_id, banned_until, original_expiration, reason) "
            "VALUES ($1, $2, $2, $3)"
            );
        conn_->prepare("unban_user",
            "UPDATE UserBans "
            "SET banned_until = now() "
            "WHERE ban_id = $1"
            );
        conn_->prepare("friend_request",
            "WITH pending AS ( "
            "SELECT COUNT(*) AS cnt "
            "FROM FriendRequests "
            "WHERE sender = $1 "
            ") "
            "INSERT INTO FriendRequests (sender, receiver) "
            "SELECT $1, $2 "
            "FROM pending "
            "WHERE cnt < $3 "
            "ON CONFLICT DO NOTHING"
            );
        conn_->prepare("delete_friend_request",
            "DELETE FROM FriendRequests "
            "WHERE sender = $1 "
            " AND receiver = $2"
            );
        conn_->prepare("accept_friend",
            "INSERT INTO Friends (user_a, user_b) "
            "VALUES (LEAST($1::uuid, $2::uuid), GREATEST($1::uuid, $2::uuid)) "
            "ON CONFLICT DO NOTHING"
            );
        conn_->prepare("find_friend_request",
            "SELECT 1 "
            "FROM FriendRequests "
            "WHERE receiver = $1 AND sender = $2 "
            "LIMIT 1"
            );
        conn_->prepare("block_user",
            "INSERT INTO BlockedUsers (blocker, blocked) "
            "VALUES ($1, $2) "
            "ON CONFLICT DO NOTHING"
            );
        conn_->prepare("check_blocked",
            "SELECT 1 "
            "FROM BlockedUsers "
            "WHERE blocker = $1::uuid AND blocked = $2::uuid "
            "LIMIT 1"
            );
        conn_->prepare("check_friends",
            "SELECT 1 "
            "FROM Friends "
            "WHERE user_a = LEAST($1::uuid, $2::uuid) "
            "AND user_b = GREATEST($1::uuid, $2::uuid) "
            "LIMIT 1"
            );
        conn_->prepare("remove_friend",
            "DELETE FROM Friends "
            "WHERE (user_a = LEAST($1::uuid, $2::uuid) "
            "AND user_b = GREATEST($1::uuid, $2::uuid))"
            );
        conn_->prepare("unblock_user",
            "DELETE FROM BlockedUsers "
            "WHERE blocker = $1::uuid AND blocked = $2::uuid"
            );
        conn_->prepare("fetch_blocks",
            "SELECT b.blocked AS user_id, u.username AS username "
            "FROM BlockedUsers b JOIN Users u ON u.user_id = b.blocked "
            "WHERE b.blocker = $1"
            );
        conn_->prepare("fetch_friends",
            "SELECT (CASE "
            "WHEN f.user_a = $1 THEN f.user_b "
            "ELSE f.user_a "
            "END) AS friend_id, u.username AS username "
            "FROM Friends f "
            "JOIN Users u "
            "ON u.user_id = (CASE WHEN f.user_a = $1 THEN f.user_b "
            "ELSE f.user_a END) "
            "WHERE user_a = $1 OR user_b = $1"
            );
        conn_->prepare("fetch_friend_requests",
            "SELECT fr.sender AS user_id, u.username AS username "
            "FROM FriendRequests fr JOIN Users u ON u.user_id = fr.sender "
            "WHERE receiver = $1"
            );
        conn_->prepare("INTERNAL_fetch_friends",
            "SELECT (CASE "
            "WHEN f.user_a = $1 THEN f.user_b "
            "ELSE f.user_a "
            "END) AS friend_id "
            "FROM Friends f "
            "WHERE f.user_a = $1 OR f.user_b = $1"
            );
        conn_->prepare("INTERNAL_fetch_blocks",
            "SELECT b.blocked as blocked "
            "FROM BlockedUsers b "
            "WHERE b.blocker = $1"
            );
    }
}
