// =============================================================================
// SimulationCamera.cpp — 640x360 16:9 simulated camera frame renderer
// =============================================================================
#include "SimulationCamera.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRandomGenerator>

SimulationCamera::SimulationCamera() {
    m_headIcon = QPixmap(QStringLiteral(":/head_icon"));
    if (!m_headIcon.isNull())
        m_headIcon = m_headIcon.scaled(72, 72, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void SimulationCamera::resetAnimation() {
    m_tickCount = 0;
    m_particles.clear();
}

QImage SimulationCamera::currentFrame() {
    ++m_tickCount;
    updateSprayParticles();

    QImage frame(FRAME_W, FRAME_H, QImage::Format_RGB32);
    QPainter p(&frame);
    p.setRenderHint(QPainter::Antialiasing);

    renderBackground(p);
    renderSpray(p);
    renderHead(p);
    renderHUD(p);
    p.end();

    return frame;
}

void SimulationCamera::updateSprayParticles() {
    auto* rng = QRandomGenerator::global();
    for (int i = 0; i < 3; ++i) {
        Particle pt;
        pt.x     = 0.0;
        pt.y     = 0.0;
        pt.speed = rng->generateDouble() * 0.035 + 0.015;
        pt.life  = 1.0;
        m_particles.append(pt);
    }
    for (auto& pt : m_particles) {
        double a = std::atan2(pt.y, pt.x) + rng->generateDouble() * 0.1 - 0.05;
        double s = pt.speed;
        pt.x += std::cos(a) * s;
        pt.y += std::sin(a) * s;
        pt.life -= 0.05;
    }
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
                       [](const Particle& p) { return p.life <= 0.0; }),
        m_particles.end());
    while (m_particles.size() > 60)
        m_particles.removeFirst();
}

void SimulationCamera::renderBackground(QPainter& p) {
    p.fillRect(0, 0, FRAME_W, FRAME_H, QColor(28, 32, 40));
    // Channel floor
    double floorY = FRAME_H * 0.78;
    p.fillRect(0, static_cast<int>(floorY), FRAME_W,
               FRAME_H - static_cast<int>(floorY), QColor(45, 50, 58));
    // Wall texture lines
    p.setPen(QPen(QColor(55, 60, 70, 100), 1));
    for (int x = 0; x < FRAME_W; x += 45) {
        int px = (x + m_tickCount * 3) % (FRAME_W + 45) - 22;
        p.drawLine(px, static_cast<int>(floorY) - 22,
                   px + 18, static_cast<int>(floorY) - 6);
    }
}

void SimulationCamera::renderSpray(QPainter& p) {
    // Particles
    for (const auto& pt : m_particles) {
        int sx = FRAME_W / 2 + static_cast<int>(pt.x * FRAME_W * 0.35);
        int sy = FRAME_H / 2 + static_cast<int>(pt.y * FRAME_H * 0.30);
        int alpha = static_cast<int>(pt.life * 200);
        int sz    = std::max(1, static_cast<int>(pt.life * 5));
        p.setBrush(QColor(80, 170, 255, alpha));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(sx, sy), sz, sz);
    }
    // Main jet cone
    QLinearGradient jg(FRAME_W/2.0, FRAME_H/2.0-10,
                       FRAME_W/2.0+55, FRAME_H/2.0+25);
    jg.setColorAt(0.0, QColor(100, 190, 255, 140));
    jg.setColorAt(0.5, QColor(70, 150, 220, 70));
    jg.setColorAt(1.0, QColor(40, 110, 190, 15));
    p.setBrush(jg);
    p.setPen(Qt::NoPen);
    QPainterPath jetPath;
    jetPath.moveTo(FRAME_W/2.0-7, FRAME_H/2.0-7);
    jetPath.quadTo(FRAME_W/2.0+45, FRAME_H/2.0-25,
                   FRAME_W/2.0+75, FRAME_H/2.0+8);
    jetPath.quadTo(FRAME_W/2.0+45, FRAME_H/2.0+25,
                   FRAME_W/2.0-7,  FRAME_H/2.0+7);
    p.drawPath(jetPath);
}

