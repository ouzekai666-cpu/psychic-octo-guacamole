// =============================================================================
// MainWindow.cpp
// =============================================================================
#include "MainWindow.h"
#include "../config/DeviceConfig.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QScrollArea>
#include <QHeaderView>
#include <QFont>
#include <QDateTime>
#include <QTimer>
#include <QApplication>
#include <QScreen>
#include <QCoreApplication>
#include <random>

static QWidget* panelContent(QWidget* panel) {
    return panel->property("contentWidget").value<QWidget*>();
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("\u6E20\u6C9F\u6E05\u7406\u6807\u5B9A"));
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen) { setGeometry(screen->availableGeometry()); }
    else { resize(1400, 900); }
    m_engine = new SimulationEngine(this);
    m_sensor = new SimulationSensor(100.0, this);
    m_engine->setSensor(m_sensor);
    QVector<Obstacle> defaults = {
        {1, QStringLiteral("\u6CE5\u6C99"), 12.0, 15.0, QStringLiteral("\u8F7B\u5EA6"), QStringLiteral("\u8F7B\u5FAE\u6DE4\u79EF")},
        {2, QStringLiteral("\u77F3\u5934"), 35.0, 38.0, QStringLiteral("\u91CD\u5EA6"), QStringLiteral("\u5927\u5757\u5CA9\u77F3")},
        {3, QStringLiteral("\u6811\u679D"), 70.0, 72.5, QStringLiteral("\u4E2D\u5EA6"), QStringLiteral("\u65AD\u679D\u5806\u79EF")},
        {4, QStringLiteral("\u6CE5\u5757"), 50.0, 52.0, QStringLiteral("\u4E2D\u5EA6"), QStringLiteral("\u6CE5\u5757\u6C89\u79EF")},
        {5, QStringLiteral("\u5783\u573E"), 85.0, 87.0, QStringLiteral("\u8F7B\u5EA6"), QStringLiteral("\u751F\u6D3B\u5783\u573E")}
    };
    m_engine->setObstacles(defaults);
    // Camera service for cleaning head monitor
    m_cameraService = new CameraService(this);
    setupUI();
    pushHistory();
    connect(m_engine, &SimulationEngine::stateUpdated, this, &MainWindow::onStateUpdated);
    connect(m_engine, &SimulationEngine::alarmUpdated, this, &MainWindow::onAlarmUpdated);
    connect(m_alarmPanel, &AlarmPanelWidget::damageConfirmed, [this]() {
        m_engine->recoverFromDamage();
        m_startBtn->setEnabled(false);
        m_stopBtn->setEnabled(true);
    });
    refreshAll();
}

MainWindow::~MainWindow() { m_engine->stop(); }

QWidget* MainWindow::createCollapsiblePanel(const QString& title, QWidget* parent) {
    auto* panel = new QWidget(parent);
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto* headerBtn = new QPushButton(QStringLiteral("\u25B6 ") + title);
    headerBtn->setCheckable(true);
    headerBtn->setChecked(false);
    headerBtn->setCursor(Qt::PointingHandCursor);
    headerBtn->setStyleSheet(QStringLiteral(
        "QPushButton { text-align: left; font-weight: bold; font-size: 13px; "
        "color: #0F172A; background: #E2E8F0; border: 2px solid #CBD5E1; "
        "border-radius: 6px; padding: 8px 12px; }"
        "QPushButton:hover { background: #CBD5E1; }"
        "QPushButton:checked { border-bottom-left-radius: 0; border-bottom-right-radius: 0; }"
    ));
    layout->addWidget(headerBtn);
    auto* content = new QWidget();
    content->setObjectName("panelContent");
    content->setVisible(false);
    content->setStyleSheet(QStringLiteral(
        "#panelContent { background: white; border: 2px solid #CBD5E1; "
        "border-top: none; border-bottom-left-radius: 6px; border-bottom-right-radius: 6px; padding: 8px; }"
    ));
    layout->addWidget(content);
    QObject::connect(headerBtn, &QPushButton::toggled, [headerBtn, content](bool checked) {
        content->setVisible(checked);
        QString baseTitle = headerBtn->toolTip().isEmpty() ? headerBtn->text().mid(2) : headerBtn->toolTip();
        headerBtn->setToolTip(baseTitle);
        headerBtn->setText((checked ? QStringLiteral("\u25BC ") : QStringLiteral("\u25B6 ")) + baseTitle);
    });
    panel->setProperty("contentWidget", QVariant::fromValue<QWidget*>(content));
    return panel;
}

