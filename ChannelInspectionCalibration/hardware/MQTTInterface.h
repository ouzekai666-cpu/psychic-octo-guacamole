// =============================================================================
// MQTTInterface.h — MQTT-based IIoT sensor (stub)
// =============================================================================
#pragma once
#include "SensorInterface.h"

class MQTTInterface : public SensorInterface {
    Q_OBJECT
public:
    explicit MQTTInterface(QObject* parent = nullptr);
    bool    connect() override;
    void    disconnect() override;
    bool    isConnected() const override { return m_connected; }
    double  getWaterLevel() override  { return 5000.0; }
    double  getPressure() override    { return 150.0; }
    double  getPosition() override    { return 0.0; }
    HeadStatus getHeadStatus() override { return HeadStatus::NORMAL; }
    double  getFlowRate() override    { return 120.0; }
    QString sensorName() const override { return QStringLiteral("MQTTInterface"); }
private:
    bool m_connected = false;
};
