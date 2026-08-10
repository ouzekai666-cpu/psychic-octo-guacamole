// =============================================================================
// AlarmEvent.h — Industrial alarm event model
// All alarms are state-driven; UI only displays.
// =============================================================================
#pragma once
#include <QString>
#include <QDateTime>
#include <QColor>
#include <QVector>
#include <QMap>

enum class AlarmSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

inline QString alarmSeverityString(AlarmSeverity s) {
    switch (s) {
    case AlarmSeverity::INFO:     return QStringLiteral("INFO");
    case AlarmSeverity::WARNING:  return QStringLiteral("WARNING");
    case AlarmSeverity::ERROR:    return QStringLiteral("ERROR");
    case AlarmSeverity::CRITICAL: return QStringLiteral("CRITICAL");
    }
    return {};
}

inline QColor alarmSeverityColor(AlarmSeverity s) {
    switch (s) {
    case AlarmSeverity::INFO:     return QColor("#3B82F6");
    case AlarmSeverity::WARNING:  return QColor("#F59E0B");
    case AlarmSeverity::ERROR:    return QColor("#F97316");
    case AlarmSeverity::CRITICAL: return QColor("#EF4444");
    }
    return QColor("#9CA3AF");
}

inline QString alarmSeverityStyle(AlarmSeverity s) {
    switch (s) {
    case AlarmSeverity::INFO:     return QStringLiteral("background-color:#3B82F6;color:white;font-weight:bold;");
    case AlarmSeverity::WARNING:  return QStringLiteral("background-color:#F59E0B;color:white;font-weight:bold;");
    case AlarmSeverity::ERROR:    return QStringLiteral("background-color:#F97316;color:white;font-weight:bold;");
    case AlarmSeverity::CRITICAL: return QStringLiteral("background-color:#EF4444;color:white;font-weight:bold;");
    }
    return QStringLiteral("background-color:#9CA3AF;color:white;");
}

enum class AlarmStatus {
    ACTIVE,
    ACKNOWLEDGED,
    CLEARED
};

inline QString alarmStatusString(AlarmStatus s) {
    switch (s) {
    case AlarmStatus::ACTIVE:       return QStringLiteral("ACTIVE");
    case AlarmStatus::ACKNOWLEDGED: return QStringLiteral("ACKNOWLEDGED");
    case AlarmStatus::CLEARED:      return QStringLiteral("CLEARED");
    }
    return {};
}

struct AlarmEvent {
    int           alarmId      = 0;
    QString       alarmCode;
    AlarmSeverity level         = AlarmSeverity::INFO;
    QString       alarmType;
    QString       alarmMessage;
    QDateTime     triggerTime;
    QDateTime     recoverTime;
    QString       deviceId;
    QString       channelId;
    AlarmStatus   status       = AlarmStatus::ACTIVE;

    // For backward compatibility with banner display
    QString bannerStyle() const { return alarmSeverityStyle(level); }
    QString bannerText() const {
        if (alarmMessage.isEmpty())
            return QStringLiteral("\u7BA1\u9053\u7545\u901A\uFF0C\u65E0\u5835\u585E");
        return alarmMessage;
    }
    bool isActive() const { return status == AlarmStatus::ACTIVE; }
};

inline bool operator==(const AlarmEvent& a, const AlarmEvent& b) {
    return a.alarmCode == b.alarmCode && a.level == b.level && a.status == b.status;
}
inline bool operator!=(const AlarmEvent& a, const AlarmEvent& b) { return !(a == b); }
