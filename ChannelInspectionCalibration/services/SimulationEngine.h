// =============================================================================
// SimulationEngine.h — Central orchestration engine
// Data flow: Sensor -> DeviceState -> AlarmManager -> UI
// =============================================================================
#pragma once

#include <QObject>
#include <QTimer>
#include "../models/DeviceState.h"
#include "../models/AlarmEvent.h"
#include "../models/WorkLog.h"
#include "../hardware/SensorInterface.h"
#include "DeviceStateMachine.h"
#include "AlarmManager.h"

class SimulationEngine : public QObject {
    Q_OBJECT
public:
    explicit SimulationEngine(QObject* parent = nullptr);
    ~SimulationEngine() override;

    void setSensor(SensorInterface* sensor);
    void start();
    void stop();
    void reset();

    DeviceState&       deviceState()       { return m_state; }
    AlarmEvent         currentAlarm() const { return m_alarmManager->latestBannerAlarm(); }
    AlarmManager*      alarmManager()       { return m_alarmManager; }
    void               recoverFromDamage();
    QVector<WorkLogEntry>& workLog()       { return m_workLog; }

    // Obstacle management
    void setObstacles(const QVector<Obstacle>& obs);
    void updateObstacle(int index, const Obstacle& obs);
    void removeObstacle(int index);
    void addObstacle(const Obstacle& obs);

    // Channel info
    void setChannelInfo(const QString& id, double length, const QString& op);

    // Business computations
    struct BusinessReport {
        int    obstacleCount   = 0;
        double blockageRate    = 0.0;
        int    heavyCount      = 0;
        double totalWater      = 0.0;
        double totalLabor      = 0.0;
    };
    BusinessReport computeReport() const;

    // Water per obstacle
    static double waterPerObstacle(const Obstacle& o);
    static double laborPerObstacle(const Obstacle& o);
    static double totalWaterFor(const QVector<Obstacle>& obs);
    static double totalLaborFor(const QVector<Obstacle>& obs);

    // Work order text
    QString generateWorkOrder(const QVector<Obstacle>& obs) const;

    // Log a work entry
    void logWorkEntry(const WorkLogEntry& entry);

signals:
    void stateUpdated();
    void alarmUpdated(const AlarmEvent& event);
    void simulationStarted();
    void simulationStopped();

private slots:
    void onSensorData();

private:
    SensorInterface*    m_sensor = nullptr;
    DeviceStateMachine* m_stateMachine;
    AlarmManager*       m_alarmManager;
    DeviceState         m_state;
    QVector<WorkLogEntry> m_workLog;
    QTimer*             m_stateTimer;
    int                 m_taskCounter = 0;

    void syncStateFromSensor();
    void evaluateAlarms();
};
