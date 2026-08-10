// =============================================================================
// ObstacleTableModel.cpp
// =============================================================================
#include "ObstacleTableModel.h"
#include "../config/DeviceConfig.h"
#include <algorithm>

ObstacleTableModel::ObstacleTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    m_types     = DeviceConfig::OBSTACLE_TYPES;
    m_severities = DeviceConfig::SEVERITY_LEVELS;
    m_typeDelegate     = new ComboBoxDelegate(m_types, this);
    m_severityDelegate = new ComboBoxDelegate(m_severities, this);
}

int ObstacleTableModel::rowCount(const QModelIndex&) const {
    return m_obstacles.size();
}

int ObstacleTableModel::columnCount(const QModelIndex&) const {
    return ColCount;
}

QVariant ObstacleTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_obstacles.size()) return {};
    const auto& o = m_obstacles[index.row()];
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case ColId:       return o.id;
        case ColType:     return o.type;
        case ColStart:    return o.start_m;
        case ColEnd:      return o.end_m;
        case ColSeverity: return o.severity;
        case ColNotes:    return o.notes;
        case ColLength:   return o.length();
        }
    }
    if (role == Qt::TextAlignmentRole)
        return static_cast<int>(Qt::AlignCenter);
    return {};
}

QVariant ObstacleTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static const QStringList headers = {
            QStringLiteral("\u5E8F\u53F7"),
            QStringLiteral("\u7C7B\u578B"),
            QStringLiteral("\u8D77(m)"),
            QStringLiteral("\u6B62(m)"),
            QStringLiteral("\u7B49\u7EA7"),
            QStringLiteral("\u5907\u6CE8"),
            QStringLiteral("\u957F(m)")
        };
        if (section < headers.size()) return headers[section];
    }
    return {};
}

Qt::ItemFlags ObstacleTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    if (index.column() == ColId || index.column() == ColLength)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

bool ObstacleTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || role != Qt::EditRole || index.row() >= m_obstacles.size())
        return false;
    auto& o = m_obstacles[index.row()];
    switch (index.column()) {
    case ColType:     o.type     = value.toString(); break;
    case ColStart:    o.start_m  = value.toDouble(); break;
    case ColEnd:      o.end_m    = value.toDouble(); break;
    case ColSeverity: o.severity = value.toString(); break;
    case ColNotes:    o.notes    = value.toString(); break;
    default: return false;
    }
    emit dataChanged(index, index);
    emit dataModified();
    return true;
}

void ObstacleTableModel::setObstacles(const QVector<Obstacle>& obs) {
    beginResetModel();
    m_obstacles = obs;
    endResetModel();
}

void ObstacleTableModel::addRow() {
    int newId = 1;
    for (const auto& o : m_obstacles)
        newId = std::max(newId, o.id + 1);
    beginInsertRows(QModelIndex(), m_obstacles.size(), m_obstacles.size());
    Obstacle o;
    o.id = newId;
    o.type = m_types.value(0);
    o.start_m = 0.0;
    o.end_m = 1.0;
    o.severity = m_severities.value(0);
    m_obstacles.append(o);
    endInsertRows();
    emit dataModified();
}

void ObstacleTableModel::removeRow(int row) {
    if (row < 0 || row >= m_obstacles.size()) return;
    beginRemoveRows(QModelIndex(), row, row);
    m_obstacles.removeAt(row);
    for (int i = 0; i < m_obstacles.size(); ++i)
        m_obstacles[i].id = i + 1;
    endRemoveRows();
    emit dataModified();
}

const Obstacle& ObstacleTableModel::obstacleAt(int row) const {
    if (row >= 0 && row < m_obstacles.size())
        return m_obstacles[row];
    static const Obstacle s_nullObs;
    return s_nullObs;
}
