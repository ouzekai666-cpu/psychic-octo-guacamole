// =============================================================================
// CameraService.h — Camera business logic: scheduling, frame dispatch, status
// Owns a SimulationCamera (or real camera via CameraInterface in future).
// =============================================================================
#pragma once

#include <QObject>
#include <QTimer>
#include <QImage>
#include "../hardware/CameraInterface.h"
#include "../hardware/SimulationCamera.h"

class CameraService : public QObject {
    Q_OBJECT
public:
    explicit CameraService(QObject* parent = nullptr);
    ~CameraService() override = default;

    void start();
    void stop();

    CameraStatus status() const { return m_camera ? m_camera->status() : CameraStatus::Offline; }

    // Inject telemetry into the simulation camera before each frame
    void setTelemetry(const SimulationCamera::Telemetry& t);

signals:
    void frameReady(const QImage& frame);
    void statusChanged(CameraStatus status);

private slots:
    void onTick();

private:
    QTimer*            m_timer  = nullptr;
    SimulationCamera*  m_camera = nullptr;   // concrete impl, replaceable via interface
    static constexpr int TICK_MS = 150;      // ~7 FPS
};