void MainWindow::setupUI() {
    auto* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    auto* rootLayout = new QHBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    auto* sidebar = new QWidget();
    sidebar->setFixedWidth(230);
    sidebar->setStyleSheet("background-color: #E2E8F0;");
    setupSidebar(sidebar);
    rootLayout->addWidget(sidebar);
    auto* mainContent = new QWidget();
    mainContent->setStyleSheet("background-color: #F1F5F9;");
    setupMainContent(mainContent);
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidget(mainContent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background-color: #F1F5F9;");
    rootLayout->addWidget(scrollArea, 1);
}

void MainWindow::setupSidebar(QWidget* sidebar) {
    auto* layout = new QVBoxLayout(sidebar);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    m_logoLabel = new QLabel();
    m_logoLabel->setAlignment(Qt::AlignCenter);
    m_logoLabel->setFixedHeight(80);
    m_logoLabel->setScaledContents(false);
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList logoPaths = { appDir + "/logo.png", QStringLiteral("assets/logo.png"), QStringLiteral("logo.png") };
    for (const auto& p : logoPaths) {
        if (QFile::exists(p)) { QPixmap px(p); if (!px.isNull()) { m_logoLabel->setPixmap(px.scaled(130, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation)); break; } }
    }
    layout->addWidget(m_logoLabel);
    auto* sidebarTitle = new QLabel(QStringLiteral("\u6E20\u6C9F\u6E05\u7406\u6807\u5B9A"));
    QFont stf; stf.setPointSize(14); stf.setBold(true);
    sidebarTitle->setFont(stf); sidebarTitle->setAlignment(Qt::AlignCenter);
    sidebarTitle->setStyleSheet("color: #0F172A;"); layout->addWidget(sidebarTitle);
    auto* sep = new QFrame(); sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #94A3B8;"); layout->addWidget(sep);
    auto* configLabel = new QLabel(QStringLiteral("\u73B0\u573A\u914D\u7F6E"));
    QFont cf; cf.setPointSize(12); cf.setBold(true);
    configLabel->setFont(cf); configLabel->setStyleSheet("color: #0F172A;"); layout->addWidget(configLabel);
    layout->addWidget(new QLabel(QStringLiteral("\u6E20\u9053\u7F16\u53F7")));
    m_channelIdEdit = new QLineEdit(QStringLiteral("A1\u8DEF\u6BB5\u4E3B\u6C9F\u6E20"));
    m_channelIdEdit->setStyleSheet("background:white; border:1px solid #CBD5E1; border-radius:4px; padding:4px;");
    layout->addWidget(m_channelIdEdit);
    layout->addWidget(new QLabel(QStringLiteral("\u957F\u5EA6(m)")));
    m_channelLenSpin = new QDoubleSpinBox(); m_channelLenSpin->setRange(1.0, 10000.0);
    m_channelLenSpin->setValue(100.0); m_channelLenSpin->setDecimals(1);
    m_channelLenSpin->setStyleSheet("background:white; border:1px solid #CBD5E1; border-radius:4px; padding:4px;");
    layout->addWidget(m_channelLenSpin);
    layout->addWidget(new QLabel(QStringLiteral("\u64CD\u4F5C\u5458")));
    m_operatorEdit = new QLineEdit(QStringLiteral("\u5F20\u5DE5\u7A0B\u5E08"));
    m_operatorEdit->setStyleSheet("background:white; border:1px solid #CBD5E1; border-radius:4px; padding:4px;");
    layout->addWidget(m_operatorEdit);
    layout->addWidget(new QLabel(QStringLiteral("\u68C0\u67E5\u65E5\u671F")));
    m_inspectDateEdit = new QDateEdit(QDate::currentDate());
    m_inspectDateEdit->setCalendarPopup(true); m_inspectDateEdit->setDisplayFormat("yyyy-MM-dd");
    m_inspectDateEdit->setStyleSheet("background:white; border:1px solid #CBD5E1; border-radius:4px; padding:4px;");
    layout->addWidget(m_inspectDateEdit);
    connect(m_channelIdEdit, &QLineEdit::editingFinished, this, &MainWindow::onChannelInfoChanged);
    connect(m_channelLenSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) { onChannelInfoChanged(); });
    connect(m_operatorEdit, &QLineEdit::editingFinished, this, &MainWindow::onChannelInfoChanged);
    connect(m_inspectDateEdit, &QDateEdit::dateChanged, this, [this](const QDate&) { onChannelInfoChanged(); });
    layout->addSpacing(6);
    auto makeBtn = [](const QString& text, const QString& color) {
        auto* btn = new QPushButton(text); btn->setMinimumHeight(42); btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral("QPushButton { background-color: %1; color: white; border-radius: 6px; font-size: 13px; font-weight: bold; border: none; } QPushButton:hover { background-color: %2; } QPushButton:disabled { background-color: #94A3B8; }").arg(color, color));
        return btn;
    };
    // Unified industrial secondary style for config/export buttons
    QString secBg  = "#475569";
    QString secHov = "#334155";
    m_randomBtn = makeBtn(QStringLiteral("\u968F\u673A\u751F\u6210\u65B0\u73B0\u573A"), secBg);
    m_randomBtn->setStyleSheet(QStringLiteral("QPushButton { background-color: %1; color: white; border-radius: 6px; font-size: 13px; font-weight: bold; border: none; } QPushButton:hover { background-color: %2; } QPushButton:disabled { background-color: #94A3B8; }").arg(secBg, secHov));
    m_pdfBtn    = makeBtn(QStringLiteral("\u751F\u6210\u73B0\u573A\u6D3E\u5DE5\u5355PDF"), secBg);
    m_pdfBtn->setStyleSheet(m_randomBtn->styleSheet());
    m_exportBtn = makeBtn(QStringLiteral("\u5BFC\u51FA\u6807\u5B9A\u6570\u636E"), secBg);
    m_exportBtn->setStyleSheet(m_randomBtn->styleSheet());
    connect(m_randomBtn, &QPushButton::clicked, this, &MainWindow::onRandomGenerate);
    connect(m_pdfBtn,    &QPushButton::clicked, this, &MainWindow::onGeneratePDF);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::onExportData);
    layout->addWidget(m_randomBtn); layout->addWidget(m_pdfBtn); layout->addWidget(m_exportBtn);
    layout->addStretch();
    // Start/Stop segmented pair
    auto* simRow = new QHBoxLayout();
    simRow->setSpacing(0);
    m_startBtn = new QPushButton(QStringLiteral("\u5F00\u59CB\u6A21\u62DF"));
    m_startBtn->setMinimumHeight(42); m_startBtn->setCursor(Qt::PointingHandCursor);
    m_startBtn->setStyleSheet("QPushButton { background-color:#10b981;color:white;font-weight:bold;font-size:13px;border:none;border-radius:6px 0 0 6px;min-height:42px; } QPushButton:hover { background-color:#059669; } QPushButton:disabled { background-color:#6B7280;color:#D1D5DB; }");
    simRow->addWidget(m_startBtn, 1);
    m_stopBtn = new QPushButton(QStringLiteral("\u505C\u6B62\u6A21\u62DF"));
    m_stopBtn->setMinimumHeight(42); m_stopBtn->setCursor(Qt::PointingHandCursor);
    m_stopBtn->setStyleSheet("QPushButton { background-color:#EF4444;color:white;font-weight:bold;font-size:13px;border:none;border-radius:0 6px 6px 0;min-height:42px; } QPushButton:hover { background-color:#DC2626; } QPushButton:disabled { background-color:#6B7280;color:#D1D5DB; }");
    m_stopBtn->setEnabled(false);
    simRow->addWidget(m_stopBtn, 1);
    layout->addLayout(simRow);
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartSimulation);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStopSimulation);
}

