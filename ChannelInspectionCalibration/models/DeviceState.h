// =============================================================================
// DeviceState.h — Central device state shared across all layers
// =============================================================================
#pragma once
#include <QString>
#include <QDateTime>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>

struct Obstacle {
    int     id       = 0;
    QString type;
    double  start_m  = 0.0;
    double  end_m    = 0.0;
    QString severity;
    QString notes;
    double length() const { return end_m - start_m; }
    QJsonObject toJson() const {
        return {{"id", id}, {"type", type}, {"start_m", start_m},
                {"end_m", end_m}, {"severity", severity}, {"notes", notes}};
    }
    static Obstacle fromJson(const QJsonObject& o) {
        Obstacle ob;
        ob.id = o["id"].toInt(); ob.type = o["type"].toString();
        ob.start_m = o["start_m"].toDouble(); ob.end_m = o["end_m"].toDouble();
        ob.severity = o["severity"].toString(); ob.notes = o["notes"].toString();
        return ob;
    }
};

enum class HeadStatus { NORMAL, WARNING, DAMAGE, STOPPED };

inline QString headStatusString(HeadStatus s) {
    switch (s) {
    case HeadStatus::NORMAL:  return QStringLiteral("\u6B63\u5E38");
    case HeadStatus::WARNING: return QStringLiteral("\u8B66\u544A");
    case HeadStatus::DAMAGE:  return QStringLiteral("\u7834\u635F");
    case HeadStatus::STOPPED: return QStringLiteral("\u5DF2\u505C\u6B62");
    }
    return QStringLiteral("\u672A\u77E5");
}

class DeviceState {
public:
    QString channelId     = QStringLiteral("A1\u8DEF\u6BB5\u4E3B\u6C9F\u6E20");
    double  channelLength = 100.0;
    QString operatorName  = QStringLiteral("\u5F20\u5DE5\u7A0B\u5E08");
    QDate   inspectDate   = QDate::currentDate();
    QVector<Obstacle> obstacles;
    double  currentPosition = 0.0;
    double  currentWater    = 5000.0;
    double  waterPressure   = 150.0;
    HeadStatus headStatus   = HeadStatus::NORMAL;
    double  flowRate        = 120.0;
    double  totalDistance   = 0.0;
    double  totalWaterUsed  = 0.0;
    double  taskProgress    = 0.0;
    QDateTime startTime;
    QDateTime updateTime;
    QString alarmStatus;
    QString alarmLevel;

    double waterPercent() const {
        return (5000.0 > 0) ? (currentWater / 5000.0) : 0.0;
    }
    int maxSeverityIndex() const {
        static constexpr int SEV_LIGHT = 1, SEV_MEDIUM = 2, SEV_HEAVY = 3;
        int maxIdx = 0;
        for (const auto& o : obstacles) {
            if (o.severity == QStringLiteral("\u91CD\u5EA6")) maxIdx = std::max(maxIdx, SEV_HEAVY);
            else if (o.severity == QStringLiteral("\u4E2D\u5EA6")) maxIdx = std::max(maxIdx, SEV_MEDIUM);
            else if (o.severity == QStringLiteral("\u8F7B\u5EA6")) maxIdx = std::max(maxIdx, SEV_LIGHT);
        }
        return maxIdx;
    }
    double completionPercent() const {
        if (channelLength <= 0.0) return 0.0;
        return std::min(100.0, (currentPosition / channelLength) * 100.0);
    }
    double remainingTime() const {
        if (flowRate <= 0.0) return 0.0;
        return currentWater / flowRate;
    }
    double estimatedRemainingDistance() const {
        if (flowRate <= 0.0) return 0.0;
        return (currentWater / flowRate) * 1.0;
    }
    QJsonObject infoJson() const {
        return {{"channel_id", channelId},
                {"total_length", channelLength},
                {"operator", operatorName},
                {"inspect_date", inspectDate.toString("yyyy-MM-dd")}};
    }
    QJsonArray obstaclesJson() const {
        QJsonArray arr;
        for (const auto& o : obstacles) arr.append(o.toJson());
        return arr;
    }
    mutable QMutex mutex;
    void lock()   const { mutex.lock(); }
    void unlock() const { mutex.unlock(); }
};
