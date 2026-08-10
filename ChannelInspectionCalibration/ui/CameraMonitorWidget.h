// =============================================================================
// CameraMonitorWidget.h — Cleaning head real-time monitor panel
// No DeviceState copy; all telemetry pushed from MainWindow::onStateUpdated().
// =============================================================================
#pragma once

#include <QWidget>
#include <QLabel>
#include "../services/CameraService.h"
#include "../hardware/SimulationCamera.h"

class CameraMonitorWidget : public QWidget {
    Q_OBJECT
public:
    explicit CameraMonitorWidget(CameraService* camera, QWidget* parent = nullptr);

    // Called by MainWindow each tick — no DeviceState stored here
    void updateTelemetry(const SimulationCamera::Telemetry& t);

private:
    void setupUI();

    CameraService* m_camera = nullptr;

    QLabel* m_videoLabel   = nullptr;
    QLabel* m_statusLabel  = nullptr;
    QLabel* m_posLabel     = nullptr;
    QLabel* m_pressLabel   = nullptr;
    QLabel* m_flowLabel    = nullptr;
    QLabel* m_stateLabel   = nullptr;
};