void MainWindow::setupMainContent(QWidget* main) {
    auto* layout = new QVBoxLayout(main);
    layout->setContentsMargins(20, 12, 20, 12);
    layout->setSpacing(10);
    m_titleLabel = new QLabel(QStringLiteral("\u6E20\u6C9F\u6E05\u7406\u6807\u5B9A"));
    QFont ttf; ttf.setPointSize(22); ttf.setBold(true); m_titleLabel->setFont(ttf);
    m_titleLabel->setStyleSheet("color: #0F172A;"); layout->addWidget(m_titleLabel);
    m_subtitleLabel = new QLabel(); QFont sbf; sbf.setPointSize(11); m_subtitleLabel->setFont(sbf);
    m_subtitleLabel->setStyleSheet("color: #475569;"); layout->addWidget(m_subtitleLabel);
    m_alarmPanel = new AlarmPanelWidget(m_engine->alarmManager(), main);
    layout->addWidget(m_alarmPanel);

    auto* monitorRow = new QHBoxLayout();
    auto* waterGroup = new QGroupBox(QStringLiteral("\u6C34\u7BB1\u6DB2\u4F4D\u76D1\u63A7"));
    waterGroup->setStyleSheet("QGroupBox { font-weight:bold; font-size:13px; color:#0F172A; border:2px solid #CBD5E1; border-radius:6px; margin-top:14px; padding-top:18px; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 6px; }");
    auto* waterLayout = new QVBoxLayout(waterGroup);
    m_waterBar = new QProgressBar(); m_waterBar->setRange(0, 100); m_waterBar->setValue(100);
    m_waterBar->setTextVisible(true); m_waterBar->setFormat("%p%"); m_waterBar->setMinimumHeight(28);
    m_waterBar->setStyleSheet("QProgressBar { border:1px solid #CBD5E1; border-radius:4px; text-align:center; font-weight:bold; } QProgressBar::chunk { background-color:#0284C7; border-radius:4px; }");
    waterLayout->addWidget(m_waterBar);
    m_waterCapLabel = new QLabel(); m_waterCapLabel->setStyleSheet("font-size:13px; font-weight:bold; color:#0F172A;");
    waterLayout->addWidget(m_waterCapLabel);
    auto* metricGrid = new QGridLayout();
    auto makeMetric = [](const QString& label) {
        auto* lbl = new QLabel(); lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("font-size:20px; font-weight:bold; color:#0369A1;");
        auto* title = new QLabel(label); title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-size:12px; font-weight:bold; color:#475569;");
        return std::make_pair(lbl, title);
    };
    auto [flowL, flowT] = makeMetric(QStringLiteral("\u51FA\u6C34\u6D41\u91CF"));
    auto [rtL, rtT]     = makeMetric(QStringLiteral("\u5269\u4F59\u65F6\u95F4"));
    auto [rdL, rdT]     = makeMetric(QStringLiteral("\u9884\u4F30\u5269\u4F59\u6E05\u6D17"));
    auto [cpL, cpT]     = makeMetric(QStringLiteral("\u5B8C\u6210\u7387"));
    m_flowLabel = flowL; m_remainTimeLabel = rtL; m_remainDistLabel = rdL; m_completionLabel = cpL;
    metricGrid->addWidget(flowL, 0, 0); metricGrid->addWidget(flowT, 1, 0);
    metricGrid->addWidget(rtL,  0, 1);  metricGrid->addWidget(rtT,  1, 1);
    metricGrid->addWidget(rdL,  0, 2);  metricGrid->addWidget(rdT,  1, 2);
    metricGrid->addWidget(cpL,  0, 3);  metricGrid->addWidget(cpT,  1, 3);
    waterLayout->addLayout(metricGrid);
    monitorRow->addWidget(waterGroup, 3);
    auto* headGroup = new QGroupBox(QStringLiteral("\u6E05\u6D17\u5934\u72B6\u6001"));
    headGroup->setStyleSheet(waterGroup->styleSheet());
    auto* headLayout = new QVBoxLayout(headGroup);
    m_headStatusFrame = new QWidget(); m_headStatusFrame->setMinimumHeight(80);
    m_headStatusFrame->setStyleSheet("background-color:#22c55e; border-radius:6px;");
    auto* hsfLayout = new QVBoxLayout(m_headStatusFrame);
    m_headStatusLabel = new QLabel(QStringLiteral("\u72B6\u6001: \u6B63\u5E38"));
    m_headStatusLabel->setAlignment(Qt::AlignCenter);
    m_headStatusLabel->setStyleSheet("color:white; font-size:16px; font-weight:bold;");
    hsfLayout->addWidget(m_headStatusLabel);
    m_headPressLabel = new QLabel(QStringLiteral("\u6C34\u538B: 150 Bar"));
    m_headPressLabel->setAlignment(Qt::AlignCenter);
    m_headPressLabel->setStyleSheet("color:white; font-size:14px;"); hsfLayout->addWidget(m_headPressLabel);
    headLayout->addWidget(m_headStatusFrame); headLayout->addStretch();
    monitorRow->addWidget(headGroup, 1); layout->addLayout(monitorRow);

    auto* filterRow = new QHBoxLayout();
    auto* filterLabel = new QLabel(QStringLiteral("\u663E\u793A\u6A21\u5F0F:"));
    filterLabel->setStyleSheet("font-weight:bold; color:#0F172A;"); filterRow->addWidget(filterLabel);
    m_filterGroup = new QButtonGroup(this);
    QStringList filterNames = { QStringLiteral("\u663E\u793A\u5168\u90E8"), QStringLiteral("\u4EC5\u770B\u91CD\u5EA6"), QStringLiteral("\u4EC5\u770B\u4E2D\u5EA6"), QStringLiteral("\u4EC5\u770B\u8F7B\u5EA6") };
    for (int i = 0; i < filterNames.size(); ++i) {
        auto* rb = new QRadioButton(filterNames[i]); rb->setStyleSheet("font-size:15px; font-weight:bold; color:#0F172A;");
        m_filterGroup->addButton(rb, i); filterRow->addWidget(rb); if (i == 0) rb->setChecked(true);
    }
    m_filterCount = new QLabel(); m_filterCount->setStyleSheet("color:#475569; font-size:12px; margin-left:8px;");
    filterRow->addWidget(m_filterCount); filterRow->addStretch();
    connect(m_filterGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &MainWindow::onFilterChanged);
    layout->addLayout(filterRow);

    m_channelMap = new ChannelMapWidget(); m_channelMap->setMinimumHeight(550);
    layout->addWidget(m_channelMap);
    layout->setSpacing(8);

    // ── Camera Monitor Panel (1st) ───────────────────────────────
    auto* camPanel = createCollapsiblePanel(
        QStringLiteral("\u5C55\u5F00\u67E5\u770B: \u6E05\u6D17\u5934\u5B9E\u65F6\u76D1\u63A7"), main);
    auto* camContent = panelContent(camPanel);
    auto* camLayout = new QVBoxLayout(camContent);
    m_cameraMonitor = new CameraMonitorWidget(m_cameraService, camContent);
    camLayout->addWidget(m_cameraMonitor);
    layout->addWidget(camPanel);

    // Collapsible: Water Resource & Time Estimate
    auto* resPanel = createCollapsiblePanel(QStringLiteral("\u5C55\u5F00\u67E5\u770B: \u6C34\u8D44\u6E90\u4E0E\u5DE5\u65F6\u9884\u4F30"), main);
    auto* resContent = panelContent(resPanel);
    auto* resGrid = new QGridLayout(resContent);
    auto mkResLabel = []() { auto* l = new QLabel(); l->setStyleSheet("font-size:20px; font-weight:bold; color:#0369A1;"); l->setAlignment(Qt::AlignCenter); return l; };
    auto mkResTitle = [](const QString& t) { auto* l = new QLabel(t); l->setStyleSheet("font-size:12px; font-weight:bold; color:#475569;"); l->setAlignment(Qt::AlignCenter); return l; };
    m_totalLenLabel = mkResLabel(); m_obsCountLabel = mkResLabel(); m_blockRateLabel = mkResLabel();
    m_heavyCountLabel = mkResLabel(); m_totalWaterLabel = mkResLabel(); m_totalLaborLabel = mkResLabel();
    resGrid->addWidget(m_totalLenLabel, 0, 0); resGrid->addWidget(mkResTitle(QStringLiteral("\u6C9F\u6E20\u603B\u957F")), 1, 0);
    resGrid->addWidget(m_obsCountLabel, 0, 1); resGrid->addWidget(mkResTitle(QStringLiteral("\u969C\u7887\u7269")), 1, 1);
    resGrid->addWidget(m_blockRateLabel, 0, 2); resGrid->addWidget(mkResTitle(QStringLiteral("\u5408\u5E76\u5835\u585E\u7387")), 1, 2);
    resGrid->addWidget(m_heavyCountLabel, 2, 0); resGrid->addWidget(mkResTitle(QStringLiteral("\u91CD\u5EA6\u533A\u6BB5")), 3, 0);
    resGrid->addWidget(m_totalWaterLabel, 2, 1); resGrid->addWidget(mkResTitle(QStringLiteral("\u603B\u6C34\u8017")), 3, 1);
    resGrid->addWidget(m_totalLaborLabel, 2, 2); resGrid->addWidget(mkResTitle(QStringLiteral("\u603B\u5DE5\u65F6")), 3, 2);
    layout->addWidget(resPanel);

    // Collapsible: Work Order
    auto* woPanel = createCollapsiblePanel(QStringLiteral("\u5C55\u5F00\u67E5\u770B: \u73B0\u573A\u4F5C\u4E1A\u6D3E\u5DE5\u5355"), main);
    auto* woContent = panelContent(woPanel);
    auto* woLayout2 = new QVBoxLayout(woContent);
    m_workOrderText = new QTextEdit(); m_workOrderText->setReadOnly(true);
    m_workOrderText->setStyleSheet("font-family:Consolas,monospace; font-size:12px; background:#F8FAFC;"); m_workOrderText->setMinimumHeight(120);
    woLayout2->addWidget(m_workOrderText);
    layout->addWidget(woPanel);

    // Collapsible: Obstacle Data Detail
    auto* dataPanel = createCollapsiblePanel(QStringLiteral("\u5C55\u5F00\u67E5\u770B: \u969C\u7887\u7269\u6570\u636E\u660E\u7EC6"), main);
    auto* dataContent = panelContent(dataPanel);
    auto* dataLayout2 = new QVBoxLayout(dataContent);
    m_obstacleModel = new ObstacleTableModel(this);
    m_dataTable = new QTableView(); m_dataTable->setModel(m_obstacleModel);
    m_dataTable->setSelectionBehavior(QAbstractItemView::SelectRows); m_dataTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_dataTable->horizontalHeader()->setStretchLastSection(true); m_dataTable->setAlternatingRowColors(true);
    m_dataTable->setStyleSheet("QTableView { gridline-color:#E2E8F0; } QTableView::item:selected { background-color:#0284C7; color:white; }");
    m_dataTable->setItemDelegateForColumn(ObstacleTableModel::ColType, m_obstacleModel->typeDelegate());
    m_dataTable->setItemDelegateForColumn(ObstacleTableModel::ColSeverity, m_obstacleModel->severityDelegate());
    dataLayout2->addWidget(m_dataTable);
    auto* btnRow = new QHBoxLayout();
    m_addBtn = new QPushButton(QStringLiteral("\u6DFB\u52A0")); m_delBtn = new QPushButton(QStringLiteral("\u5220\u9664"));
    m_undoBtn = new QPushButton(QStringLiteral("\u4E0A\u4E00\u6B65")); m_redoBtn = new QPushButton(QStringLiteral("\u4E0B\u4E00\u6B65"));
    for (auto* b : {m_addBtn, m_delBtn, m_undoBtn, m_redoBtn}) {
        b->setStyleSheet("background-color:#0284C7;color:white;font-weight:bold;border-radius:4px;min-height:32px;"); b->setCursor(Qt::PointingHandCursor);
    }
    btnRow->addWidget(m_addBtn); btnRow->addWidget(m_delBtn); btnRow->addWidget(m_undoBtn); btnRow->addWidget(m_redoBtn); btnRow->addStretch();
    dataLayout2->addLayout(btnRow);
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddObstacle);
    connect(m_delBtn, &QPushButton::clicked, this, &MainWindow::onDeleteSelected);
    connect(m_undoBtn, &QPushButton::clicked, this, &MainWindow::onUndo);
    connect(m_redoBtn, &QPushButton::clicked, this, &MainWindow::onRedo);
    connect(m_obstacleModel, &ObstacleTableModel::dataModified, this, &MainWindow::onObstacleDataModified);
    layout->addWidget(dataPanel);

    // Collapsible: Work Log
    auto* logPanel = createCollapsiblePanel(QStringLiteral("\u5DE5\u4F5C\u65E5\u5FD7"), main);
    auto* logContent = panelContent(logPanel);
    auto* logLayout2 = new QVBoxLayout(logContent);
    m_workLogModel = new WorkLogTableModel(this);
    m_logTable = new QTableView(); m_logTable->setModel(m_workLogModel);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows); m_logTable->horizontalHeader()->setStretchLastSection(true);
    m_logTable->setAlternatingRowColors(true); m_logTable->setStyleSheet(m_dataTable->styleSheet());
    logLayout2->addWidget(m_logTable);
    layout->addWidget(logPanel);
}

