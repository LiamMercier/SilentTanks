#pragma once

#include <QObject>
#include <QAbstractListModel>

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
    QList<QString> messages_;
};
