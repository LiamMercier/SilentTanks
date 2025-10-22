#include "server-list.h"

#include "asset-resolver.h"
#include "server-identity.h"

#include <fstream>

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

ServerList::ServerList(QObject* parent)
:QAbstractListModel(parent)
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

void ServerList::initialize_server_list(std::vector<ServerIdentity> server_identities)
{
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
