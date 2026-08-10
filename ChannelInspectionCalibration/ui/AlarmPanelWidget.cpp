// =============================================================================
// AlarmPanelWidget.cpp — SCADA/HMI alarm center implementation
// =============================================================================
#include "AlarmPanelWidget.h"
#include "../services/AlarmManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFont>
#include <QFrame>

AlarmPanelWidget::AlarmPanelWidget(AlarmManager* manager, QWidget* parent)
    : QWidget(parent), m_alarmManager(manager)
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    setupCollapsedBar();
    setupExpandedPanel();

    rootLayout->addWidget(m_collapsedBar);
    rootLayout->addWidget(m_expandedPanel);
    m_expandedPanel->setVisible(false);
    refreshAlarms();
}

void AlarmPanelWidget::setupCollapsedBar() {
    m_collapsedBar = new QWidget(this);
    m_collapsedBar->setFixedHeight(34);
    m_collapsedBar->setCursor(Qt::PointingHandCursor);
    m_collapsedBar->setStyleSheet(QStringLiteral(
        "background-color: #22C55E; border-radius: 4px;"
    ));

    auto* barLayout = new QHBoxLayout(m_collapsedBar);
    barLayout->setContentsMargins(8, 4, 8, 4);
    barLayout->setSpacing(8);

    // Expand/collapse triangle button
    m_expandBtn = new QPushButton(QStringLiteral("\u25B6"));
    m_expandBtn->setFixedSize(24, 24);
    m_expandBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; border: none; color: white; font-size: 14px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.2); border-radius: 3px; }"
    ));
    m_expandBtn->setCursor(Qt::PointingHandCursor);
    barLayout->addWidget(m_expandBtn);

    // Title label
    auto* titleLbl = new QLabel(QStringLiteral("\u62A5\u8B66\u4E2D\u5FC3"));
    QFont tf; tf.setPointSize(12); tf.setBold(true);
    titleLbl->setFont(tf);
    titleLbl->setStyleSheet("color: white; background: transparent;");
    barLayout->addWidget(titleLbl);

    // Status text
    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: white; background: transparent; font-size: 12px;");
    barLayout->addWidget(m_statusLabel, 1);

    // Alarm count (right-aligned)
    m_alarmCountLabel = new QLabel();
    QFont cf; cf.setPointSize(12); cf.setBold(true);
    m_alarmCountLabel->setFont(cf);
    m_alarmCountLabel->setStyleSheet("color: white; background: transparent;");
    barLayout->addWidget(m_alarmCountLabel);

    // Breathing alarm light, inserted to the left of "活动报警" count label
    m_breathingLight = new BreathingAlarmLight(m_collapsedBar);
    m_breathingLight->setAlarmLevel(BreathingAlarmLight::Green);
    int labelIndex = barLayout->indexOf(m_alarmCountLabel);
    if (labelIndex != -1) {
        barLayout->insertWidget(labelIndex, m_breathingLight);
        barLayout->insertSpacing(labelIndex + 1, 6);
    } else {
        barLayout->addWidget(m_breathingLight);
    }

    connect(m_expandBtn, &QPushButton::clicked, this, &AlarmPanelWidget::onToggleExpand);
    // Also allow clicking the whole bar
    m_collapsedBar->installEventFilter(this);
}

