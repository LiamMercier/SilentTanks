#pragma once

#include <iostream>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <system_error>

namespace AppAssets {

    constexpr const char * application_name = "silent-tanks";

    inline std::filesystem::path join(const std::filesystem::path & base,
                                      const std::string & appended)
    {
        if (base.empty())
        {
            return {};
        }

        return base / appended;
    }

    inline std::filesystem::path get_env_path(const char * name)
    {
        // If we didn't get nullptr back return the path
        if (const char * v = std::getenv(name))
        {
            // prevent empty strings like XDG_DATA_HOME=""
            if (v[0] != '\0')
            {
                return std::filesystem::path(v);
            }
        }

        return {};
    }

    inline std::filesystem::path get_assets_dir()
    {

#ifdef _WIN32
        // Search for the local app data folder in the environment.
        auto pf = get_env_path("LOCALAPPDATA");

        // If this doesn't exist, asset lookup fails.
        if (pf.empty())
        {
            return {};
        }

        return pf / application_name;

#else
        // On linux, we want to use ~/.local/share

        // Start by looking for the XDG_DATA_HOME env variable.
        auto xdg = get_env_path("XDG_DATA_HOME");
        if (!xdg.empty())
        {
            return xdg / application_name;
        }

        // Try manual home if xdg isn't defined.
        auto home = get_env_path("HOME");
        if (!home.empty())
        {
            return home / ".local" / "share" / application_name;
        }

        return {};

#endif
    }

    inline bool fs_safe_exists(const std::filesystem::path & p)
    {
        if (p.empty())
        {
            return false;
        }

        std::error_code ec;
        bool exists = std::filesystem::exists(p, ec);

        // If ec was set, return false.
        return (exists && !ec);
    }

    // Take a filename and resolve an asset, returning an empty path if it failed.
    inline std::filesystem::path resolve_asset(const std::string & filename)
    {
        // If we set an env path for this application, use this instead.
        {
            auto override_dir = get_env_path("SILENTTANKS_ASSET_DIR");
            if (!override_dir.empty())
            {
                auto candidate = join(override_dir, filename);
                if (fs_safe_exists(candidate))
                {
                    return candidate;
                }

                return {};
            }
        }

        // Otherwise, if a dev mode flag was set use this
#if defined(DEV_BUILD)
        {
            std::error_code ec;
            std::filesystem::path cwd = std::filesystem::current_path(ec);

            // Prevent using cwd if empty or does not exist.
            if (!ec && !cwd.empty())
            {
                auto devpath = cwd / filename;

                if (fs_safe_exists(devpath))
                {
                    return devpath;
                }
            }

            return {};
        }
#else

        // For release, grab the asset folder and join.
        {
            std::filesystem::path assets_dir = get_assets_dir();

            if (assets_dir.empty())
            {
                return {};
            }

            auto candidate = join(assets_dir, filename);

            if (fs_safe_exists(candidate))
            {
                return candidate;
            }

            return {};
        }
#endif

    }

    // Try to copy missing files when an asset does not exist at startup.
    inline bool try_copy_missing_file(const std::filesystem::path & src,
                                      const std::filesystem::path & destination)
    {
        namespace fs = std::filesystem;
        std::error_code ec;

        if (!fs::exists(src, ec))
        {
            std::cerr << "Failed to find "
                          << src
                          << " (does not exist)\n";
            return false;
        }

        if (fs::is_directory(src, ec))
        {
            if (ec)
            {
                std::cerr << "Error checking source directory "
                          << ec.message()
                          << "\n";
                return false;
            }

            fs::copy(src,
                     destination,
                     fs::copy_options::recursive
                     | fs::copy_options::copy_symlinks
                     | fs::copy_options::skip_existing,
                     ec);

            if (ec)
            {
                std::cerr << "Failed to copy directory "
                          << src
                          << " to "
                          << destination
                          << " error:"
                          << ec.message()
                          << "\n";

                return false;
            }
        }
        // Handle source files or symlinks.
        else
        {
            auto parent = destination.parent_path();
            if (!parent.empty() && !fs::exists(parent, ec))
            {
                fs::create_directories(destination.parent_path(), ec);

                if (ec)
                {
                    std::cerr << "Failed to create destination parent "
                              << parent
                              << " error: "
                              << ec.message()
                              << "\n";

                    return false;
                }
            }

            fs::copy_file(src, destination, fs::copy_options::skip_existing, ec);
            if (ec)
            {
                std::cerr << "Failed to copy file "
                          << src
                          << " to "
                          << destination
                          << " error: "
                          << ec.message()
                          << "\n";

                return false;
            }
        }

        // Try to set owner perms for files.
        std::error_code perm_ec;
        fs::permissions(destination, fs::perms::owner_all,
                        fs::perm_options::add,
                        perm_ec);

        if (ec)
        {
            return false;
        }

        return true;
    }

    // Go through each expected location and check the asset can be resolved.
    // Otherwise, fail fast and report back to the user.
    //
    // This function is called at the start of the server.
    inline bool assert_server_assets_resolvable()
    {
        bool resolvable = true;

        std::vector<std::string> asset_names = {
            // Server TLS files
            "certs/server.crt",
            "certs/server.key",

            // List of maps
            "mapfile.txt",

            // Maps directory
            "envs/"
        };

        for (const auto & filename : asset_names)
        {
            std::filesystem::path path = resolve_asset(filename);

            if (path.empty())
            {
                std::cerr << "Failed to resolve "
                          << filename
                          << " (resolved to "
                          << path
                          << ")\n";

                std::cerr << "Env filepath: "
                          << get_env_path("SILENTTANKS_ASSET_DIR")
                          << "\n";

                resolvable = false;
            }
        }

        return resolvable;
    }

    // Same case, but for client specific files and directories.
    inline bool assert_client_assets_resolvable()
    {
        bool resolvable = true;

        std::vector<std::string> asset_names = {
            "mapfile.txt",
            "server-list.txt",
            "envs"
        };

// TODO: windows specific copy path.
#ifdef _WIN32
        std::filesystem::path package_share = "";

// Linux copy path, uses /usr/share/silent-tanks
#else
        std::filesystem::path package_share = "/usr/share";
        package_share = package_share / application_name;

        if (package_share.empty())
        {
            std::cerr << "Warning: usr/share/"
                      << application_name
                      << " not found.\n";
        }

        std::filesystem::path user_dir = get_assets_dir();

        if (user_dir.empty())
        {
            std::cerr << "User asset directory not found.\n";
            return false;
        }
#endif

        for (const auto & filename : asset_names)
        {
            std::filesystem::path path = resolve_asset(filename);

            if (path.empty())
            {
                std::cerr << "Failed to resolve "
                          << filename
                          << " (resolved to "
                          << path
                          << ")\n";

                // Try to copy the file if possible.
                std::filesystem::path copy_path = join(package_share, filename);

                if (copy_path.empty())
                {
                    std::cerr << "Copy path not available for "
                              << filename
                              << '\n';

                    resolvable = false;
                    continue;
                }

                std::filesystem::path dest = user_dir / filename;
                bool copied = try_copy_missing_file(copy_path, dest);

                if (!copied)
                {
                    std::cerr << "Failed to copy "
                              << copy_path
                              << " to "
                              << path
                              << "\n";
                }

                resolvable = copied;
            }
        }

        return resolvable;
    }

}
