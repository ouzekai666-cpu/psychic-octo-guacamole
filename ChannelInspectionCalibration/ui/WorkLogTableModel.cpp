// =============================================================================
// WorkLogTableModel.cpp
// =============================================================================
#include "WorkLogTableModel.h"

WorkLogTableModel::WorkLogTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

int WorkLogTableModel::rowCount(const QModelIndex&) const {
    return m_logs.size();
}

int WorkLogTableModel::columnCount(const QModelIndex&) const {
    return 16;
}

QVariant WorkLogTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_logs.size()) return {};
    const auto& e = m_logs[index.row()];
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0:  return e.taskId;
        case 1:  return e.operatorName;
        case 2:  return e.dateTime;
        case 3:  return e.channelId;
        case 4:  return e.channelLength;
        case 5:  return e.cleanedLength;
        case 6:  return e.startTime;
        case 7:  return e.endTime;
        case 8:  return e.totalWater;
        case 9:  return e.avgFlowRate;
        case 10: return e.maxPressure;
        case 11: return e.obstacleCount;
        case 12: return e.maxSeverity;
        case 13: return e.anomalyRecord;
        case 14: return e.result;
        case 15: return e.deviceStatus;
        }
    }
    if (role == Qt::TextAlignmentRole)
        return static_cast<int>(Qt::AlignCenter);
    return {};
}

QVariant WorkLogTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        const auto h = WorkLogEntry::headers();
        if (section < h.size()) return h[section];
    }
    return {};
}

void WorkLogTableModel::setLogs(const QVector<WorkLogEntry>& logs) {
    beginResetModel();
    m_logs = logs;
    endResetModel();
}

void WorkLogTableModel::appendLog(const WorkLogEntry& entry) {
    beginInsertRows(QModelIndex(), m_logs.size(), m_logs.size());
    m_logs.append(entry);
    endInsertRows();
}
