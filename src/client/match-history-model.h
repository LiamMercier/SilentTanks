#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QDateTime>
#include <QTimeZone>

#include "match-result-structs.h"

class MatchHistoryModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int current_history_mode
               READ current_history_mode
               NOTIFY current_history_mode_changed)

public:

    enum Roles {
        MatchIDRole = Qt::UserRole + 1,
        FinishedAtRole,
        PlacementRole,
        EloRole
    };

    Q_ENUM(Roles)

    explicit MatchHistoryModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex & parent) const override;

    QVariant data(const QModelIndex & index, int role) const override;

    // Map role enums to string names.
    QHash<int, QByteArray> roleNames() const override;

    int current_history_mode() const;

    Q_INVOKABLE void set_current_history_mode(int mode);

    void set_history_row(MatchResultList results);

signals:
    void current_history_mode_changed(int newMode);

private:
    std::vector<std::vector<MatchResultRow>> history_rows_;
    int current_history_mode_{0};
};
