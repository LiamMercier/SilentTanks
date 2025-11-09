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

#pragma once

#include <deque>

#include <QObject>
#include <QAbstractListModel>

constexpr int MAX_MESSAGES = 250;

class ChatMessageModel : public QAbstractListModel
{
    Q_OBJECT

public:

    enum Roles {
        MessageRole = Qt::UserRole + 1
    };

    Q_ENUM(Roles)

    explicit ChatMessageModel(QObject* parent = nullptr);

    void add_message(std::string message);

    int rowCount(const QModelIndex & parent) const override;

    QVariant data(const QModelIndex & index, int role) const override;

    // Map role enums to string names.
    QHash<int, QByteArray> roleNames() const override;

private:
    std::deque<QString> messages_;
};
