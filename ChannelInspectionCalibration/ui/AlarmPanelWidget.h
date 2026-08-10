// =============================================================================
// AlarmPanelWidget.h — Industrial SCADA/HMI alarm center panel
// Collapsible bar + animated expand/collapse + severity color bars
// UI only: zero alarm judgment logic (all data from AlarmManager)
// =============================================================================
#pragma once
#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QLabel>
#include <QEvent>
#include <QColor>
#include <QPainter>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include "../database/AlarmHistoryModel.h"
#include "../models/AlarmEvent.h"

class AlarmManager;

class BreathingAlarmLight : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal breathAlpha READ breathAlpha WRITE setBreathAlpha NOTIFY breathAlphaChanged)

public:
    enum AlarmLevel { Green, Yellow, Red, Off };

    explicit BreathingAlarmLight(QWidget* parent = nullptr)
        : QWidget(parent), m_level(Green), m_color(Qt::green), m_breathAlpha(1.0)
    {
        setFixedSize(16, 16);

        m_anim = new QPropertyAnimation(this, "breathAlpha", this);
        m_anim->setEasingCurve(QEasingCurve::InOutSine);
        m_anim->setLoopCount(-1);
        m_anim->setKeyValueAt(0.0, 0.2);
        m_anim->setKeyValueAt(0.5, 1.0);
        m_anim->setKeyValueAt(1.0, 0.2);

        setAlarmLevel(Green);
    }

    void setAlarmLevel(AlarmLevel level) {
        if (m_level == level && m_anim->state() == QAbstractAnimation::Running) return;

        m_level = level;
        m_anim->stop();

        int duration = 2000;
        if (level == Green)       { m_color = QColor(0, 255, 120);  duration = 2000; }
        else if (level == Yellow) { m_color = QColor(255, 220, 0);  duration = 1000; }
        else if (level == Red)    { m_color = QColor(255, 60, 60);  duration = 500;  }
        else if (level == Off)    { m_color = QColor(148, 163, 184); }

        m_anim->setDuration(duration);
        if (level != Off) m_anim->start();
        update();
    }

    qreal breathAlpha() const { return m_breathAlpha; }
    void setBreathAlpha(qreal alpha) { m_breathAlpha = alpha; update(); }

signals:
    void breathAlphaChanged(qreal alpha);

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QColor drawColor = m_color;
        drawColor.setAlphaF(m_breathAlpha);

        QPointF center(width() / 2.0, height() / 2.0);

        // 1. White translucent backing pad, keeps the lamp visible on any bar color
        painter.setBrush(QColor(255, 255, 255, 180));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(center, 7.5, 7.5);

        // 2. Breathing core
        painter.setBrush(drawColor);
        painter.drawEllipse(center, 6.0, 6.0);
    }

private:
    AlarmLevel m_level;
    QColor m_color;
    qreal m_breathAlpha;
    QPropertyAnimation* m_anim;
};

class AlarmPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit AlarmPanelWidget(AlarmManager* manager, QWidget* parent = nullptr);

    void refreshAlarms();

signals:
    void damageConfirmed();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onToggleExpand();
    void onAcknowledge();
    void onConfirmDamageRecovery();

private:
    int  contentHeight() const;
    void setContentHeight(int h);
    void setupCollapsedBar();
    void setupExpandedPanel();
    void updateStatusDisplay();

    AlarmManager*      m_alarmManager;
    AlarmHistoryModel* m_model;

    // Collapsed bar widgets
    QWidget*      m_collapsedBar;
    QPushButton*  m_expandBtn;
    QLabel*       m_statusLabel;
    QLabel*       m_alarmCountLabel;
    BreathingAlarmLight* m_breathingLight = nullptr;

    // Expanded panel widgets
    QWidget*      m_expandedPanel;
    QLabel*       m_titleLabel;
    QTableView*   m_table;
    QPushButton*  m_ackBtn;
    QPushButton*  m_damageRecoverBtn;
    bool m_expanded = false;
};
