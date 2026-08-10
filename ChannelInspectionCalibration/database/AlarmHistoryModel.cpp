// =============================================================================
// AlarmHistoryModel.cpp
// =============================================================================
#include "AlarmHistoryModel.h"
#include <QColor>
#include <QFont>

AlarmHistoryModel::AlarmHistoryModel(QObject* parent) : QAbstractTableModel(parent) {}

int AlarmHistoryModel::rowCount(const QModelIndex&) const { return m_alarms.size(); }
int AlarmHistoryModel::columnCount(const QModelIndex&) const { return ColCount; }

QVariant AlarmHistoryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_alarms.size()) return {};
    const auto& ev = m_alarms[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColLevel:   return QStringLiteral("\u25CF %1").arg(alarmSeverityString(ev.level));
        case ColTime:    return ev.triggerTime.toString("HH:mm:ss");
        case ColType:    return ev.alarmType;
        case ColMessage: return ev.alarmMessage;
        case ColStatus:  return alarmStatusString(ev.status);
        }
    }

    if (role == Qt::ForegroundRole && index.column() == ColLevel) {
        return alarmSeverityColor(ev.level);
    }

    // Left color bar for severity column
    if (role == Qt::BackgroundRole && index.column() == ColLevel) {
        QColor c = alarmSeverityColor(ev.level);
        c.setAlpha(30);
        return c;
    }

    if (role == Qt::FontRole && index.column() == ColLevel) {
        QFont f; f.setBold(true); return f;
    }

    if (role == Qt::TextAlignmentRole)
        return static_cast<int>(Qt::AlignCenter);

    return {};
}

QVariant AlarmHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static const QStringList headers = {
            QStringLiteral("\u7B49\u7EA7"),
            QStringLiteral("\u65F6\u95F4"),
            QStringLiteral("\u7C7B\u578B"),
            QStringLiteral("\u63CF\u8FF0"),
            QStringLiteral("\u72B6\u6001")
        };
        if (section < headers.size()) return headers[section];
    }
    return {};
}

void AlarmHistoryModel::setAlarms(const QVector<AlarmEvent>& alarms) {
    beginResetModel();
    m_alarms = alarms;
    endResetModel();
}

void AlarmHistoryModel::appendAlarm(const AlarmEvent& ev) {
    beginInsertRows(QModelIndex(), m_alarms.size(), m_alarms.size());
    m_alarms.append(ev);
    endInsertRows();
}

void AlarmHistoryModel::updateAlarm(const AlarmEvent& ev) {
    for (int i = 0; i < m_alarms.size(); ++i) {
        if (m_alarms[i].alarmId == ev.alarmId) {
            m_alarms[i] = ev;
            emit dataChanged(index(i, 0), index(i, ColCount - 1));
            return;
        }
    }
}

void AlarmHistoryModel::clear() {
    beginResetModel();
    m_alarms.clear();
    endResetModel();
}
