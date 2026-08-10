// =============================================================================
// DeviceStateMachine.cpp
// =============================================================================
#include "DeviceStateMachine.h"
#include "../config/DeviceConfig.h"

DeviceStateMachine::DeviceStateMachine(QObject* parent)
    : QObject(parent), m_status(HeadStatus::NORMAL) {}

void DeviceStateMachine::setStatus(HeadStatus s) {
    if (m_status != s) {
        m_status = s;
        emit statusChanged(s);
    }
}

void DeviceStateMachine::evaluate(const DeviceState& state) {
    if (state.currentWater < DeviceConfig::MAX_WATER_CAPACITY * DeviceConfig::LOW_WATER_THRESHOLD) {
        setStatus(HeadStatus::STOPPED);
        return;
    }
    if (m_status == HeadStatus::DAMAGE && !m_damageConfirmed) {
        return; // DAMAGE persists until human confirmation
    }
    setStatus(state.headStatus);
}

void DeviceStateMachine::confirmDamageRecovery() {
    if (m_status == HeadStatus::DAMAGE) {
        m_damageConfirmed = true;
        setStatus(HeadStatus::NORMAL);
    }
}
