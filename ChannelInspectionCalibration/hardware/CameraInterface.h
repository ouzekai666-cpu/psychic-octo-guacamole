// =============================================================================
// CameraInterface.h — Abstract camera interface + status enum
// Future: replace SimulationCamera with real IP/USB camera without UI changes.
// =============================================================================
#pragma once

#include <QImage>
#include <QString>

enum class CameraStatus {
    Offline,
    Connecting,
    Online,
    Error
};

inline QString cameraStatusString(CameraStatus s) {
    switch (s) {
    case CameraStatus::Offline:    return QStringLiteral("\u79BB\u7EBF");
    case CameraStatus::Connecting: return QStringLiteral("\u8FDE\u63A5\u4E2D");
    case CameraStatus::Online:     return QStringLiteral("\u5728\u7EBF");
    case CameraStatus::Error:      return QStringLiteral("\u6545\u969C");
    }
    return {};
}

class CameraInterface {
public:
    virtual ~CameraInterface() = default;
    virtual QImage       currentFrame() = 0;
    virtual CameraStatus status() const = 0;
};
