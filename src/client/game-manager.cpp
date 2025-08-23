#include "game-manager.h"

GameManager::GameManager(QObject * parent)
:QAbstractListModel(parent)
{
}

int GameManager::rowCount(const QModelIndex & parent = QModelIndex()) const
{
    Q_UNUSED(parent)
    return current_view_.width() * current_view_.height();
}

QVariant GameManager::data(const QModelIndex & index, int role) const
{
    if (!index.isValid()
        || index.row() >= static_cast<int>(current_view_.width()
                                           * current_view_.height())
        || index.row() < 0)
    {
        return {};
    }

    const GridCell & cell = current_view_.map_view[index.row()];

    switch (role)
    {
        case TypeRole:
            return static_cast<int>(cell.type_);
        case OccupantRole:
            return static_cast<int>(cell.occupant_);
        case VisibleRole:
            return cell.visible_;
        default:
            return {};
    }
}

QHash<int, QByteArray> GameManager::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TypeRole] = "type";
    roles[OccupantRole] = "occupant";
    roles[VisibleRole] = "visible";
    return roles;
}

Q_INVOKABLE QVariantMap GameManager::cell_at(int x, int y) const
{
    QVariantMap m;
    if (x < 0
        || y < 0
        || x >= current_view_.width()
        || y >= current_view_.height())
    {
        return m;
    }

    size_t index = current_view_.map_view.idx(x, y);
    const GridCell & cell = current_view_.map_view[index];

    m["type"] = static_cast<int>(cell.type_);
    m["occupant"] = static_cast<int>(cell.occupant_);
    m["visible"] = cell.visible_;
    return m;
}

void GameManager::update_view(PlayerView new_view)
{
    bool width_changed = (new_view.width() != current_view_.width());
    bool height_changed = (new_view.height() != current_view_.height());

    beginResetModel();
    current_view_ = new_view;
    endResetModel();

    if (width_changed)
    {
        emit map_width_changed();
    }
    if (height_changed)
    {
        emit map_height_changed();
    }

    emit view_changed();
}

int GameManager::map_width() const
{
    return static_cast<int>(current_view_.width());
}

int GameManager::map_height() const
{
    return static_cast<int>(current_view_.height());
}