void MainWindow::onStateUpdated() {
    QMutexLocker lock(&m_engine->deviceState().mutex);
    const auto& s = m_engine->deviceState();
    double pct = s.waterPercent() * 100.0;
    m_waterBar->setValue(static_cast<int>(pct));
    m_waterCapLabel->setText(QStringLiteral("\u5F53\u524D\u5BB9\u91CF: %1 L / 5000 L (%2%)").arg(s.currentWater, 0, 'f', 0).arg(pct, 0, 'f', 1));
    m_flowLabel->setText(QStringLiteral("120 L/min"));
    m_remainTimeLabel->setText(QStringLiteral("%1 min").arg(s.remainingTime(), 0, 'f', 1));
    m_remainDistLabel->setText(QStringLiteral("%1 m").arg(s.estimatedRemainingDistance(), 0, 'f', 1));
    m_completionLabel->setText(QStringLiteral("%1%").arg(s.completionPercent(), 0, 'f', 1));
    QString hsText = headStatusString(s.headStatus);
    m_headStatusLabel->setText(QStringLiteral("\u72B6\u6001: %1").arg(hsText));
    m_headPressLabel->setText(QStringLiteral("\u6C34\u538B: %1 Bar").arg(s.waterPressure, 0, 'f', 0));
    QString frameColor;
    switch (s.headStatus) { case HeadStatus::NORMAL: frameColor="#22c55e"; break; case HeadStatus::WARNING: frameColor="#f59e0b"; break; case HeadStatus::DAMAGE: frameColor="#ef4444"; break; case HeadStatus::STOPPED: frameColor="#6b7280"; break; }
    m_headStatusFrame->setStyleSheet(QStringLiteral("background-color:%1; border-radius:6px;").arg(frameColor));
    double chanLen = s.channelLength; double headPos = s.currentPosition;
    QVector<Obstacle> obsCopy = s.obstacles; double compPct = s.completionPercent(); double curWater = s.currentWater;
    QString chanId = s.channelId; QString opName = s.operatorName; QString inspDate = s.inspectDate.toString("yyyy-MM-dd");
    lock.unlock();
    m_channelMap->setChannelLength(chanLen); m_channelMap->setHeadPosition(headPos); m_channelMap->setObstacles(obsCopy);
    m_filterCount->setText(QStringLiteral("\u663E\u793A %1 / %2 \u6761").arg(m_channelMap->filteredCount()).arg(m_channelMap->totalCount()));
    auto report = m_engine->computeReport();
    m_totalLenLabel->setText(QStringLiteral("%1 m").arg(chanLen, 0, 'f', 1));
    m_obsCountLabel->setText(QStringLiteral("%1 \u4E2A").arg(report.obstacleCount));
    m_blockRateLabel->setText(QStringLiteral("%1%").arg(report.blockageRate, 0, 'f', 1));
    m_heavyCountLabel->setText(QStringLiteral("%1 \u5904").arg(report.heavyCount));
    m_totalWaterLabel->setText(QStringLiteral("%1 L").arg(report.totalWater, 0, 'f', 0));
    double laborMin = report.totalLabor; QString laborStr;
    if (laborMin >= 60) laborStr = QStringLiteral("%1h%2min").arg(static_cast<int>(laborMin/60)).arg(static_cast<int>(laborMin) % 60);
    else laborStr = QStringLiteral("%1 min").arg(static_cast<int>(laborMin));
    m_totalLaborLabel->setText(laborStr);
    m_workOrderText->setText(m_engine->generateWorkOrder(obsCopy));
    m_subtitleLabel->setText(QStringLiteral("%1 | %2 | %3").arg(chanId, opName, inspDate));
    static bool s_wasComplete = false;
    if (compPct >= 99.9 && curWater > 0 && !s_wasComplete) {
        s_wasComplete = true;
        WorkLogEntry entry; entry.taskId = QStringLiteral("WO-%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
        entry.operatorName = opName; entry.dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        entry.channelId = chanId; entry.channelLength = chanLen; entry.cleanedLength = headPos;
        entry.startTime = m_engine->deviceState().startTime.toString("yyyy-MM-dd HH:mm:ss");
        entry.endTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"); entry.totalWater = report.totalWater;
        entry.avgFlowRate = 120.0; entry.maxPressure = 200.0; entry.obstacleCount = report.obstacleCount;
        entry.maxSeverity = (report.heavyCount > 0) ? QStringLiteral("\u91CD\u5EA6") : QStringLiteral("\u4E2D\u5EA6");
        entry.deviceStatus = QStringLiteral("\u5B8C\u6210"); entry.result = QStringLiteral("\u6E05\u6D17\u5B8C\u6210");
        m_workLogModel->appendLog(entry);
    }
    if (compPct < 90.0) s_wasComplete = false;
    if (m_cameraMonitor) {
        SimulationCamera::Telemetry t;
        QMutexLocker lock(&m_engine->deviceState().mutex);
        const auto& s = m_engine->deviceState();
        t.pos      = s.currentPosition;
        t.pressure = s.waterPressure;
        t.flow     = s.flowRate;
        t.headSev  = static_cast<int>(s.headStatus);
        lock.unlock();
        m_cameraService->setTelemetry(t);
        m_cameraMonitor->updateTelemetry(t);
    }
}

void MainWindow::onAlarmUpdated(const AlarmEvent&) { m_alarmPanel->refreshAlarms(); }

void MainWindow::onRandomGenerate() {
    static const QStringList cids = { QStringLiteral("B3\u6BB5\u6392\u6C34\u6C9F"), QStringLiteral("C2\u4E3B\u5E72\u9053"), QStringLiteral("AA\u652F\u6E20") };
    static const QStringList ops = { QStringLiteral("\u5F20\u5DE5"), QStringLiteral("\u674E\u5DE5"), QStringLiteral("\u738B\u5DE5") };
    std::random_device rd; std::mt19937 gen(rd());
    std::uniform_int_distribution<int> cidDist(0, cids.size()-1), lenDist(0, 3), opDist(0, ops.size()-1), nDist(2, 5);
    std::uniform_real_distribution<double> posDist(0, 1);
    QString cid = cids[cidDist(gen)]; double lens[] = {60, 80, 100, 120}; double tl = lens[lenDist(gen)];
    QString op = ops[opDist(gen)]; int n = nDist(gen);
    QVector<Obstacle> obs;
    for (int i = 0; i < n; ++i) {
        Obstacle o; o.id = i + 1;
        o.type = DeviceConfig::OBSTACLE_TYPES[std::uniform_int_distribution<int>(0, 5)(gen)];
        o.severity = DeviceConfig::SEVERITY_LEVELS[std::uniform_int_distribution<int>(0, 2)(gen)];
        o.start_m = std::round(posDist(gen) * (tl - 5) * 10.0) / 10.0;
        o.end_m = std::min(tl, std::round((o.start_m + posDist(gen) * 4.0 + 1.5) * 10.0) / 10.0);
        o.notes = QStringLiteral("\u968F\u673A"); obs.append(o);
    }
    std::sort(obs.begin(), obs.end(), [](const Obstacle& a, const Obstacle& b) { return a.start_m < b.start_m; });
    m_channelIdEdit->setText(cid); m_channelLenSpin->setValue(tl);
    m_operatorEdit->setText(op); m_inspectDateEdit->setDate(QDate::currentDate());
    m_engine->stop(); m_engine->reset(); m_engine->setChannelInfo(cid, tl, op); m_engine->setObstacles(obs);
    if (m_sensor) m_sensor->setChannelLength(tl);
    m_obstacleModel->setObstacles(obs); pushHistory();
    m_startBtn->setEnabled(true); m_stopBtn->setEnabled(false); refreshAll();
}

void MainWindow::onGeneratePDF() {
    QMessageBox::information(this, QStringLiteral("PDF\u6D3E\u5DE5\u5355"), QStringLiteral("PDF\u751F\u6210\u529F\u80FD\u5C06\u5728\u540E\u7EED\u7248\u672C\u4E2D\u5B8C\u6210\u3002\n\u5F53\u524D\u8BF7\u4F7F\u7528\u5BFC\u51FA\u6570\u636E\u529F\u80FD\u3002"));
}

void MainWindow::onExportData() {
    QString fileName = QFileDialog::getSaveFileName(this, QStringLiteral("\u5BFC\u51FA\u6807\u5B9A\u6570\u636E"), QStringLiteral("\u6807\u5B9A_%1.json").arg(m_engine->deviceState().channelId), QStringLiteral("JSON (*.json)"));
    if (fileName.isEmpty()) return;
    QJsonObject root; root["info"] = m_engine->deviceState().infoJson(); root["obstacles"] = m_engine->deviceState().obstaclesJson();
    QJsonDocument doc(root); QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) { file.write(doc.toJson(QJsonDocument::Indented)); file.close();
        QMessageBox::information(this, QStringLiteral("\u5BFC\u51FA\u6210\u529F"), QStringLiteral("\u6570\u636E\u5DF2\u5BFC\u51FA\u5230:\n%1").arg(fileName)); }
}

