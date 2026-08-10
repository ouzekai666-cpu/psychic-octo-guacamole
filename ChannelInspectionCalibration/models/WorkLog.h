// =============================================================================
// WorkLog.h — Work log entry matching all 16 fields from the original app
// =============================================================================
#pragma once

#include <QString>
#include <QDateTime>

struct WorkLogEntry {
    QString  taskId;             // 任务编号
    QString  operatorName;       // 负责人
    QString  dateTime;           // 日期时间
    QString  channelId;          // 渠道编号
    double   channelLength = 0;   // 渠道长度(m)
    double   cleanedLength = 0;   // 当前清洗长度(m)
    QString  startTime;          // 开始时间
    QString  endTime;            // 结束时间
    double   totalWater    = 0;   // 累计用水量(L)
    double   avgFlowRate   = 0;   // 平均流量(L/min)
    double   maxPressure   = 0;   // 最大压力(Bar)
    int      obstacleCount = 0;   // 发现障碍数量
    QString  maxSeverity;        // 堵塞最高等级
    QString  anomalyRecord;      // 异常记录
    QString  result;             // 处理结果
    QString  deviceStatus;       // 设备状态

    static QStringList headers() {
        return {
            QStringLiteral("\u4EFB\u52A1\u7F16\u53F7"),       // 任务编号
            QStringLiteral("\u8D1F\u8D23\u4EBA"),             // 负责人
            QStringLiteral("\u65E5\u671F\u65F6\u95F4"),       // 日期时间
            QStringLiteral("\u6E20\u9053\u7F16\u53F7"),       // 渠道编号
            QStringLiteral("\u6E20\u9053\u957F\u5EA6(m)"),    // 渠道长度(m)
            QStringLiteral("\u5F53\u524D\u6E05\u6D17\u957F\u5EA6(m)"), // 当前清洗长度(m)
            QStringLiteral("\u5F00\u59CB\u65F6\u95F4"),       // 开始时间
            QStringLiteral("\u7ED3\u675F\u65F6\u95F4"),       // 结束时间
            QStringLiteral("\u7D2F\u8BA1\u7528\u6C34\u91CF(L)"),  // 累计用水量(L)
            QStringLiteral("\u5E73\u5747\u6D41\u91CF(L/min)"), // 平均流量(L/min)
            QStringLiteral("\u6700\u5927\u538B\u529B(Bar)"),  // 最大压力(Bar)
            QStringLiteral("\u53D1\u73B0\u969C\u7887\u6570\u91CF"), // 发现障碍数量
            QStringLiteral("\u5835\u585E\u6700\u9AD8\u7B49\u7EA7"), // 堵塞最高等级
            QStringLiteral("\u5F02\u5E38\u8BB0\u5F55"),       // 异常记录
            QStringLiteral("\u5904\u7406\u7ED3\u679C"),       // 处理结果
            QStringLiteral("\u8BBE\u5907\u72B6\u6001")        // 设备状态
        };
    }
};
