// =============================================================================
// AlarmHistoryModel.h — QAbstractTableModel for alarm history display
// =============================================================================
#pragma once
#include <QAbstractTableModel>
#include <QVector>
#include "../models/AlarmEvent.h"

class AlarmHistoryModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { ColLevel = 0, ColTime, ColType, ColMessage, ColStatus, ColCount };

    explicit AlarmHistoryModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void setAlarms(const QVector<AlarmEvent>& alarms);
    void appendAlarm(const AlarmEvent& ev);
    void updateAlarm(const AlarmEvent& ev);
    void clear();

private:
    QVector<AlarmEvent> m_alarms;
};