bool AlarmPanelWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_collapsedBar && event->type() == QEvent::MouseButtonPress) {
        onToggleExpand();
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void AlarmPanelWidget::setupExpandedPanel() {
    m_expandedPanel = new QWidget(this);
    m_expandedPanel->setStyleSheet(QStringLiteral(
        "background-color: #F8FAFC; border: 2px solid #CBD5E1; border-top: none; "
        "border-bottom-left-radius: 4px; border-bottom-right-radius: 4px;"
    ));
    m_expandedPanel->setMinimumHeight(0);
    m_expandedPanel->setMaximumHeight(400);

    auto* layout = new QVBoxLayout(m_expandedPanel);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(4);

    // Title row
    auto* titleRow = new QHBoxLayout();
    m_titleLabel = new QLabel(QStringLiteral("\u62A5\u8B66\u4E2D\u5FC3"));
    QFont ttf; ttf.setPointSize(13); ttf.setBold(true);
    m_titleLabel->setFont(ttf);
    m_titleLabel->setStyleSheet("color: #0F172A; background: transparent;");
    titleRow->addWidget(m_titleLabel);
    titleRow->addStretch();

    m_ackBtn = new QPushButton(QStringLiteral("\u786E\u8BA4\u62A5\u8B66"));
    m_ackBtn->setStyleSheet("background-color:#3B82F6;color:white;font-weight:bold;border-radius:4px;padding:4px 12px;font-size:12px;");
    m_ackBtn->setCursor(Qt::PointingHandCursor);
    titleRow->addWidget(m_ackBtn);

    m_damageRecoverBtn = new QPushButton(QStringLiteral("\u6E05\u6D17\u5934\u635F\u574F\u5DF2\u4FEE\u590D"));
    m_damageRecoverBtn->setStyleSheet("background-color:#EF4444;color:white;font-weight:bold;border-radius:4px;padding:4px 12px;font-size:12px;");
    m_damageRecoverBtn->setCursor(Qt::PointingHandCursor);
    m_damageRecoverBtn->setEnabled(false);
    titleRow->addWidget(m_damageRecoverBtn);
    layout->addLayout(titleRow);

    // Table
    m_model = new AlarmHistoryModel(this);
    m_table = new QTableView();
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setAlternatingRowColors(true);
    m_table->setMinimumHeight(0);
    m_table->setMinimumHeight(120);
    m_table->setStyleSheet(QStringLiteral(
        "QTableView { gridline-color: #E2E8F0; background: white; }"
        "QTableView::item:selected { background-color: #DBEAFE; color: #0F172A; }"
        "QHeaderView::section { background: #E2E8F0; color: #475569; font-weight: bold; padding: 3px; border: none; }"
    ));
    m_table->setColumnWidth(0, 80);  // Level
    m_table->setColumnWidth(1, 70);  // Time
    m_table->setColumnWidth(2, 80);  // Type
    m_table->setColumnWidth(4, 90);  // Status
    m_table->verticalHeader()->setVisible(false);
    layout->addWidget(m_table, 1);

    connect(m_ackBtn, &QPushButton::clicked, this, &AlarmPanelWidget::onAcknowledge);
    connect(m_damageRecoverBtn, &QPushButton::clicked, this, &AlarmPanelWidget::onConfirmDamageRecovery);
}

int AlarmPanelWidget::contentHeight() const { return m_expandedPanel->isVisible() ? 1 : 0; }
void AlarmPanelWidget::setContentHeight(int h) {
    m_expandedPanel->setVisible(h != 0);
    if (m_collapsedBar) {
        QString barColor = m_collapsedBar->property("barColor").toString();
        if (barColor.isEmpty()) barColor = "#22C55E";
        if (h == 0) {
            m_collapsedBar->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 4px;").arg(barColor));
        }
    }
}

void AlarmPanelWidget::onToggleExpand() {
    m_expanded = !m_expanded;
    m_expandBtn->setText(m_expanded ? QStringLiteral("\u25BC") : QStringLiteral("\u25B6"));

    if (m_expanded) {
        m_collapsedBar->setStyleSheet(QStringLiteral(
            "background-color: %1; border-bottom-left-radius: 0; border-bottom-right-radius: 0;"
        ).arg(m_collapsedBar->property("barColor").toString()));
    }
    setContentHeight(m_expanded ? 1 : 0);
}

void AlarmPanelWidget::refreshAlarms() {
    if (!m_alarmManager) return;
    auto active = m_alarmManager->activeAlarms();
    m_model->setAlarms(active);
    int count = active.size();

    updateStatusDisplay();

    m_alarmCountLabel->setText(QStringLiteral("\u6D3B\u52A8\u62A5\u8B66: %1\u6761").arg(count));

    // Title shows count
    m_titleLabel->setText(QStringLiteral("\u62A5\u8B86\u4E2D\u5FC3  \u5F53\u524D\u6D3B\u52A8\u62A5\u8B66: %1\u6761").arg(count));

    // Enable damage recovery if HEAD-001 is active
    bool hasDamage = false;
    for (const auto& ev : active) {
        if (ev.alarmCode == QStringLiteral("ALM-HEAD-001")) { hasDamage = true; break; }
    }
    m_damageRecoverBtn->setEnabled(hasDamage);
}

void AlarmPanelWidget::updateStatusDisplay() {
    if (!m_alarmManager) return;
    auto active = m_alarmManager->activeAlarms();
    AlarmSeverity maxSev = m_alarmManager->highestSeverity();
    int count = active.size();

    QString barColor;
    QString statusText;
    switch (maxSev) {
    case AlarmSeverity::INFO:
        barColor = "#22C55E";
        statusText = (count == 0)
            ? QStringLiteral("\u7CFB\u7EDF\u8FD0\u884C\u6B63\u5E38\uFF0C\u65E0\u6D3B\u52A8\u62A5\u8B66")
            : QStringLiteral("\u7CFB\u7EDF\u8FD0\u884C\u6B63\u5E38");
        break;
    case AlarmSeverity::WARNING:
        barColor = "#F59E0B";
        statusText = QStringLiteral("\u5B58\u5728\u8B66\u544A\u62A5\u8B66\uFF0C\u8BF7\u68C0\u67E5\u8BBE\u5907\u72B6\u6001");
        break;
    case AlarmSeverity::ERROR:
        barColor = "#F97316";
        statusText = QStringLiteral("\u5B58\u5728\u4E25\u91CD\u62A5\u8B66\uFF0C\u8BF7\u5C3D\u5FEB\u5904\u7406");
        break;
    case AlarmSeverity::CRITICAL:
        barColor = "#EF4444";
        statusText = QStringLiteral("\u5B58\u5728\u5371\u6025\u62A5\u8B66\uFF0C\u8BF7\u7ACB\u5373\u5904\u7406");
        break;
    }

    // Drive the breathing light in sync with the bar color
    BreathingAlarmLight::AlarmLevel lightLevel = BreathingAlarmLight::Green;
    switch (maxSev) {
    case AlarmSeverity::WARNING:
        lightLevel = BreathingAlarmLight::Yellow;
        break;
    case AlarmSeverity::ERROR:
    case AlarmSeverity::CRITICAL:
        lightLevel = BreathingAlarmLight::Red;
        break;
    default:
        lightLevel = BreathingAlarmLight::Green;
        break;
    }
    if (m_breathingLight) m_breathingLight->setAlarmLevel(lightLevel);

    m_statusLabel->setText(statusText);

    // Store bar color for animation reuse
    m_collapsedBar->setProperty("barColor", barColor);
    if (!m_expanded) {
        m_collapsedBar->setStyleSheet(QStringLiteral(
            "background-color: %1; border-radius: 4px;"
        ).arg(barColor));
    } else {
        m_collapsedBar->setStyleSheet(QStringLiteral(
            "background-color: %1; border-bottom-left-radius: 0; border-bottom-right-radius: 0;"
        ).arg(barColor));
    }
}

void AlarmPanelWidget::onAcknowledge() {
    auto idx = m_table->currentIndex();
    if (!idx.isValid()) return;
    auto active = m_alarmManager->activeAlarms();
    if (idx.row() < active.size()) {
        m_alarmManager->acknowledgeAlarm(active[idx.row()].alarmId);
        refreshAlarms();
    }
}

void AlarmPanelWidget::onConfirmDamageRecovery() {
    emit damageConfirmed();
    refreshAlarms();
}
