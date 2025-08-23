#pragma once

#include <QAbstractListModel>
#include <QVariantMap>
#include <QObject>

#include "player-view.h"

class GameManager : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int map_width
               READ map_width
               NOTIFY map_width_changed)

    Q_PROPERTY(int map_height
               READ map_height
               NOTIFY map_height_changed)
public:

    enum Roles {
        TypeRole = Qt::UserRole + 1,
        OccupantRole,
        VisibleRole
    };

    GameManager(QObject * parent = nullptr);

    int rowCount(const QModelIndex & parent) const override;

    QVariant data(const QModelIndex & index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    // Callable by GUI to fetch cell.
    Q_INVOKABLE QVariantMap cell_at(int x, int y) const;

    // Callable from C++ code for changing the view.
    void update_view(PlayerView new_view);

    int map_width() const;

    int map_height() const;

signals:
    void view_changed();

    void map_width_changed();

    void map_height_changed();

private:
    PlayerView current_view_;
};
