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

#include "message-model.h"

ChatMessageModel::ChatMessageModel(QObject* parent)
:QAbstractListModel(parent)
{

}

// TODO <optimization>: create message limit.
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
