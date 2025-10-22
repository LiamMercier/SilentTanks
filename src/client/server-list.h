#pragma once

#include <QObject>
#include <QAbstractListModel>

#include <unordered_map>

#include "server-identity.h"

bool read_server_list(std::string server_list_filename,
                      std::vector<ServerIdentity> & output_list);

class ServerList : public QAbstractListModel
{
    Q_OBJECT

public:

    enum Roles {
        AddressRole = Qt::UserRole + 1,
        PortRole,
        NameRole
    };

    Q_ENUM(Roles)

    explicit ServerList(QObject* parent = nullptr);

    int rowCount(const QModelIndex & parent) const override;

    QVariant data(const QModelIndex & index, int role) const override;

    // Map role enums to string names.
    QHash<int, QByteArray> roleNames() const override;

    void initialize_server_list(std::vector<ServerIdentity> server_identities);

signals:

private:;
    std::unordered_map<std::string, size_t> identity_to_index_;

    std::vector<ServerIdentity> server_identities_;
};
