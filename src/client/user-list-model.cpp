#include "user-list-model.h"

#include <boost/uuid/uuid_io.hpp>

UserListModel::UserListModel(QObject* parent)
:QAbstractListModel(parent)
{

}

void UserListModel::set_users(const UserMap & user_map)
{
    beginResetModel();
    users_.clear();
    for (const auto & [id, user] : user_map)
    {
        users_.push_back(user);
    }
    endResetModel();
}

void UserListModel::set_users(const UserList & user_list)
{
    beginResetModel();
    users_ = user_list.users;
    endResetModel();
}

int UserListModel::rowCount(const QModelIndex & parent = QModelIndex()) const
{
    return static_cast<int>(users_.size());
}

QVariant UserListModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()
        || index.row() >= static_cast<int>(users_.size())
        || index.row() < 0)
    {
        return {};
    }

    const ExternalUser & user = users_[index.row()];

    switch (role)
    {
        case UsernameRole:
            return QString::fromStdString(user.username);
        case UUIDRole:
            return QString::fromStdString(boost::uuids::to_string(user.user_id));
        default:
            return {};
    }
}

QHash<int, QByteArray> UserListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UsernameRole] = "username";
    roles[UUIDRole] = "uuid";
    return roles;
}