void MainWindow::onFilterChanged(int mode) { m_channelMap->setDisplayFilter(mode);
    m_filterCount->setText(QStringLiteral("\u663E\u793A %1 / %2 \u6761").arg(m_channelMap->filteredCount()).arg(m_channelMap->totalCount())); }

void MainWindow::onAddObstacle() { m_obstacleModel->addRow(); pushHistory(); applyObstaclesToEngine(); refreshAll(); }

void MainWindow::onDeleteSelected() {
    auto sel = m_dataTable->selectionModel()->selectedRows(); if (sel.isEmpty()) return;
    QVector<int> rows; for (const auto& idx : sel) rows.append(idx.row());
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows) m_obstacleModel->removeRow(r);
    pushHistory(); applyObstaclesToEngine(); refreshAll();
}

void MainWindow::onUndo() { if (m_historyPos <= 0) return; m_historyPos--;
    m_obstacleModel->setObstacles(m_history[m_historyPos]); applyObstaclesToEngine(); refreshAll(); }

void MainWindow::onRedo() { if (m_historyPos >= m_history.size() - 1) return; m_historyPos++;
    m_obstacleModel->setObstacles(m_history[m_historyPos]); applyObstaclesToEngine(); refreshAll(); }

void MainWindow::onObstacleDataModified() { pushHistory(); applyObstaclesToEngine(); refreshAll(); }

