// =============================================================================
// SensorInterface.h — Abstract sensor interface
// Replace SimulationSensor with CAN/Serial/MQTT without changing business logic.
// =============================================================================
#pragma once

#include <QObject>
#include "../models/DeviceState.h"

class SensorInterface : public QObject {
    Q_OBJECT
public:
    explicit SensorInterface(QObject* parent = nullptr) : QObject(parent) {}
    ~SensorInterface() override = default;

    virtual bool    connect()          = 0;
    virtual void    disconnect()       = 0;
    virtual bool    isConnected() const = 0;

    virtual double  getWaterLevel()    = 0;  // L
    virtual double  getPressure()      = 0;  // Bar
    virtual double  getPosition()      = 0;  // m
    virtual HeadStatus getHeadStatus() = 0;
    virtual double  getFlowRate()      = 0;  // L/min
    virtual QString sensorName() const = 0;

signals:
    void dataUpdated();
    void connectionChanged(bool connected);
};
