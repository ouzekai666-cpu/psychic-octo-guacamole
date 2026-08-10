// =============================================================================
// AlarmManager.cpp — State-driven alarm evaluation, no UI logic
// Five alarm types: low-water, pressure, head-damage, device-stopped, comm-fault
// =============================================================================
#include "AlarmManager.h"
#include "../config/DeviceConfig.h"

AlarmManager::AlarmManager(QObject* parent) : QObject(parent) {}

void AlarmManager::evaluate(const DeviceState& state) {
    double water = state.currentWater;
    double press = state.waterPressure;
    HeadStatus hs = state.headStatus;

    // --- 1. Low water alarm (CRITICAL) ---
    if (water < DeviceConfig::MAX_WATER_CAPACITY * DeviceConfig::LOW_WATER_THRESHOLD) {
        triggerAlarm(QStringLiteral("ALM-WATER-001"), AlarmSeverity::CRITICAL,
            QStringLiteral("\u4F4E\u6C34\u4F4D"),          // 低水位
            QStringLiteral("\u4F4E\u6C34\u4F4D\u81EA\u52A8\u505C\u673A\u4FDD\u62A4 - \u5F53\u524D\u6C34\u4F4D: %1 L").arg(water, 0, 'f', 0),
            state);
    } else {
        clearAlarm(QStringLiteral("ALM-WATER-001"));
    }

    // --- 2. Pressure abnormal (WARNING) ---
    if (press < DeviceConfig::PRESSURE_NORMAL_MIN || press > DeviceConfig::PRESSURE_NORMAL_MAX) {
        if (press > 0.01) { // only if sensor is reading
            triggerAlarm(QStringLiteral("ALM-PRESS-001"), AlarmSeverity::WARNING,
                QStringLiteral("\u538B\u529B\u5F02\u5E38"),  // 压力异常
                QStringLiteral("\u6E05\u6D17\u538B\u529B\u5F02\u5E38 - \u5F53\u524D: %1 Bar").arg(press, 0, 'f', 0),
                state);
        }
    } else {
        clearAlarm(QStringLiteral("ALM-PRESS-001"));
    }

    // --- 3. Head damage alarm (CRITICAL) ---
    if (hs == HeadStatus::DAMAGE) {
        triggerAlarm(QStringLiteral("ALM-HEAD-001"), AlarmSeverity::CRITICAL,
            QStringLiteral("\u6E05\u6D17\u5934\u7834\u635F"),  // 清洗头破损
            QStringLiteral("\u6E05\u6D17\u5934\u7834\u635F\uFF0C\u8BF7\u7ACB\u5373\u505C\u6B62\u8BBE\u5907\u68C0\u67E5"),
            state);
    } else {
        clearAlarm(QStringLiteral("ALM-HEAD-001"));
    }

    // --- 4. Device stopped (ERROR) ---
    if (hs == HeadStatus::STOPPED) {
        triggerAlarm(QStringLiteral("ALM-STOP-001"), AlarmSeverity::ERROR,
            QStringLiteral("\u8BBE\u5907\u505C\u6B62"),        // 设备停止
            QStringLiteral("\u8BBE\u5907\u5DF2\u505C\u6B62\u8FD0\u884C"),
            state);
    } else {
        clearAlarm(QStringLiteral("ALM-STOP-001"));
    }

    // --- 5. Communication fault (reserved for future CAN/RS485/MQTT) ---
    // TODO: activate when hardware interface detects comm loss

    emit alarmsChanged();
}

void AlarmManager::triggerAlarm(const QString& code, AlarmSeverity sev, const QString& type, const QString& msg, const DeviceState& st) {
    if (m_activeAlarmMap.contains(code)) {
        int idx = m_activeAlarmMap[code];
        if (idx >= 0 && idx < m_alarmHistory.size() && m_alarmHistory[idx].status == AlarmStatus::ACTIVE) {
            return; // already active
        }
    }
    AlarmEvent ev;
    ev.alarmId      = m_nextId++;
    ev.alarmCode    = code;
    ev.level        = sev;
    ev.alarmType    = type;
    ev.alarmMessage = msg;
    ev.triggerTime  = QDateTime::currentDateTime();
    ev.deviceId     = QStringLiteral("HEAD-001");
    ev.channelId    = st.channelId;
    ev.status       = AlarmStatus::ACTIVE;
    m_alarmHistory.append(ev);
    m_activeAlarmMap[code] = m_alarmHistory.size() - 1;
    emit alarmTriggered(ev);
    emit alarmUpdated(ev);
}

void AlarmManager::clearAlarm(const QString& code) {
    if (!m_activeAlarmMap.contains(code)) return;
    int idx = m_activeAlarmMap[code];
    if (idx >= 0 && idx < m_alarmHistory.size()) {
        auto& ev = m_alarmHistory[idx];
        if (ev.status == AlarmStatus::ACTIVE || ev.status == AlarmStatus::ACKNOWLEDGED) {
            ev.status = AlarmStatus::CLEARED;
            ev.recoverTime = QDateTime::currentDateTime();
            emit alarmUpdated(ev);
        }
    }
    m_activeAlarmMap.remove(code);
}

QVector<AlarmEvent> AlarmManager::activeAlarms() const {
    QVector<AlarmEvent> result;
    for (const auto& ev : m_alarmHistory)
        if (ev.status == AlarmStatus::ACTIVE || ev.status == AlarmStatus::ACKNOWLEDGED)
            result.append(ev);
    return result;
}

void AlarmManager::acknowledgeAlarm(int alarmId) {
    for (auto& ev : m_alarmHistory) {
        if (ev.alarmId == alarmId && ev.status == AlarmStatus::ACTIVE) {
            ev.status = AlarmStatus::ACKNOWLEDGED;
            emit alarmUpdated(ev);
            emit alarmsChanged();
            return;
        }
    }
}

AlarmSeverity AlarmManager::highestSeverity() const {
    AlarmSeverity maxSev = AlarmSeverity::INFO;
    for (const auto& ev : m_alarmHistory) {
        if (ev.status == AlarmStatus::ACTIVE || ev.status == AlarmStatus::ACKNOWLEDGED) {
            if (static_cast<int>(ev.level) > static_cast<int>(maxSev))
                maxSev = ev.level;
        }
    }
    return maxSev;
}

AlarmEvent AlarmManager::latestBannerAlarm() const {
    AlarmEvent banner;
    AlarmSeverity maxSev = AlarmSeverity::INFO;
    for (const auto& ev : m_alarmHistory) {
        if ((ev.status == AlarmStatus::ACTIVE || ev.status == AlarmStatus::ACKNOWLEDGED) &&
            static_cast<int>(ev.level) > static_cast<int>(maxSev)) {
            maxSev = ev.level;
            banner = ev;
        }
    }
    return banner;
}
