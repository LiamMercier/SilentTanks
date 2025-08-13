#include "message-model.h"

ChatMessageModel::ChatMessageModel(QObject* parent)
:QAbstractListModel(parent)
{

}

// TODO: create message limit.
void ChatMessageModel::add_message(std::string message)
{
    QString msg = QString::fromStdString(message);

    beginInsertRows(QModelIndex(), messages_.size(), messages_.size());

    messages_.append(msg);

    endInsertRows();
}

int ChatMessageModel::rowCount(const QModelIndex & parent = QModelIndex()) const
{
    return static_cast<int>(messages_.size());
}

QVariant ChatMessageModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()
        || index.row() >= static_cast<int>(messages_.size())
        || index.row() < 0)
    {
        return {};
    }

    const QString & message = messages_[index.row()];

    switch (role)
    {
        case MessageRole:
            return message;
        default:
            return {};
    }
}

QHash<int, QByteArray> ChatMessageModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[MessageRole] = "message";
    return roles;
}
