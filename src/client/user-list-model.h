#pragma once

#include "client-data.h"

#include <QObject>
#include <QAbstractListModel>

class UserListModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum Roles {
        UsernameRole = Qt::UserRole + 1,
        UUIDRole,
        TimeRole
    };

    Q_ENUM(Roles)

    explicit UserListModel(QObject* parent = nullptr);

    void set_users(const UserMap & user_map);

    void set_users(const UserList & user_list);

    void set_timers(const std::vector<std::chrono::milliseconds> & timers);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;

    QVariant data(const QModelIndex & index, int role) const override;

    // Map role enums to string names.
    QHash<int, QByteArray> roleNames() const override;

    size_t get_size() const;

    ExternalUser get_user(size_t index) const;

private:
    std::vector<ExternalUser> users_;
    std::vector<std::chrono::milliseconds> timers_;
};
