// =============================================================================
// SimulationEngine.cpp — Core business logic
// =============================================================================
#include "SimulationEngine.h"
#include "../config/DeviceConfig.h"
#include <algorithm>

SimulationEngine::SimulationEngine(QObject* parent)
    : QObject(parent)
{
    m_stateMachine = new DeviceStateMachine(this);
    m_alarmManager = new AlarmManager(this);

    m_stateTimer = new QTimer(this);
    m_stateTimer->setInterval(DeviceConfig::SIM_TICK_MS);
    QObject::connect(m_stateTimer, &QTimer::timeout, this, &SimulationEngine::onSensorData);

    QObject::connect(m_stateMachine, &DeviceStateMachine::statusChanged,
                     [this](HeadStatus) { evaluateAlarms(); });
    QObject::connect(m_alarmManager, &AlarmManager::alarmsChanged,
                     [this]() { emit alarmUpdated(m_alarmManager->latestBannerAlarm()); });

    m_state.startTime = QDateTime::currentDateTime();
}

SimulationEngine::~SimulationEngine() {
    stop();
}

void SimulationEngine::setSensor(SensorInterface* sensor) {
    if (m_sensor) {
        QObject::disconnect(m_sensor, &SensorInterface::dataUpdated,
                            this, &SimulationEngine::onSensorData);
    }
    m_sensor = sensor;
    if (m_sensor) {
        QObject::connect(m_sensor, &SensorInterface::dataUpdated,
                         this, &SimulationEngine::onSensorData);
    }
}

void SimulationEngine::start() {
    if (m_sensor && !m_sensor->isConnected())
        m_sensor->connect();
    m_stateTimer->start();
    m_state.startTime = QDateTime::currentDateTime();
    emit simulationStarted();
}

void SimulationEngine::stop() {
    m_stateTimer->stop();
    if (m_sensor) m_sensor->disconnect();
    emit simulationStopped();
}

void SimulationEngine::reset() {
    stop();
    // Cannot assign DeviceState (QMutex is non-copyable), reset fields manually
    m_state.channelId        = QStringLiteral("A1\u8DEF\u6BB5\u4E3B\u6C9F\u6E20");
    m_state.channelLength    = 100.0;
    m_state.operatorName     = QStringLiteral("\u5F20\u5DE5\u7A0B\u5E08");
    m_state.inspectDate      = QDate::currentDate();
    m_state.obstacles.clear();
    m_state.currentPosition  = 0.0;
    m_state.currentWater     = 5000.0;
    m_state.waterPressure    = 150.0;
    m_state.headStatus       = HeadStatus::NORMAL;
    m_state.flowRate         = 120.0;
    m_state.totalDistance    = 0.0;
    m_state.totalWaterUsed   = 0.0;
    m_state.taskProgress     = 0.0;
    m_state.startTime        = QDateTime::currentDateTime();
    m_state.updateTime       = QDateTime::currentDateTime();
    m_state.alarmStatus.clear();
    m_state.alarmLevel.clear();
    m_workLog.clear();
    m_taskCounter = 0;
}

void SimulationEngine::onSensorData() {
    if (!m_sensor) return;

    QMutexLocker lock(&m_state.mutex);
    m_state.currentPosition = m_sensor->getPosition();
    m_state.currentWater    = m_sensor->getWaterLevel();
    m_state.waterPressure   = m_sensor->getPressure();
    m_state.headStatus      = m_sensor->getHeadStatus();
    m_state.flowRate        = m_sensor->getFlowRate();
    m_state.totalWaterUsed  = DeviceConfig::MAX_WATER_CAPACITY - m_state.currentWater;
    m_state.totalDistance   = m_state.currentPosition;
    m_state.taskProgress    = m_state.completionPercent();
    m_state.updateTime      = QDateTime::currentDateTime();
    lock.unlock();

    // DAMAGE auto-stop: immediately halt simulation, prevent further operation
    if (m_state.headStatus == HeadStatus::DAMAGE) {
        stop();
    }

    m_stateMachine->evaluate(m_state);
    evaluateAlarms();
    emit stateUpdated();
}

void SimulationEngine::evaluateAlarms() {
    m_alarmManager->evaluate(m_state);
}

// --- Obstacle management ---
void SimulationEngine::setObstacles(const QVector<Obstacle>& obs) {
    QMutexLocker lock(&m_state.mutex);
    m_state.obstacles = obs;
}

void SimulationEngine::updateObstacle(int index, const Obstacle& obs) {
    QMutexLocker lock(&m_state.mutex);
    if (index >= 0 && index < m_state.obstacles.size())
        m_state.obstacles[index] = obs;
}

void SimulationEngine::removeObstacle(int index) {
    QMutexLocker lock(&m_state.mutex);
    if (index >= 0 && index < m_state.obstacles.size())
        m_state.obstacles.removeAt(index);
}

