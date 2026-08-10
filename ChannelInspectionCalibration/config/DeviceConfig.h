// =============================================================================
// DeviceConfig.h 鈥?Global device configuration constants
// 娓犳矡妫€娴嬫爣瀹? Industrial Desktop Application
// =============================================================================
#pragma once

#include <QString>
#include <QStringList>
#include <QMap>
#include <QColor>
#include <vector>

namespace DeviceConfig {

// --- Digital Twin Visualization ---
namespace DigitalTwin {
    // --- Trajectory ---
    constexpr double TRAJECTORY_LINE_WIDTH      = 10.0;
    constexpr double TRAJECTORY_OUTLINE_WIDTH   = 2.0;
    constexpr double TRAJECTORY_DASH_ON         = 20.0;
    constexpr double TRAJECTORY_DASH_OFF        = 10.0;
    constexpr double TRAJECTORY_BG_OPACITY      = 0.12;
    inline QColor   TRAJECTORY_COLOR()     { return QColor("#0284C7"); }
    inline QColor   TRAJECTORY_OUTLINE_COLOR()  { return QColor("#0369A1"); }
    inline QColor   TRAJECTORY_BG_COLOR()       { return QColor("#0284C7"); }
    // --- Cleaning Head ---
    constexpr int   CLEANING_HEAD_ICON_SIZE = 96;
    inline QColor   HEAD_LABEL_COLOR()     { return QColor("#0284C7"); }
    inline QColor   HEAD_LABEL_BG()        { return QColor("#FFFFFF"); }
    // --- Layout offsets ---
    constexpr double CHANNEL_VIEW_Y_RANGE    = 1.0;
    constexpr double TRAJECTORY_Y_OFFSET     = 1.0;
    constexpr double AXIS_Y_OFFSET           = 0.8;
    constexpr double HEAD_LABEL_Y_OFFSET     = -30.0; // px offset for head label above icon
    // --- Hard pixel spacing (scene coordinates, survives fitInView scaling) ---
    constexpr double TRAJECTORY_XAXIS_GAP_PX = 55.0;
    constexpr double CHART_LEFT_MARGIN_PX    = 100.0;
    constexpr double TRAJECTORY_LEGEND_OFFSET_X = -80.0;
    // --- Z-order ---
    constexpr int   Z_DITCH     = 0;
    constexpr int   Z_AXIS      = 5;
    constexpr int   Z_OBSTACLE  = 10;
    constexpr int   Z_LABEL     = 20;
    constexpr int   Z_TRAJECTORY= 50;
    constexpr int   Z_HEAD      = 100;
} // namespace DigitalTwin

// --- Water system ---
constexpr double MAX_WATER_CAPACITY   = 5000.0;   // L
constexpr double DEFAULT_FLOW_RATE    = 120.0;    // L/min
constexpr double LOW_WATER_THRESHOLD  = 0.05;     // 5% of capacity triggers STOPPED
constexpr double CLEANING_SPEED       = 1.0;      // m/min at default flow

// --- Pressure ---
constexpr double PRESSURE_NORMAL_MIN  = 140.0;    // Bar
constexpr double PRESSURE_NORMAL_MAX  = 160.0;    // Bar

// --- Simulation ---
constexpr int    SIM_TICK_MS          = 100;      // 100 ms tick
constexpr int    STATE_NORMAL_MIN_S   = 5;
constexpr int    STATE_NORMAL_MAX_S   = 30;

// --- Obstacle types & severity ---
inline const QStringList OBSTACLE_TYPES = {
    QStringLiteral("\u6CE5\u6C99"),   // 娉ユ矙
    QStringLiteral("\u6CE5\u5757"),   // 娉ュ潡
    QStringLiteral("\u77F3\u5934"),   // 鐭冲ご
    QStringLiteral("\u6811\u679D"),   // 鏍戞灊
    QStringLiteral("\u5783\u573E"),   // 鍨冨溇
    QStringLiteral("\u79EF\u6C34")    // 绉按
};

inline const QStringList SEVERITY_LEVELS = {
    QStringLiteral("\u8F7B\u5EA6"),   // 杞诲害
    QStringLiteral("\u4E2D\u5EA6"),   // 涓害
    QStringLiteral("\u91CD\u5EA6")    // 閲嶅害
};

// Water consumption rate per obstacle (L/m)
inline double waterRate(const QString& severity) {
    if (severity == QStringLiteral("\u8F7B\u5EA6"))  return 25.0;
    if (severity == QStringLiteral("\u4E2D\u5EA6"))  return 50.0;
    if (severity == QStringLiteral("\u91CD\u5EA6"))  return 80.0;
    return 25.0;
}

// Labor rate per obstacle (min/m)
inline double laborRate(const QString& severity) {
    if (severity == QStringLiteral("\u8F7B\u5EA6"))  return 0.5;
    if (severity == QStringLiteral("\u4E2D\u5EA6"))  return 1.0;
    if (severity == QStringLiteral("\u91CD\u5EA6"))  return 2.0;
    return 0.5;
}

constexpr double SETUP_TIME = 10.0;   // min
constexpr double STONE_BONUS = 1.2;   // multiplier for 閲嶅害 + 鐭冲ご/娉ュ潡

// Pressure suggestion map
inline QString pressureSuggestion(const QString& severity, const QString& type) {
    if (severity == QStringLiteral("\u91CD\u5EA6")) {
        if (type == QStringLiteral("\u77F3\u5934") || type == QStringLiteral("\u6CE5\u5757"))
            return QStringLiteral("200Bar\u6162\u901F"); // 200Bar鎱㈤€?        return QStringLiteral("180Bar\u52A0\u5F3A");     // 180Bar鍔犲己
    }
    if (severity == QStringLiteral("\u4E2D\u5EA6"))
        return QStringLiteral("150Bar\u5E38\u89C4");     // 150Bar甯歌
    return QStringLiteral("120Bar\u8F7B\u51B2");          // 120Bar杞诲啿
}

// Severity colors
inline QMap<QString, QColor> severityColors() {
    return {
        {QStringLiteral("\u8F7B\u5EA6"), QColor("#10b981")},
        {QStringLiteral("\u4E2D\u5EA6"), QColor("#f59e0b")},
        {QStringLiteral("\u91CD\u5EA6"), QColor("#ef4444")}
    };
}

} // namespace DeviceConfig
