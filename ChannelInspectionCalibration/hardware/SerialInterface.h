// =============================================================================
// SerialInterface.h — RS-232/485 serial sensor (stub)
// =============================================================================
#pragma once
#include "SensorInterface.h"

class SerialInterface : public SensorInterface {
    Q_OBJECT
public:
    explicit SerialInterface(QObject* parent = nullptr);
    bool    connect() override;
    void    disconnect() override;
    bool    isConnected() const override { return m_connected; }
    double  getWaterLevel() override  { return 5000.0; }
    double  getPressure() override    { return 150.0; }
    double  getPosition() override    { return 0.0; }
    HeadStatus getHeadStatus() override { return HeadStatus::NORMAL; }
    double  getFlowRate() override    { return 120.0; }
    QString sensorName() const override { return QStringLiteral("SerialInterface"); }
private:
    bool m_connected = false;
};