void SimulationEngine::addObstacle(const Obstacle& obs) {
    QMutexLocker lock(&m_state.mutex);
    m_state.obstacles.append(obs);
}

void SimulationEngine::setChannelInfo(const QString& id, double length, const QString& op) {
    QMutexLocker lock(&m_state.mutex);
    m_state.channelId     = id;
    m_state.channelLength = length;
    m_state.operatorName  = op;
}

// --- Business computations ---
SimulationEngine::BusinessReport SimulationEngine::computeReport() const {
    QMutexLocker lock(&m_state.mutex);
    BusinessReport r;
    const auto& obs = m_state.obstacles;
    r.obstacleCount = obs.size();

    // Merged interval blockage rate
    if (m_state.channelLength > 0 && !obs.isEmpty()) {
        struct Iv { double s, e; };
        QVector<Iv> ivs;
        for (const auto& o : obs) {
            if (o.end_m > o.start_m)
                ivs.append({o.start_m, o.end_m});
        }
        if (!ivs.isEmpty()) {
            std::sort(ivs.begin(), ivs.end(), [](const Iv& a, const Iv& b) { return a.s < b.s; });
            double covered = 0.0;
            double cs = ivs[0].s, ce = ivs[0].e;
            for (int i = 1; i < ivs.size(); ++i) {
                if (ivs[i].s <= ce) ce = std::max(ce, ivs[i].e);
                else { covered += (ce - cs); cs = ivs[i].s; ce = ivs[i].e; }
            }
            covered += (ce - cs);
            r.blockageRate = std::round(covered / m_state.channelLength * 1000.0) / 10.0;
        }
    }

    for (const auto& o : obs) {
        if (o.severity == QStringLiteral("\u91CD\u5EA6")) r.heavyCount++;
    }
    r.totalWater = totalWaterFor(obs);
    r.totalLabor = totalLaborFor(obs);
    return r;
}

double SimulationEngine::waterPerObstacle(const Obstacle& o) {
    double len = o.end_m - o.start_m;
    double rate = DeviceConfig::waterRate(o.severity);
    if (o.severity == QStringLiteral("\u91CD\u5EA6") &&
        (o.type == QStringLiteral("\u77F3\u5934") || o.type == QStringLiteral("\u6CE5\u5757")))
        rate *= DeviceConfig::STONE_BONUS;
    return std::round(len * rate * 10.0) / 10.0;
}

double SimulationEngine::laborPerObstacle(const Obstacle& o) {
    double len = o.end_m - o.start_m;
    return std::round(len * DeviceConfig::laborRate(o.severity) * 10.0) / 10.0;
}

double SimulationEngine::totalWaterFor(const QVector<Obstacle>& obs) {
    double total = 0.0;
    for (const auto& o : obs) total += waterPerObstacle(o);
    return std::round(total * 10.0) / 10.0;
}

double SimulationEngine::totalLaborFor(const QVector<Obstacle>& obs) {
    double total = DeviceConfig::SETUP_TIME;
    for (const auto& o : obs) total += laborPerObstacle(o);
    return std::round(total * 10.0) / 10.0;
}

QString SimulationEngine::generateWorkOrder(const QVector<Obstacle>& obs) const {
    if (obs.isEmpty()) return QStringLiteral("\u65E0\u4F5C\u4E1A\u4EFB\u52A1"); // 无作业任务
    QVector<Obstacle> sorted = obs;
    std::sort(sorted.begin(), sorted.end(),
              [](const Obstacle& a, const Obstacle& b) { return a.start_m < b.start_m; });
    QStringList lines;
    for (int i = 0; i < sorted.size(); ++i) {
        const auto& o = sorted[i];
        QString press = DeviceConfig::pressureSuggestion(o.severity, o.type);
        double w = waterPerObstacle(o);
        double lb = laborPerObstacle(o);
        lines.append(QStringLiteral("\u3010\u4EFB\u52A1%1\u3011%2m~%3m|%4(%5)->%6|\u6C34%7L|\u65F6%8min")
            .arg(i+1).arg(o.start_m,0,'f',1).arg(o.end_m,0,'f',1)
            .arg(o.type, o.severity, press).arg(w,0,'f',1).arg(lb,0,'f',1));
    }
    return lines.join("\n");
}

void SimulationEngine::recoverFromDamage() {
    m_alarmManager->clearAlarm(QStringLiteral("ALM-HEAD-001"));
    m_stateMachine->confirmDamageRecovery();
    // Re-enable simulation after damage recovery
    if (m_sensor) {
        m_sensor->connect();
    }
    m_stateTimer->start();
}

void SimulationEngine::logWorkEntry(const WorkLogEntry& entry) {
    m_workLog.append(entry);
}
