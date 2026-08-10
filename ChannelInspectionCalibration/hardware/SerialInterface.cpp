// =============================================================================
// SerialInterface.cpp — Stub. Replace with QSerialPort when hardware is ready.
// =============================================================================
#include "SerialInterface.h"

SerialInterface::SerialInterface(QObject* parent) : SensorInterface(parent) {}

bool SerialInterface::connect() {
    // TODO: Open QSerialPort with baud/parity/stop bits
    m_connected = true;
    emit connectionChanged(true);
    return true;
}

void SerialInterface::disconnect() {
    m_connected = false;
    emit connectionChanged(false);
}
