#include "database.h"

// We expect that a file exists at ~/.pgpass
Database::Database(asio::io_context& io,
         AuthCallback auth_callback)
: strand_(io.get_executor()),
thread_pool_(MAX_CONCURRENT_AUTHS),
auth_callback_(std::move(auth_callback))
{}

void Database::authenticate(Message msg, std::shared_ptr<Session> session)
{
    asio::post(strand_, [this, m = std::move(msg), s = std::move(session)]
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
        do_auth(std::move(request), std::move(s));

    });
}

void Database::register_account(Message msg, std::shared_ptr<Session> session)
{
    asio::post(strand_, [this, m = std::move(msg), s = std::move(session)]{

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
        do_register(std::move(request), std::move(s));

    });

}

// One connection to the database per thread in the thread pool.
static thread_local std::unique_ptr<pqxx::connection> conn_;

void Database::do_auth(LoginRequest request, std::shared_ptr<Session> session)
{
    asio::post(thread_pool_, [this, req=std::move(request), s = std::move(session)]() mutable{

        UserData data;

        try
        {
            if (!conn_)
            {
                conn_ = std::make_unique<pqxx::connection>();
                conn_->prepare("auth",
                              "SELECT uuid, hash, salt \
                              FROM Users \
                              WHERE username = $1");
                conn_->prepare("reg",
                              "INSERT INTO Users (uuid, username, hash, salt) \
                              VALUES ($1, $2, $3, $4)");
            }

            pqxx::work txn{*conn_};
            auto res = txn.exec_prepared("auth",
                                         req.username);

            if (!res.empty())
            {
                auto row = res[0];

                // get hash and salt from DB row (as string).
                //
                // this is allowed because std::string is not
                // null terminated.
                std::string hash_bytes = row["hash"].as<std::string>();
                std::string salt_bytes = row["salt"].as<std::string>();

                if (hash_bytes.size() != HASH_LENGTH ||
                    salt_bytes.size() != SALT_LENGTH)
                {
                    std::cerr << "DB call had wrong hashing lengths!\n";
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

                // First, take our 32 byte H(password) from the user, and compute
                // H(H(password), salt) using libargon2
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
                        // Turn uuid data into a boost UUID.
                        std::string uuid_str = row["uuid"].as<std::string>();
                        boost::uuids::string_generator gen;
                        data.user_id = gen(uuid_str);

                        data.username = req.username;
                    }
                }
                // Bad function call.
                else
                {
                    std::cerr << "Argon2 hashing failed " << argon2_error_message(result) << "\n";
                }

            }
            else
            {
                // Empty, failed to find user.
            }

            txn.commit();
        }

        catch(const pqxx::sql_error& e)
        {
            std::cerr << "Database error: " << e.what() << "\n";
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
}

void Database::do_register(LoginRequest request, std::shared_ptr<Session> session)
{
    asio::post(thread_pool_, [this, req=std::move(request), s = std::move(session)]() mutable{

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
            if (!conn_)
            {
                conn_ = std::make_unique<pqxx::connection>();
                conn_->prepare("auth",
                              "SELECT uuid, hash, salt \
                              FROM Users \
                              WHERE username = $1");
                conn_->prepare("reg",
                              "INSERT INTO Users (uuid, username, hash, salt) \
                              VALUES ($1, $2, $3, $4)");
            }

            pqxx::work txn{*conn_};

            // Create a random UUID for database insert
            boost::uuids::uuid user_id = boost::uuids::random_generator()();

            txn.exec_prepared(
                "reg",
                boost::uuids::to_string(user_id),
                req.username,
                pqxx::binarystring(reinterpret_cast<const char*>(computed_hash.data()), computed_hash.size()),
                pqxx::binarystring(reinterpret_cast<const char*>(salt.data()), salt.size()));

            txn.commit();

            // Tell client that registration was successful
            Message good_reg;
            good_reg.create_serialized(HeaderType::GoodRegistration);
            s->deliver(good_reg);
            return;
        }

        catch (const pqxx::unique_violation & e)
        {
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
