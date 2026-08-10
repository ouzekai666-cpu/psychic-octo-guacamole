// =============================================================================
// DeviceStateMachine.h — Head status state machine
// States: NORMAL -> WARNING -> ERROR -> STOPPED
// =============================================================================
#pragma once

#include <QObject>
#include "../models/DeviceState.h"

class DeviceStateMachine : public QObject {
    Q_OBJECT
public:
    explicit DeviceStateMachine(QObject* parent = nullptr);

    HeadStatus currentStatus() const { return m_status; }
    void setStatus(HeadStatus s);

    // Evaluate transition based on current DeviceState
    void evaluate(const DeviceState& state);
    bool isDamageConfirmed() const { return m_damageConfirmed; }
    void confirmDamageRecovery();

signals:
    void statusChanged(HeadStatus newStatus);

private:
    HeadStatus m_status = HeadStatus::NORMAL;
    bool m_damageConfirmed = false;
};
