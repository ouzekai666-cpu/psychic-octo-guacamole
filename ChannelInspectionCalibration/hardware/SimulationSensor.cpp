// =============================================================================
// SimulationSensor.cpp
// =============================================================================
#include "SimulationSensor.h"
#include "../config/DeviceConfig.h"
#include <cmath>

SimulationSensor::SimulationSensor(double channelLength, QObject* parent)
    : SensorInterface(parent)
    , m_channelLength(channelLength)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(DeviceConfig::SIM_TICK_MS);
    QObject::connect(m_timer, &QTimer::timeout, this, &SimulationSensor::onTick);
}

bool SimulationSensor::connect() {
    m_connected = true;
    m_waterLevel = DeviceConfig::MAX_WATER_CAPACITY;
    m_pressure   = 150.0;
    m_position   = 0.0;
    m_headStatus = HeadStatus::NORMAL;
    m_tickCount  = 0;
    m_normalTicks = 0;
    m_normalTarget = QRandomGenerator::global()->bounded(
        DeviceConfig::STATE_NORMAL_MIN_S * 10,
        DeviceConfig::STATE_NORMAL_MAX_S * 10
    );
    m_abnormalTicks = 0;
    m_inAnomaly    = false;
    m_timer->start();
    emit connectionChanged(true);
    return true;
}

void SimulationSensor::disconnect() {
    m_timer->stop();
    m_connected = false;
    emit connectionChanged(false);
}

double SimulationSensor::getWaterLevel()  { return m_waterLevel; }
double SimulationSensor::getPressure()    { return m_pressure; }
double SimulationSensor::getPosition()    { return m_position; }
HeadStatus SimulationSensor::getHeadStatus() { return m_headStatus; }

void SimulationSensor::onTick() {
    if (!m_connected) return;
    m_tickCount++;

    // --- Water consumption ---
    double consumption = DeviceConfig::DEFAULT_FLOW_RATE * (DeviceConfig::SIM_TICK_MS / 60000.0);
    m_waterLevel = std::max(0.0, m_waterLevel - consumption);

    // --- Position advance ---
    double advance = DeviceConfig::CLEANING_SPEED * (DeviceConfig::SIM_TICK_MS / 60000.0);
    if (m_headStatus != HeadStatus::STOPPED && m_waterLevel > 0.0) {
        m_position = std::min(m_channelLength, m_position + advance);
    }

    // --- Head state machine ---
    if (m_headStatus == HeadStatus::STOPPED) {
        // Already stopped; do nothing further
        emit dataUpdated();
        return;
    }

    // Low water protection: immediate STOPPED
    if (m_waterLevel < DeviceConfig::MAX_WATER_CAPACITY * DeviceConfig::LOW_WATER_THRESHOLD) {
        m_headStatus = HeadStatus::STOPPED;
        m_pressure   = 0.0;
        emit dataUpdated();
        return;
    }

    if (!m_inAnomaly) {
        m_normalTicks++;
        // Normal: pressure in range
        m_pressure = 150.0 + QRandomGenerator::global()->bounded(-10, 10);
        m_headStatus = HeadStatus::NORMAL;

        if (m_normalTicks >= m_normalTarget) {
            // Randomly enter anomaly
            m_inAnomaly = true;
            m_abnormalTicks = 0;
            m_headStatus = HeadStatus::WARNING;
            m_pressure = (QRandomGenerator::global()->bounded(2) == 0)
                ? 180.0 + QRandomGenerator::global()->bounded(5, 30)   // high
                : 110.0 - QRandomGenerator::global()->bounded(5, 30);  // low
        }
    } else {
        m_abnormalTicks++;
        // Anomaly: fluctuate, then recover
        if (m_abnormalTicks > 50) {
            // Escalate to ERROR
            m_headStatus = HeadStatus::DAMAGE;
            m_pressure = (QRandomGenerator::global()->bounded(2) == 0) ? 210.0 : 50.0;
        }
        if (m_abnormalTicks > 80) {
            // Recover
            m_inAnomaly = false;
            m_normalTicks = 0;
            m_normalTarget = QRandomGenerator::global()->bounded(
                DeviceConfig::STATE_NORMAL_MIN_S * 10,
                DeviceConfig::STATE_NORMAL_MAX_S * 10
            );
            m_headStatus = HeadStatus::NORMAL;
            m_pressure = 150.0;
        }
    }

    emit dataUpdated();
}