void SimulationCamera::renderHead(QPainter& p) {
    if (!m_headIcon.isNull()) {
        int hx = FRAME_W/2 - m_headIcon.width()/2;
        int hy = FRAME_H/2 - m_headIcon.height()/2;
        p.drawPixmap(hx, hy, m_headIcon);
    } else {
        p.setBrush(QColor(2, 132, 199));
        p.setPen(QPen(QColor(2, 132, 199), 2));
        QPolygonF tri;
        tri << QPointF(FRAME_W/2.0, FRAME_H/2.0-18)
            << QPointF(FRAME_W/2.0-14, FRAME_H/2.0+14)
            << QPointF(FRAME_W/2.0+14, FRAME_H/2.0+14);
        p.drawPolygon(tri);
    }
    // Crosshair
    p.setPen(QPen(QColor(0, 200, 255, 70), 1));
    p.drawLine(FRAME_W/2-18, FRAME_H/2, FRAME_W/2-5, FRAME_H/2);
    p.drawLine(FRAME_W/2+5,  FRAME_H/2, FRAME_W/2+18, FRAME_H/2);
    p.drawLine(FRAME_W/2, FRAME_H/2-18, FRAME_W/2, FRAME_H/2-5);
    p.drawLine(FRAME_W/2, FRAME_H/2+5,  FRAME_W/2, FRAME_H/2+18);
}

void SimulationCamera::renderHUD(QPainter& p) {
    QFont f; f.setPointSize(10); f.setBold(true);
    p.setFont(f);
    int lx = 10, ly = 20, gap = 18;

    // Status color
    QString color;
    switch (m_camStatus) {
    case CameraStatus::Online:     color = "#22C55E"; break;
    case CameraStatus::Connecting: color = "#F59E0B"; break;
    case CameraStatus::Error:      color = "#EF4444"; break;
    default:                       color = "#6B7280"; break;
    }

    p.setPen(QColor(color));
    p.drawText(lx, ly, QStringLiteral("\u25CF  %1").arg(cameraStatusString(m_camStatus)));
    p.setPen(QColor(0, 200, 255, 200));

    // Device info
    f.setPointSize(9);
    p.setFont(f);
    ly += gap;
    p.drawText(lx, ly, QStringLiteral("\u8BBE\u5907: HEAD-CAM-001"));

    ly += gap;
    p.drawText(lx, ly, QStringLiteral("\u4F4D\u7F6E: %1 m").arg(m_telem.pos, 0, 'f', 1));

    ly += gap;
    p.drawText(lx, ly, QStringLiteral("\u6C34\u538B: %1 Bar").arg(m_telem.pressure, 0, 'f', 0));

    ly += gap;
    p.drawText(lx, ly, QStringLiteral("\u6D41\u91CF: %1 L/min").arg(m_telem.flow, 0, 'f', 0));

    ly += gap;
    QStringList stateLabels = {
        QStringLiteral("\u6E05\u6D17\u4E2D"),
        QStringLiteral("\u8B66\u544A"),
        QStringLiteral("\u7834\u635F"),
        QStringLiteral("\u5DF2\u505C\u6B62")
    };
    QString stateTxt = (m_telem.headSev >= 0 && m_telem.headSev < stateLabels.size())
                       ? stateLabels[m_telem.headSev] : QStringLiteral("--");
    p.drawText(lx, ly, QStringLiteral("\u8FD0\u884C: %1").arg(stateTxt));

    // Top-right timestamp-like indicator
    p.setPen(QColor(100, 120, 140, 120));
    f.setPointSize(8);
    p.setFont(f);
    p.drawText(FRAME_W - 80, 16, QStringLiteral("FPS ~7"));
}
