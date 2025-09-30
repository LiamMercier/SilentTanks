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

void UserListModel::set_timers(const std::vector<std::chrono::milliseconds> & timers)
{
    timers_ = timers;

    const int rcount = rowCount();

    if (rcount > 0)
    {
        const QModelIndex top = index(0, 0);
        const QModelIndex bottom = index(rcount - 1, 0);

        // Notify that only this role changed so we aren't resetting the model
        // and thus delegates for each view computation.
        QVector<int> roles;
        roles.append(TimeRole);
        emit dataChanged(top, bottom, roles);
    }
}

int UserListModel::rowCount(const QModelIndex & parent) const
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

    switch (role)
    {
        case UsernameRole:
        {
            const ExternalUser & user = users_[index.row()];
            return QString::fromStdString(user.username);
        }
        case UUIDRole:
        {
            const ExternalUser & user = users_[index.row()];
            return QString::fromStdString(boost::uuids::to_string(user.user_id));
        }
        case TimeRole:
        {
            if (index.row() >= static_cast<int>(timers_.size()))
            {
                return {};
            }

            std::chrono::milliseconds time = timers_[index.row()];
            return static_cast<qint64>(time.count());
        }
        default:
            return {};
    }
}

QHash<int, QByteArray> UserListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[UsernameRole] = "username";
    roles[UUIDRole] = "uuid";
    roles[TimeRole] = "timer";
    return roles;
}
