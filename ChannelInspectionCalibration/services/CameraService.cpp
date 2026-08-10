// =============================================================================
// CameraService.cpp — Frame scheduling & dispatch, no rendering logic.
// =============================================================================
#include "CameraService.h"

CameraService::CameraService(QObject* parent)
    : QObject(parent)
{
    m_camera = new SimulationCamera();   // no parent — standalone object, not a QWidget
    m_timer = new QTimer(this);
    m_timer->setInterval(TICK_MS);
    connect(m_timer, &QTimer::timeout, this, &CameraService::onTick);
}

void CameraService::start() {
    m_camera->setStatus(CameraStatus::Connecting);
    emit statusChanged(CameraStatus::Connecting);

    m_camera->resetAnimation();
    m_timer->start();

    // Simulate connection delay, then go online
    QTimer::singleShot(600, this, [this]() {
        m_camera->setStatus(CameraStatus::Online);
        emit statusChanged(CameraStatus::Online);
    });
}

void CameraService::stop() {
    m_timer->stop();
    m_camera->setStatus(CameraStatus::Offline);
    emit statusChanged(CameraStatus::Offline);
}

void CameraService::setTelemetry(const SimulationCamera::Telemetry& t) {
    m_camera->setTelemetry(t);
}

void CameraService::onTick() {
    QImage frame = m_camera->currentFrame();
    emit frameReady(frame);
}
