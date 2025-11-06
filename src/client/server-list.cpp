// Copyright (c) 2025 Liam Mercier
//
// This file is part of SilentTanks.
//
// SilentTanks is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License Version 3.0
// as published by the Free Software Foundation.
//
// SilentTanks is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License v3.0
// for more details.
//
// You should have received a copy of the GNU Affero General Public License v3.0
// along with SilentTanks. If not, see <https://www.gnu.org/licenses/agpl-3.0.txt>

#include "server-list.h"

#include "generic-constants.h"
#include "asset-resolver.h"
#include "server-identity.h"

#include <fstream>
#include <chrono>
#include <random>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <fcntl.h>
    #include <unistd.h>
#endif

bool read_server_list(std::string server_list_filename,
                      std::vector<ServerIdentity> & output_list)
{
    std::filesystem::path list_path = AppAssets::resolve_asset(server_list_filename);

    if (list_path.empty())
    {
        return false;
    }

    // Iterate through the server list and try to parse each line.
    std::ifstream servers_file(list_path);

    // If we failed to open the file, stop.
    if (!servers_file.is_open())
    {
        return false;
    }

    bool failed = false;

    std::string line;
    while (std::getline(servers_file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        // Try to convert the line to a server identity.
        ServerIdentity current_server;

        bool success = current_server.try_parse_list_line(line);

        if (success)
        {
            output_list.push_back(current_server);
        }
        else
        {
            failed = true;
        }
    }

    return !failed;
}

//
// START server-list.cpp specific filesystem functions.
// We need these for cross platform compatibility, very ugly since
// windows API has to be used from what I looked into.
//

// Take a directory and generate a temporary path name using the current
// time and a basic random generator.
//
// example input: dir = "/", base_name = "server-lists.txt"
//         output: "/server-lists.txt.tmp.<TIMESTAMP>.<RANDOM_UINT64_T>"
inline std::filesystem::path make_temp_path(const std::filesystem::path & dir,
                                            const std::string & base_name)
{
    // Fetch the current time and try to randomize the data with the
    // pointer for increased volatility.
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();

    std::mt19937_64 rng(static_cast<unsigned long long>
                        (
                            now ^ reinterpret_cast<uintptr_t>(&rng)
                        ));

    std::uniform_int_distribution<uint64_t> dist;

    uint64_t n = dist(rng);
    std::string suffix = ".tmp." + std::to_string(now) + "." + std::to_string(n);

    return dir / (base_name + suffix);
}

// Forces the operating system to write to disk instead of caching.
// std::filesystem does not seem to do this natively.
//
// We need this to ensure power loss doesn't destroy our list
// or a user program / file explorer doesn't interfere.
inline bool flush_file_disk(const std::filesystem::path & file)
{
// Windows specific flush
#ifdef _WIN32
    std::wstring wpath = file.wstring();
    HANDLE h = CreateFileW(wpath.c_str(),
                           GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           nullptr);

    if (h == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    BOOL ok = FlushFileBuffers(h);
    CloseHandle(h);
    return ok == TRUE;

// On linux we simply need to grab the file descriptor and call fsync.
#else
    int file_desc = ::open(file.c_str(), O_RDONLY);

    if (file_desc == -1)
    {
        return false;
    }

    int rc = ::fsync(file_desc);
    ::close(file_desc);
    return rc == 0;
#endif
}

// Util to swap files on disk.
inline bool atomic_replace(const std::filesystem::path & replacement_path,
                           const std::filesystem::path & target_path,
                           std::error_code & ec)
{
    ec.clear();
#ifdef _WIN32
    // Convert to wide string for windows specific filenames.
    std::wstring target_w = target_path.wstring();
    std::wstring repl_w = replacement_path.wstring();

    // Try to replace the file on disk.
    BOOL res = ReplaceFileW(target_w.c_str(),
                            repl_w.c_str(),
                            nullptr,
                            REPLACEFILE_WRITE_THROUGH,
                            nullptr,
                            nullptr);

    if (res)
    {
        return true;
    }

    // If we failed, try to move with replacement flag set.
    if (GetLastError() != ERROR_SUCCESS)
    {
        if (MoveFileExW(repl_w.c_str(),
                        target_w.c_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            return true;
        }
    }

    // Give an error back in case we want to do handling in the future.
    ec = std::make_error_code(static_cast<std::errc>(GetLastError()));
    return false;
#else
    // On linux, simply use std::filesystem and use the ec.
    std::filesystem::rename(replacement_path,
                            target_path,
                            ec);

    // If good, return true.
    if (!ec)
    {
        return true;
    }

    return false;
#endif
}

// Write all lines to disk and replace the old file atomically. Errors are
// no-op and result in out_error to print to console.
inline bool atomic_write_lines(const std::filesystem::path & target_path,
                               std::vector<std::string> lines,
                               std::string & out_error)
{
    out_error.clear();
    std::error_code ec;

    auto dir = target_path.parent_path();
    if (dir.empty())
    {
        dir = ".";
    }

    auto base = target_path.filename().string();
    auto temp_path = make_temp_path(dir, base);

    // First, create a temporary file.
    {
        std::ofstream ofs(temp_path, std::ios::binary | std::ios::trunc);

        if (!ofs.good())
        {
            out_error = "Failed to open/create temp file: "
                        + temp_path.string();
            std::error_code rm_ec;
            std::filesystem::remove(temp_path, rm_ec);
            return false;
        }

        // Write each line to the temporary file.
        for (const auto & ln : lines)
        {
            ofs << ln << "\n";

            if (!ofs.good())
            {
                out_error = "Failed while writing line to temp file: "
                            + temp_path.string();
                ofs.close();
                std::error_code rm_ec;
                std::filesystem::remove(temp_path, rm_ec);
                return false;
            }
        }

        // Flush from the buffer to the file descriptor, possibly stored in an OS cache.
        ofs.flush();

        if (!ofs.good())
        {
            out_error = "Failed to flush to temp file: "
                        + temp_path.string();
            std::error_code rm_ec;
            std::filesystem::remove(temp_path, rm_ec);
            return false;
        }
    }

    // Try to force the data to disk if it is cached.
    if (!flush_file_disk(temp_path))
    {
        out_error = "Flushing to disk failed for temp file: "
                    + temp_path.string();
        std::error_code rm_ec;
        std::filesystem::remove(temp_path, rm_ec);
        return false;
    }

    // Now try to replace the file with our new file.
    if (!atomic_replace(temp_path, target_path, ec))
    {
        out_error = "Atomic replace failed: "
                    + ec.message()
                    + "\n";

        std::error_code rm_ec;
        std::filesystem::remove(temp_path, rm_ec);
        return false;
    }

    // If everything went well, return true.
    return true;
}
//
// END server-list.cpp specific filesystem functions.
//

ServerList::ServerList(std::string server_list_filename,
                       QObject* parent)
:QAbstractListModel(parent),
server_list_filename_(server_list_filename)
{
}

int ServerList::rowCount(const QModelIndex & parent = QModelIndex()) const
{
    return static_cast<int>(server_identities_.size());
}

QVariant ServerList::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()
        || index.row() >= static_cast<int>(server_identities_.size())
        || index.row() < 0)
    {
        return {};
    }

    const ServerIdentity & identity = server_identities_[index.row()];

    switch (role)
    {
        case NameRole:
        {
            return QString::fromStdString(identity.name);
        }
        case AddressRole:
        {
            return QString::fromStdString(identity.address);
        }
        case PortRole:
        {
            return identity.port;
        }
        case FingerprintRole:
        {
            return QString::fromStdString(identity.display_hash);
        }
        default:
            return {};
    }
}

QHash<int, QByteArray> ServerList::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[AddressRole] = "address";
    roles[PortRole] = "port";
    roles[FingerprintRole] = "fingerprint";
    return roles;
}

