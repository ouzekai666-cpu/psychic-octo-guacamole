// =============================================================================
// CANInterface.cpp — Stub implementation
// Replace with real CAN driver (e.g. QCanBus) when hardware is available.
// =============================================================================
#include "CANInterface.h"

CANInterface::CANInterface(QObject* parent) : SensorInterface(parent) {}

bool CANInterface::connect() {
    // TODO: Initialize QCanBusDevice for real hardware
    m_connected = true;
    emit connectionChanged(true);
    return true;
}

void CANInterface::disconnect() {
    m_connected = false;
    emit connectionChanged(false);
}
