// =============================================================================
// CameraMonitorWidget.cpp — Cleaning head real-time monitor
// Frame from CameraService signal; telemetry pushed from MainWindow.
// =============================================================================
#include "CameraMonitorWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFont>

namespace {

// Video label whose sizeHint is fixed to its minimum size, so pushing frames
// never triggers a layout recalculation that makes the monitor grow/shrink.
class VideoLabel : public QLabel {
public:
    explicit VideoLabel(QWidget* parent = nullptr) : QLabel(parent) {}
    QSize sizeHint() const override { return minimumSize(); }
    QSize minimumSizeHint() const override { return minimumSize(); }
};

} // namespace

CameraMonitorWidget::CameraMonitorWidget(CameraService* camera, QWidget* parent)
    : QWidget(parent), m_camera(camera)
{
    setupUI();

    // Frame signal from CameraService
    connect(m_camera, &CameraService::frameReady, this, [this](const QImage& frame) {
        if (frame.isNull() || m_videoLabel->size().isEmpty()) return;
        QPixmap scaledPix = QPixmap::fromImage(frame).scaled(
            m_videoLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_videoLabel->setPixmap(scaledPix);
    });

    // Camera status changes
    connect(m_camera, &CameraService::statusChanged, this, [this](CameraStatus s) {
        QString color;
        switch (s) {
        case CameraStatus::Online:     color = "#22C55E"; break;
        case CameraStatus::Connecting: color = "#F59E0B"; break;
        case CameraStatus::Error:      color = "#EF4444"; break;
        default:                       color = "#6B7280"; break;
        }
        m_statusLabel->setText(QStringLiteral("\u25CF  %1").arg(cameraStatusString(s)));
        m_statusLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: bold; font-size: 14px; background: transparent;").arg(color));
    });
}

void CameraMonitorWidget::setupUI() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    // Title
    auto* titleLbl = new QLabel(QStringLiteral("\u6E05\u6D17\u5934\u89C6\u89C9\u76D1\u63A7"));
    QFont tf; tf.setPointSize(14); tf.setBold(true);
    titleLbl->setFont(tf);
    titleLbl->setStyleSheet("color: #0F172A;");
    root->addWidget(titleLbl);

    // Row: video | telemetry
    auto* row = new QHBoxLayout();
    row->setSpacing(12);

    // Video feed
    m_videoLabel = new VideoLabel();
    m_videoLabel->setMinimumSize(SimulationCamera::FRAME_W, SimulationCamera::FRAME_H);
    m_videoLabel->setScaledContents(true);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoLabel->setStyleSheet(QStringLiteral(
        "QLabel { background-color: #1C2028; border: 2px solid #0284C7;"
        " border-radius: 6px; color: #94A3B8; font-size: 13px; }"
    ));
    m_videoLabel->setText(QStringLiteral("\u6A21\u62DF\u6444\u50CF\u5934\u753B\u9762"));
    row->addWidget(m_videoLabel);
    row->setStretchFactor(m_videoLabel, 4);

    // Telemetry panel
    auto* telemGroup = new QGroupBox(QStringLiteral("\u8FD0\u884C\u53C2\u6570"));
    telemGroup->setStyleSheet(QStringLiteral(
        "QGroupBox { font-weight: bold; font-size: 13px; color: #0F172A;"
        " border: 2px solid #CBD5E1; border-radius: 6px; margin-top: 14px;"
        " padding-top: 18px; background: white; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 6px; }"
    ));
    telemGroup->setFixedWidth(200);
    auto* telemLayout = new QVBoxLayout(telemGroup);
    telemLayout->setSpacing(8);

    auto makeLbl = [](const QString& prefix) {
        auto* l = new QLabel(prefix);
        l->setStyleSheet("font-size: 14px; color: #0F172A; background: transparent;");
        l->setWordWrap(true);
        return l;
    };

    m_statusLabel = makeLbl(QStringLiteral("\u25CF  %1").arg(cameraStatusString(CameraStatus::Offline)));
    m_statusLabel->setStyleSheet("color: #6B7280; font-weight: bold; font-size: 14px; background: transparent;");
    m_posLabel    = makeLbl(QStringLiteral("\u4F4D\u7F6E: -- m"));
    m_pressLabel  = makeLbl(QStringLiteral("\u6C34\u538B: -- Bar"));
    m_flowLabel   = makeLbl(QStringLiteral("\u6D41\u91CF: -- L/min"));
    m_stateLabel  = makeLbl(QStringLiteral("\u8FD0\u884C\u72B6\u6001: --"));

    telemLayout->addWidget(m_statusLabel);
    telemLayout->addWidget(m_posLabel);
    telemLayout->addWidget(m_pressLabel);
    telemLayout->addWidget(m_flowLabel);
    telemLayout->addWidget(m_stateLabel);
    telemLayout->addStretch();

    row->addWidget(telemGroup, 1);
    row->setStretchFactor(telemGroup, 1);
    root->addLayout(row);
}

void CameraMonitorWidget::updateTelemetry(const SimulationCamera::Telemetry& t) {
    m_posLabel->setText(QStringLiteral("\u4F4D\u7F6E: %1 m").arg(t.pos, 0, 'f', 1));
    m_pressLabel->setText(QStringLiteral("\u6C34\u538B: %1 Bar").arg(t.pressure, 0, 'f', 0));
    m_flowLabel->setText(QStringLiteral("\u6D41\u91CF: %1 L/min").arg(t.flow, 0, 'f', 0));

    QStringList stateLabels = {
        QStringLiteral("\u6E05\u6D17\u4E2D"),
        QStringLiteral("\u8B66\u544A"),
        QStringLiteral("\u7834\u635F"),
        QStringLiteral("\u5DF2\u505C\u6B62")
    };
    QString stateTxt = (t.headSev >= 0 && t.headSev < stateLabels.size())
                       ? stateLabels[t.headSev] : QStringLiteral("--");
    m_stateLabel->setText(QStringLiteral("\u8FD0\u884C\u72B6\u6001: %1").arg(stateTxt));
}