void ServerList::save_server_list_to_disk(std::string & out_err)
{
    std::vector<std::string> lines;
    lines.reserve(server_identities_.size());

    for (const auto & id : server_identities_)
    {
        lines.push_back(id.get_file_line_string());
    }

    std::filesystem::path path = AppAssets::resolve_asset(server_list_filename_);

    atomic_write_lines(path, lines, out_err);
}

void ServerList::initialize_server_list(std::vector<ServerIdentity> server_identities)
{
    std::lock_guard lock(servers_mutex_);

    server_identities_.clear();
    identity_to_index_.clear();

    // Perform duplicate removal on server list load.
    std::vector<ServerIdentity> filtered_server_list;

    filtered_server_list.reserve(server_identities.size());
    identity_to_index_.reserve(server_identities.size());

    for (auto & identity : server_identities)
    {
        // Get a string ready for hashing.
        std::string key = identity.get_hashmap_string();

        // Skip duplicates.
        if (identity_to_index_.contains(key))
        {
            continue;
        }

        size_t curr_index = filtered_server_list.size();
        filtered_server_list.push_back(std::move(identity));
        identity_to_index_[key] = curr_index;
    }

    beginResetModel();
    server_identities_ = std::move(filtered_server_list);
    endResetModel();
}

void ServerList::add_server_identity(ServerIdentity identity)
{
    std::lock_guard lock(servers_mutex_);

    // Make sure the hash has expected length.
    if (identity.display_hash.size() != ServerIdentity::EXPECTED_HASH_LENGTH)
    {
        identity.display_hash = make_empty_hash_string
                                        (
                                            ServerIdentity::EXPECTED_HASH_LENGTH
                                        );
    }

    std::string key = identity.get_hashmap_string();

    // Prevent adding duplicate servers.
    if (identity_to_index_.contains(key))
    {
        return;
    }

    // Otherwise, add to the map.

    beginResetModel();

    size_t curr_index = server_identities_.size();
    server_identities_.push_back(std::move(identity));
    identity_to_index_[key] = curr_index;

    endResetModel();

    // Now, save the server list to disk.
    std::string err_report;
    save_server_list_to_disk(err_report);

    if (!err_report.empty())
    {
        std::cerr << "ERROR WRITING FILE: "
                  << err_report
                  << "\n";

        identity_to_index_.erase(key);
        server_identities_.pop_back();
    }

}

void ServerList::remove_server_identity(const ServerIdentity & identity)
{
    std::lock_guard lock(servers_mutex_);

    // Find the identity if it exists.
    std::string key = identity.get_hashmap_string();

    auto iter = identity_to_index_.find(key);
    if (iter == identity_to_index_.end())
    {
        return;
    }

    size_t removed_index = iter->second;

    beginRemoveRows(QModelIndex(), removed_index, removed_index);
    server_identities_.erase(server_identities_.begin() + removed_index);
    endRemoveRows();

    identity_to_index_.erase(iter);

    // Update indexes
    for (auto & [k, index] : identity_to_index_)
    {
        if (index > removed_index)
        {
            index -= 1;
        }
    }

    // Save the server list to disk.
    std::string err_report;
    save_server_list_to_disk(err_report);

    if (!err_report.empty())
    {
        std::cerr << "ERROR WRITING FILE: "
                  << err_report
                  << "\n";
    }
}
