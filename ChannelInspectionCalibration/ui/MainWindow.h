// =============================================================================
// MainWindow.h — Main application window for 渠沟检测标定
// =============================================================================
#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTableView>
#include <QGroupBox>
#include <QTextEdit>
#include <QComboBox>
#include <QToolButton>
#include <QSplitter>
#include <QScrollArea>

#include "../services/SimulationEngine.h"
#include "../hardware/SimulationSensor.h"
#include "../visualization/ChannelMapWidget.h"
#include "ObstacleTableModel.h"
#include "WorkLogTableModel.h"
#include "AlarmPanelWidget.h"
#include "../services/CameraService.h"
#include "CameraMonitorWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStateUpdated();
    void onAlarmUpdated(const AlarmEvent& ev);
    void onRandomGenerate();
    void onGeneratePDF();
    void onExportData();
    void onFilterChanged(int mode);
    void onAddObstacle();
    void onDeleteSelected();
    void onUndo();
    void onRedo();
    void onStartSimulation();
    void onStopSimulation();
    void onChannelInfoChanged();
    void onObstacleDataModified();
    QWidget* createCollapsiblePanel(const QString& title, QWidget* parent);

private:
    void setupUI();
    void setupSidebar(QWidget* sidebar);
    void setupMainContent(QWidget* main);
    void refreshAll();
    void pushHistory();
    void applyObstaclesToEngine();

    // Engine & hardware
    SimulationEngine*  m_engine   = nullptr;
    SimulationSensor*  m_sensor   = nullptr;

    // Visualization
    ChannelMapWidget*  m_channelMap = nullptr;

    // Table models
    ObstacleTableModel* m_obstacleModel = nullptr;
    WorkLogTableModel*  m_workLogModel  = nullptr;

    // Sidebar widgets
    QLabel*           m_logoLabel      = nullptr;
    QLineEdit*        m_channelIdEdit  = nullptr;
    QDoubleSpinBox*   m_channelLenSpin = nullptr;
    QLineEdit*        m_operatorEdit  = nullptr;
    QDateEdit*        m_inspectDateEdit = nullptr;
    QPushButton*      m_randomBtn      = nullptr;
    QPushButton*      m_pdfBtn         = nullptr;
    QPushButton*      m_exportBtn      = nullptr;

    // Main content widgets
    QLabel*           m_titleLabel     = nullptr;
    QLabel*           m_subtitleLabel  = nullptr;
    QProgressBar*     m_waterBar       = nullptr;
    QLabel*           m_waterCapLabel  = nullptr;
    QLabel*           m_flowLabel      = nullptr;
    QLabel*           m_remainTimeLabel= nullptr;
    QLabel*           m_remainDistLabel= nullptr;
    QLabel*           m_completionLabel= nullptr;
    QLabel*           m_headStatusLabel= nullptr;
    QLabel*           m_headPressLabel = nullptr;
    QWidget*          m_headStatusFrame = nullptr;

    QButtonGroup*     m_filterGroup    = nullptr;
    QLabel*           m_filterCount    = nullptr;

    // Business panels
    QGroupBox*        m_resourceGroup  = nullptr;
    QGroupBox*        m_workOrderGroup = nullptr;
    QGroupBox*        m_dataGroup      = nullptr;
    QGroupBox*        m_logGroup       = nullptr;

    // Resource labels
    QLabel*           m_totalLenLabel  = nullptr;
    QLabel*           m_obsCountLabel  = nullptr;
    QLabel*           m_blockRateLabel = nullptr;
    QLabel*           m_heavyCountLabel= nullptr;
    QLabel*           m_totalWaterLabel= nullptr;
    QLabel*           m_totalLaborLabel= nullptr;

    // Work order
    QTextEdit*        m_workOrderText  = nullptr;

    // Data table
    QTableView*       m_dataTable      = nullptr;
    QPushButton*      m_addBtn         = nullptr;
    QPushButton*      m_delBtn         = nullptr;
    QPushButton*      m_undoBtn        = nullptr;
    QPushButton*      m_redoBtn        = nullptr;

    // Work log table
    QTableView*       m_logTable       = nullptr;

    // Simulation controls
    QPushButton*      m_startBtn       = nullptr;
    QPushButton*      m_stopBtn        = nullptr;
    AlarmPanelWidget* m_alarmPanel     = nullptr;
    CameraService*       m_cameraService  = nullptr;
    CameraMonitorWidget* m_cameraMonitor  = nullptr;

    // History for undo/redo
    QVector<QVector<Obstacle>> m_history;
    int m_historyPos = -1;
};
