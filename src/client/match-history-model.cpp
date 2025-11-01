#include "match-history-model.h"

MatchHistoryModel::MatchHistoryModel(QObject* parent)
:QAbstractListModel(parent),
history_rows_(NUMBER_OF_MODES)
{
}

int MatchHistoryModel::rowCount(const QModelIndex & parent = QModelIndex()) const
{
    if (current_history_mode_ < 0
        || current_history_mode_ >= static_cast<int>(history_rows_.size()))
    {
        return 0;
    }

    return static_cast<int>(history_rows_[current_history_mode_].size());
}

QVariant MatchHistoryModel::data(const QModelIndex & index, int role) const
{
    if (current_history_mode_ < 0
        || current_history_mode_ >= static_cast<int>(history_rows_.size()))
    {
        return {};
    }

    if (!index.isValid()
        || index.row() >= static_cast<int>(history_rows_[current_history_mode_].size())
        || index.row() < 0)
    {
        return {};
    }

    const auto & mode_history = history_rows_[current_history_mode_];

    const MatchResultRow & row = mode_history[index.row()];

    switch (role)
    {
        case MatchIDRole:
            return QVariant::fromValue(static_cast<qlonglong>(row.match_id));
        case FinishedAtRole:
        {
            // Convert to QDateTime and send.
            QDateTime date_time = QDateTime::fromSecsSinceEpoch(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        row.finished_at.time_since_epoch()
                    ).count(),
                    QTimeZone::systemTimeZone()
                );

            return date_time.toString("yyyy-MM-dd hh:mm:ss");
        }
        case PlacementRole:
            return row.placement;
        case EloRole:
            return row.elo_change;
        default:
            return {};
    }
}

QHash<int, QByteArray> MatchHistoryModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[MatchIDRole] = "MatchID";
    roles[FinishedAtRole] = "finished_at";
    roles[PlacementRole] = "placement";
    roles[EloRole] = "elo_change";
    return roles;
}

int MatchHistoryModel::current_history_mode() const
{
    return current_history_mode_;
}

Q_INVOKABLE void MatchHistoryModel::set_current_history_mode(int mode)
{
    beginResetModel();

    current_history_mode_ = mode;

    endResetModel();

    emit current_history_mode_changed(mode);
}

void MatchHistoryModel::set_history_row(MatchResultList results)
{
    // Prevent accessing rows that don't exist.
    if (static_cast<uint8_t>(results.mode) >= static_cast<uint8_t>(GameMode::NO_MODE))
    {
        return;
    }

    beginResetModel();

    // Set this match result list to the new results.
    history_rows_[static_cast<uint8_t>(results.mode)] = results.match_results;

    endResetModel();
}
