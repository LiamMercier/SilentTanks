#pragma once

#include <QObject>
#include <QAbstractListModel>

#include <unordered_map>

#include "client-constants.h"
#include "server-identity.h"

bool read_server_list(std::string server_list_filename,
                      std::vector<ServerIdentity> & output_list);

class ServerList : public QAbstractListModel
{
    Q_OBJECT

public:

    enum Roles {
        NameRole = Qt::UserRole + 1,
        AddressRole,
        PortRole,
        FingerprintRole
    };

    Q_ENUM(Roles)

    explicit ServerList(std::string server_list_filename,
                        QObject* parent = nullptr);

    int rowCount(const QModelIndex & parent) const override;

    QVariant data(const QModelIndex & index, int role) const override;

    // Map role enums to string names.
    QHash<int, QByteArray> roleNames() const override;

    void save_server_list_to_disk(std::string & out_err);

    void initialize_server_list(std::vector<ServerIdentity> server_identities);

    void add_server_identity(ServerIdentity identity);

    void remove_server_identity(const ServerIdentity & identity);

signals:

private:;
    std::string server_list_filename_;

    mutable std::mutex servers_mutex_;

    // TODO <optimization>: store ServerIdentity* instead of size_t so we
    // do not need to do an O(n) pass over the map on identity removal.
    //
    // Not a big improvement since we likely wont have thousands of servers.
    std::unordered_map<std::string, size_t> identity_to_index_;
    std::vector<ServerIdentity> server_identities_;
};
