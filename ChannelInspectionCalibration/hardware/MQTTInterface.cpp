// =============================================================================
// MQTTInterface.cpp — Stub. Replace with QMqttClient when IIoT is deployed.
// =============================================================================
#include "MQTTInterface.h"

MQTTInterface::MQTTInterface(QObject* parent) : SensorInterface(parent) {}

bool MQTTInterface::connect() {
    // TODO: QMqttClient::connectToHost(broker, port)
    m_connected = true;
    emit connectionChanged(true);
    return true;
}

void MQTTInterface::disconnect() {
    m_connected = false;
    emit connectionChanged(false);
}
