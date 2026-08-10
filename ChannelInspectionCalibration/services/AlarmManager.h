// =============================================================================
// AlarmManager.h — Industrial alarm center: state-driven, zero UI logic
// Data flow: DeviceState -> AlarmManager -> AlarmEvent -> UI display
// =============================================================================
#pragma once
#include <QObject>
#include <QVector>
#include <QMap>
#include "../models/DeviceState.h"
#include "../models/AlarmEvent.h"

class AlarmManager : public QObject {
    Q_OBJECT
public:
    explicit AlarmManager(QObject* parent = nullptr);
    void evaluate(const DeviceState& state);
    QVector<AlarmEvent> activeAlarms() const;
    QVector<AlarmEvent> allAlarms() const { return m_alarmHistory; }
    void acknowledgeAlarm(int alarmId);
    void clearAlarm(const QString& code);
    AlarmSeverity highestSeverity() const;
    AlarmEvent   latestBannerAlarm() const;

signals:
    void alarmTriggered(const AlarmEvent& event);
    void alarmUpdated(const AlarmEvent& event);
    void alarmsChanged();

private:
    int m_nextId = 1;
    QVector<AlarmEvent> m_alarmHistory;
    QMap<QString, int> m_activeAlarmMap;
    void triggerAlarm(const QString& code, AlarmSeverity sev, const QString& type, const QString& msg, const DeviceState& st);
};
