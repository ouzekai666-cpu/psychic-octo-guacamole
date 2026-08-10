// =============================================================================
// WorkLogTableModel.h — QAbstractTableModel for work log display
// =============================================================================
#pragma once
#include <QAbstractTableModel>
#include <QVector>
#include "../models/WorkLog.h"

class WorkLogTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit WorkLogTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setLogs(const QVector<WorkLogEntry>& logs);
    void appendLog(const WorkLogEntry& entry);

private:
    QVector<WorkLogEntry> m_logs;
};