void MainWindow::onStartSimulation() {
    m_engine->start();
    m_cameraService->start();
    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
}

void MainWindow::onStopSimulation() {
    m_engine->stop();
    m_cameraService->stop();
    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
}

void MainWindow::onChannelInfoChanged() {
    QString cid = m_channelIdEdit->text(); double len = m_channelLenSpin->value();
    QString op = m_operatorEdit->text(); QDate dt = m_inspectDateEdit->date();
    m_engine->setChannelInfo(cid, len, op);
    { QMutexLocker lock(&m_engine->deviceState().mutex); m_engine->deviceState().inspectDate = dt; }
    if (m_sensor) m_sensor->setChannelLength(len); refreshAll();
}

void MainWindow::pushHistory() {
    while (m_history.size() > m_historyPos + 1) m_history.removeLast();
    m_history.append(m_obstacleModel->obstacles()); m_historyPos = m_history.size() - 1;
    if (m_history.size() > 100) { m_history.removeFirst(); m_historyPos--; }
    m_undoBtn->setEnabled(m_historyPos > 0); m_redoBtn->setEnabled(m_historyPos < m_history.size() - 1);
}

void MainWindow::applyObstaclesToEngine() { m_engine->setObstacles(m_obstacleModel->obstacles()); }

void MainWindow::refreshAll() { onStateUpdated(); auto ev = m_engine->currentAlarm(); onAlarmUpdated(ev); m_alarmPanel->refreshAlarms(); }
