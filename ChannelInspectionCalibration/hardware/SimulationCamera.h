// =============================================================================
// SimulationCamera.h — Simulated camera feeding generated industrial frames
// Implements CameraInterface; produces 640x360 16:9 frames with animated
// cleaning-head view, spray particles, and HUD overlay.
// =============================================================================
#pragma once

#include "CameraInterface.h"
#include <QPixmap>
#include <QVector>
#include <cmath>

class SimulationCamera : public CameraInterface {
public:
    SimulationCamera();
    ~SimulationCamera() override = default;

    // CameraInterface
    QImage       currentFrame() override;
    CameraStatus status() const override { return m_camStatus; }

    void setStatus(CameraStatus s) { m_camStatus = s; }
    void resetAnimation();

    // Telemetry data injected before each frame render
    struct Telemetry {
        double pos      = 0.0;
        double pressure = 150.0;
        double flow     = 120.0;
        int    headSev  = 0;   // 0=Normal, 1=Warning, 2=Damage, 3=Stopped
    };
    void setTelemetry(const Telemetry& t) { m_telem = t; }

    static constexpr int FRAME_W = 640;
    static constexpr int FRAME_H = 360;

private:
    void updateSprayParticles();
    void renderBackground(QPainter& p);
    void renderSpray(QPainter& p);
    void renderHead(QPainter& p);
    void renderHUD(QPainter& p);

    int  m_tickCount = 0;
    CameraStatus m_camStatus = CameraStatus::Offline;
    Telemetry m_telem;

    QPixmap m_headIcon;   // cached scaled icon

    struct Particle { double x, y, speed, life; };
    QVector<Particle> m_particles;
};
