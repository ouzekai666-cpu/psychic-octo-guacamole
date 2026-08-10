// =============================================================================
// SimulationSensor.h — Simulated sensor data for development/demo
// =============================================================================
#pragma once

#include "SensorInterface.h"
#include <QTimer>
#include <QRandomGenerator>

class SimulationSensor : public SensorInterface {
    Q_OBJECT
public:
    explicit SimulationSensor(double channelLength, QObject* parent = nullptr);
    ~SimulationSensor() override = default;

    bool    connect() override;
    void    disconnect() override;
    bool    isConnected() const override { return m_connected; }

    double  getWaterLevel() override;
    double  getPressure() override;
    double  getPosition() override;
    HeadStatus getHeadStatus() override;
    double  getFlowRate() override { return 120.0; }
    QString sensorName() const override { return QStringLiteral("SimulationSensor"); }

    void setChannelLength(double len) { m_channelLength = len; }

private slots:
    void onTick();

private:
    QTimer* m_timer     = nullptr;
    bool    m_connected = false;
    double  m_channelLength;
    double  m_waterLevel   = 5000.0;
    double  m_pressure     = 150.0;
    double  m_position     = 0.0;
    HeadStatus m_headStatus = HeadStatus::NORMAL;
    int     m_tickCount    = 0;
    int     m_normalTicks  = 0;
    int     m_normalTarget = 100; // ticks to stay normal before potential anomaly
    int     m_abnormalTicks= 0;
    bool    m_inAnomaly    = false;
};
