#pragma once

#include "client-data.h"

#include <QObject>
#include <QAbstractListModel>

class UserListModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum Roles {
        UsernameRole = Qt::UserRole + 1
    };

    Q_ENUM(Roles)

    explicit UserListModel(QObject* parent = nullptr);

    void set_users(const UserMap & user_map);

    int rowCount(const QModelIndex & parent) const override;

    QVariant data(const QModelIndex & index, int role) const override;

    // Map role enums to string names.
    QHash<int, QByteArray> roleNames() const override;

private:
    std::vector<ExternalUser> users_;
};
