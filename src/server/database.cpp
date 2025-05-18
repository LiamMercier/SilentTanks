#include "database.h"

Database(asio::io_context& io,
         const std::string & conn_str,
         AuthCallback auth_callback)
: strand_(io.get_executor()),
thread_pool_(MAX_CONCURRENT_AUTHS),
conn_(conn_str),
auth_callback_(std::move(auth_callback))
{}

void Database::authenticate(Message msg, std::shared_ptr<Session> session)
{
    asio::post(strand_, [this, m = std::move(msg), s = std::move(session)]
    {

        // First, turn the message into an authentication attempt
        LoginRequest request = msg.to_login_request();

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

void Database::do_auth(LoginRequest request, std::shared_ptr<Session> session)
{
    asio::post(thread_pool_, [this, req=std::move(request), s = std::move(session)]() mutable{
        UserData data;

        try
        {
            pqxx::work txn{conn_};
            auto res = txn.exec_params(
                "SELECT uuid, hash FROM Users WHERE username = $1",
                req.username);

            if (!res.empty())
            {
                auto good_hash = res[0]["hash"].as<std::array<uint8_t, HASH_LENGTH>();

                auto user_salt = res[0]["salt"].as<std::array<uint8_t, SALT_LENGTH>();

                // First, take our 32 byte H(password) from the user, and compute
                // H(H(password), salt) using libargon2
                std::array<uint8_t, HASH_LENGTH> computed_hash(HASH_LENGTH);
                int result = argon2id_hash_raw(
                    ARGON2_TIME,
                    ARGON2_MEMORY,
                    ARGON2_PARALLEL,
                    reinterpret_cast<const uint8_t*>(req.pwd.data()),
                    req.pwd.size(),
                    user_salt.data(),
                    user_salt.size(),
                    computed_hash.data(),
                    computed_hash.size(),
                    nullptr,
                    0);

                // If function hashed successfully, compare hashes.
                if (result == ARGON2_OK)
                {
                    // If correct, password matched and we can authenticate.
                    if (computed_hash == good_hash)
                    {
                        data.user_id = res[0]["uuid"].as<boost::uuids::uuid>();
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
            txn.rollback();
        }
        catch(const std::exception& e)
        {
            std::cerr << "Unexpected exception occured during auth " << e.what() << "\n";
            txn.rollback();
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
