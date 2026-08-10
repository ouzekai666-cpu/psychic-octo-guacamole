// =============================================================================
// ChannelMapWidget.cpp — Transparent text, dynamic scene rect
// =============================================================================
#include "ChannelMapWidget.h"
#include "../config/DeviceConfig.h"
using namespace DeviceConfig::DigitalTwin;
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QPolygonF>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QResizeEvent>
#include <algorithm>
#include <cmath>

// ── Helper: rotated cleaning-head icon (QPainter transform pattern) ──────
// Paints the pixmap rotated -90° so the "head" faces right along the trajectory.
// Item position (setPos) represents the center of the icon.
class HeadIconItem : public QGraphicsItem
{
public:
    explicit HeadIconItem(const QPixmap& pm) : m_pm(pm) {
    }

    QRectF boundingRect() const override {
        double w = m_pm.width();
        double h = m_pm.height();
        return QRectF(-w/2.0, -h/2.0, w, h);
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->save();
        // (0,0) in item coords is the center of the icon (by setPos convention)
        // 1. Translate coordinate-system origin to the center of where the
        //    icon should be — identity here since (0,0) is already the center.
        painter->translate(0, 0);
        // 2. Rotate 90° CCW so the original downward-facing head faces right
        painter->rotate(-90);
        // 3. Draw pixmap centered at the rotated origin, swapping width/height
        //    so the visual proportions are correct after rotation.
        double w = m_pm.width();
        double h = m_pm.height();
        painter->drawPixmap(-h/2.0, -w/2.0, h, w, m_pm);
        painter->restore();
    }

private:
    QPixmap m_pm;
};

// ── Helper: concrete wall with CAD 45° hatching (clipRect prevents overflow) ──
class HatchedWallItem : public QGraphicsItem
{
public:
    HatchedWallItem(const QRectF& rect, const QColor& bg, const QColor& hatch, int spacing, int thk)
        : m_rect(rect), m_bg(bg), m_hatch(hatch), m_spacing(spacing), m_thk(thk) {}

    QRectF boundingRect() const override { return m_rect; }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->save();
        painter->setClipRect(m_rect);
        painter->fillRect(m_rect, m_bg);
        painter->setPen(QPen(m_hatch, 1));
        double xStart = m_rect.left() - m_thk;
        double xEnd   = m_rect.right() + m_thk;
        double top    = m_rect.top();
        double bot    = m_rect.bottom();
        for (double x = xStart; x < xEnd; x += m_spacing)
            painter->drawLine(QPointF(x, top), QPointF(x + m_thk, bot));
        painter->restore();
    }

private:
    QRectF  m_rect;
    QColor  m_bg, m_hatch;
    int     m_spacing, m_thk;
};

ChannelMapWidget::ChannelMapWidget(QWidget* parent) : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);  // anchor scene to top-left when smaller than viewport  // prevent vertical centering of scene in viewport
    setBackgroundBrush(QColor("#F8FAFC"));
    setFrameShape(QFrame::NoFrame);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto sz = CLEANING_HEAD_ICON_SIZE;
    m_headIcon = QPixmap(QStringLiteral(":/head_icon"));
    if (!m_headIcon.isNull()) {
        m_headIcon = m_headIcon.scaled(sz, sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        // Remove dark blue-gray background via color-distance chroma key
        QImage img = m_headIcon.toImage().convertToFormat(QImage::Format_ARGB32);
        const int bgR = 27, bgG = 34, bgB = 43;       // background key color
        const int thresholdSq = 225;                   // 15^2 — catches background (dist<15)
        for (int y = 0; y < img.height(); ++y) {
            for (int x = 0; x < img.width(); ++x) {
                QRgb p = img.pixel(x, y);
                int dr = qRed(p)   - bgR;
                int dg = qGreen(p) - bgG;
                int db = qBlue(p)  - bgB;
                int distSq = dr*dr + dg*dg + db*db;
                if (distSq < thresholdSq)
                    img.setPixelColor(x, y, QColor(0, 0, 0, 0));
            }
        }
        m_headIcon = QPixmap::fromImage(img);
    }

    m_ditchGroup      = new QGraphicsItemGroup();
    m_obstacleGroup   = new QGraphicsItemGroup();
    m_trajectoryGroup = new QGraphicsItemGroup();
    m_headGroup       = new QGraphicsItemGroup();
    m_labelGroup      = new QGraphicsItemGroup();

    m_scene->addItem(m_ditchGroup);      m_ditchGroup->setZValue(Z_DITCH);
    m_scene->addItem(m_obstacleGroup);   m_obstacleGroup->setZValue(Z_OBSTACLE);
    m_scene->addItem(m_trajectoryGroup); m_trajectoryGroup->setZValue(Z_TRAJECTORY);
    m_scene->addItem(m_headGroup);       m_headGroup->setZValue(Z_HEAD);
    m_scene->addItem(m_labelGroup);      m_labelGroup->setZValue(Z_LABEL);

    m_labelFont.setPointSize(13);
    m_labelFont.setBold(true);
    QFontMetrics fm(m_labelFont);
    m_labelFontPixelH = fm.height();
}

void ChannelMapWidget::setChannelLength(double l) {
    l = std::max(5.0, l);
    if (qFuzzyCompare(m_prevChanLen, l)) return;   // P0-2: skip unchanged
    m_prevChanLen = m_channelLength = l;
    rebuildScene();
}
void ChannelMapWidget::setObstacles(const QVector<Obstacle>& o) {
    // P0-2: fast checksum comparison — skip rebuild if data unchanged
    int cs = static_cast<int>(o.size());
    for (const auto& ob : o) cs ^= (ob.id * 31) ^ (static_cast<int>(ob.start_m * 10) * 37)
                                    ^ (static_cast<int>(ob.end_m * 10) * 41);
    if (cs == m_obsCheckSum && o.size() == m_obstacles.size()) return;
    m_obsCheckSum = cs;
    m_obstacles = o;
    rebuildScene();
}
void ChannelMapWidget::setHeadPosition(double p) {
    m_headPos = p;
    if (m_headItem) {
        double hx = CHART_LEFT_MARGIN_PX + m_headPos *
            ((viewport() ? viewport()->width() : 800.0) - 20.0 - CHART_LEFT_MARGIN_PX)
            / std::max(m_channelLength, 1.0);
        double trajY = 40.0 + 280.0 / 2.0;   // matches rebuildScene: y_trajAreaTop + trajAreaH/2
        m_headItem->setPos(hx, trajY);
        // Also update the head label
        auto kids = m_headGroup->childItems();
        for (auto* ki : kids) {
            if (auto* ti = qgraphicsitem_cast<QGraphicsTextItem*>(ki)) {
                ti->setPlainText(QStringLiteral("清洗头当前位置 %1m").arg(p,0,'f',1));
                double hlCx = hx, mapLeft = CHART_LEFT_MARGIN_PX;
                double hlMinCx = mapLeft + 4.0 + ti->boundingRect().width() / 2.0;
                if (hlCx < hlMinCx) hlCx = hlMinCx;
                ti->setPos(hlCx - ti->boundingRect().width() / 2.0, trajY - 96.0 / 2.0 - 8.0);
                break;
            }
        }
        return;
    }
    rebuildScene();   // first call — build full scene
}
void ChannelMapWidget::setDisplayFilter(int m)       { m_filterMode=m; rebuildScene(); }
void ChannelMapWidget::resizeEvent(QResizeEvent* e)  { QGraphicsView::resizeEvent(e); rebuildScene(); }

QSize ChannelMapWidget::sizeHint() const
{
    const int w = viewport() ? viewport()->width() : 800;
    int h = static_cast<int>(std::ceil(m_contentBottom));
    if (h < 1) h = minimumHeight();
    return QSize(w, h);
}

QSize ChannelMapWidget::minimumSizeHint() const
{
    return sizeHint();
}

// ---------------------------------------------------------------------------
// Merge overlapping obstacle intervals into groups
// ---------------------------------------------------------------------------
QVector<ChannelMapWidget::GroupedObstacle> ChannelMapWidget::groupOverlapping(
    const QVector<Obstacle>& raw) const
{
    if (raw.isEmpty()) return {};
    QVector<GroupedObstacle> groups;
    QVector<Obstacle> sorted = raw;
    std::sort(sorted.begin(), sorted.end(),
              [](const Obstacle& a, const Obstacle& b) { return a.start_m < b.start_m; });
    for (const auto& o : sorted) {
        double s = o.start_m, e = std::min(o.end_m, m_channelLength);
        if (s >= m_channelLength) continue;
        if (groups.isEmpty()) {
            GroupedObstacle g; g.start_m=s; g.end_m=e;
            g.typeNames.append(o.type); g.severities.append(o.severity);
            groups.append(g); continue;
        }
        auto& last = groups.last();
        if (s <= last.end_m + 0.1) {
            last.end_m = std::max(last.end_m, e);
            if (!last.typeNames.contains(o.type)) last.typeNames.append(o.type);
            if (!last.severities.contains(o.severity)) last.severities.append(o.severity);
        } else {
            GroupedObstacle g; g.start_m=s; g.end_m=e;
            g.typeNames.append(o.type); g.severities.append(o.severity);
            groups.append(g);
        }
    }
    return groups;
}

QString ChannelMapWidget::mergeTypeNames(const QStringList& names) {
    if (names.isEmpty()) return {};
    if (names.size() == 1) return names.first();
    return names.join(QStringLiteral(" | "));
}

QString ChannelMapWidget::mergeSeverityColor(const QStringList& sevs) {
    if (sevs.isEmpty()) return QStringLiteral("轻度");
    if (sevs.contains(QStringLiteral("重度"))) return QStringLiteral("重度");
    if (sevs.contains(QStringLiteral("中度"))) return QStringLiteral("中度");
    return QStringLiteral("轻度");
}

// ---------------------------------------------------------------------------
// Pre-compute type labels BELOW ditch, stagger UP only
// ---------------------------------------------------------------------------
static void computeTypeLabelLayouts(
    const QVector<ChannelMapWidget::GroupedObstacle>& groups,
    double mapLeft, double scaleX,
    double baseY, double ceilingY,
    double staggerDelta, double labelH,
    QVector<ChannelMapWidget::LabelLayout>& out)
{
    if (groups.isEmpty()) return;
    QFont f; f.setPointSize(13); f.setBold(true);
    QFontMetrics fm(f);
    QVector<ChannelMapWidget::LabelLayout> placed;

    for (const auto& g : groups) {
        double mid = (g.start_m+g.end_m)/2.0;
        double xc = mapLeft + mid*scaleX;
        QString text = ChannelMapWidget::mergeTypeNames(g.typeNames);
        int tw = fm.horizontalAdvance(text);

        ChannelMapWidget::LabelLayout ll;
        ll.x=xc; ll.w=static_cast<double>(tw); ll.h=labelH; ll.y=baseY;

        for (int a=0; a<12; ++a) {
            bool hit=false;
            for (const auto& p : placed) if (ll.overlaps(p)) {hit=true; break;}
            if (!hit) break;
            ll.y = baseY - (a+1)*staggerDelta;
            if (ll.y < ceilingY) { ll.y = ceilingY;
                for (const auto& p : placed) if (ll.overlaps(p)) { hit=true; break; }
                break;
            }
        }
        ll.y = std::max(ceilingY, std::min(ll.y, baseY));
        placed.append(ll); out.append(ll);
    }
}

// ---------------------------------------------------------------------------
// Pre-compute mileage labels ABOVE ditch, stagger UP
// ---------------------------------------------------------------------------
static void computeMileLabelLayouts(
    const QVector<ChannelMapWidget::GroupedObstacle>& groups,
    double mapLeft, double scaleX,
    double baseY, double staggerDelta, double labelH, double ceilingY,
    QVector<ChannelMapWidget::LabelLayout>& out)
{
    if (groups.isEmpty()) return;
    QFont f; f.setPointSize(13); f.setBold(true);
    QFontMetrics fm(f);
    QVector<ChannelMapWidget::LabelLayout> placed;

    for (const auto& g : groups) {
        double mid = (g.start_m+g.end_m)/2.0;
        double xc = mapLeft + mid*scaleX;
        QString text = QStringLiteral("%1-%2m").arg(g.start_m,0,'f',1).arg(g.end_m,0,'f',1);
        int tw = fm.horizontalAdvance(text);

        ChannelMapWidget::LabelLayout ll;
        ll.x=xc; ll.w=static_cast<double>(tw); ll.h=labelH; ll.y=baseY;

        for (int a=0; a<12; ++a) {
            bool hit=false;
            for (const auto& p : placed) if (ll.overlaps(p)) {hit=true; break;}
            if (!hit) break;
            ll.y = baseY - (a+1)*staggerDelta;
            if (ll.y < ceilingY) { ll.y = ceilingY; break; }
        }
        placed.append(ll); out.append(ll);
    }
}

// ============================================================================
// Draw a horizontal axis with ticks and number labels
// ============================================================================
// ---------------------------------------------------------------------------
// Obstacle type → color mapping
// ---------------------------------------------------------------------------
static QColor getObstacleColor(const QString& type) {
    if (type == QStringLiteral("垃圾")) return QColor("#1E293B");
    if (type == QStringLiteral("石头") || type == QStringLiteral("泥块")) return QColor("#EF4444");
    return QColor("#10B981");
}

static QColor typeColor(const QStringList& typeNames) {
    for (const auto& t : typeNames) {
        if (t == QStringLiteral("垃圾")) return getObstacleColor(t);
    }
    for (const auto& t : typeNames) {
        if (t == QStringLiteral("石头") || t == QStringLiteral("泥块")) return getObstacleColor(t);
    }
    return getObstacleColor(QString());
}

// ---------------------------------------------------------------------------
// Severity → height fraction (of ditchHeight)
// ---------------------------------------------------------------------------
static double severityHeightFrac(const QStringList& sevs) {
    if (sevs.contains(QStringLiteral("重度"))) return 1.0;
    if (sevs.contains(QStringLiteral("中度"))) return 2.0 / 3.0;
    return 1.0 / 3.0;
}
void ChannelMapWidget::drawXAxis(double yLine, double yLabel, double ml, double sx) {
    auto toXloc = [&](double m) { return ml + m * sx; };
    auto* axisLine = new QGraphicsLineItem(toXloc(0), yLine, toXloc(m_channelLength), yLine);
    axisLine->setPen(QPen(QColor("#64748B"), 1));
    m_ditchGroup->addToGroup(axisLine);

    double ti = std::max(5.0, std::round(m_channelLength/10.0));
    for (double m = 0; m <= m_channelLength + 0.001; m += ti) {
        double tx = toXloc(m);
        auto* tick = new QGraphicsLineItem(tx, yLine-5, tx, yLine+5);
        tick->setPen(QPen(QColor("#64748B"),1));
        m_ditchGroup->addToGroup(tick);
        auto* tkl = new QGraphicsTextItem(QString::number(static_cast<int>(m)));
        tkl->setDefaultTextColor(QColor("#475569"));
        QFont tkf; tkf.setPointSize(11); tkl->setFont(tkf);
        tkl->setPos(tx - tkl->boundingRect().width()/2.0, yLabel);
        m_labelGroup->addToGroup(tkl);
    }
}

// ============================================================================
// rebuildScene()
// ============================================================================
void ChannelMapWidget::rebuildScene() {
    for (auto* g : {m_ditchGroup,m_obstacleGroup,m_trajectoryGroup,m_headGroup,m_labelGroup})
        for (auto* item : g->childItems()) delete item;
    m_headItem = nullptr;   // P0-1: all head items deleted above, reset cache pointer

    double vpW = viewport() ? viewport()->width()  : 800.0;
    double vpH = viewport() ? viewport()->height() : 400.0;

    // ── Scene geometry ──────────────────────────────────────────────────
    // Use a generous initial scene rect for layout calculation;
    // the FINAL scene rect will be computed from content bounds.
    const double mapLeft   = CHART_LEFT_MARGIN_PX;
    const double mapRight  = vpW - 20.0;   // baseline full-width layout (unchanged)
    const double mapWidth  = mapRight - mapLeft;
    const double scaleX    = (m_channelLength>0) ? mapWidth/m_channelLength : 1.0;
    auto toX = [&](double m) { return mapLeft + m*scaleX; };

                                        // ── Y-band positions (280px traj, 40px gap) ─────────────────────────
    const double ditchH = 85.0;
    const double trajAreaH = 280.0;
    const double iSz = CLEANING_HEAD_ICON_SIZE;

    // 1. Title at Y=10
    const double y_mileBase = 40.0;               // y_trajTop = Y=40

    // 2. Trajectory area (280px)
    const double y_trajAreaTop   = y_mileBase;     // 40
    const double y_traj          = y_trajAreaTop + trajAreaH / 2.0;  // 180
    const double y_headLabel     = y_traj - iSz / 2.0 - 8.0;         // ~174
    const double y_trajAreaBot   = y_trajAreaTop + trajAreaH;         // 320

    // 3. Upper X-axis — tick labels at axis-14 (inside traj area)
    const double y_upperAxis     = y_trajAreaBot + 5.0;               // 325
    const double y_upperTick     = y_upperAxis - 22.0;                // 411

    // 4. Ditch area (mileage labels above)
    const double y_ditchTop      = y_upperAxis + 80.0;                // 405
    const double y_mileBase_actual = y_ditchTop - 25.0;              // 380
    const double y_ditchBottom   = y_ditchTop + ditchH;               // 490
    const double y_channelTop    = y_ditchTop;
    const double y_channelBottom = y_ditchBottom;

    // 5. Lower X-axis
    const double y_lowerAxis     = y_ditchBottom + 5.0;               // 495
    const double y_lowerTick     = y_lowerAxis + 18.0;                // 513
    const double y_axisTitle     = y_lowerAxis + 35.0;                // 530
// ── 1. Ditch (Z=0) ──────────────────────────────────────────────────
    auto* ditch = new QGraphicsRectItem(toX(0),y_channelTop,
        toX(m_channelLength)-toX(0), y_channelBottom-y_channelTop);
    ditch->setBrush(QColor("#E2E8F0")); ditch->setPen(QPen(QColor("#94A3B8"),2));
    ditch->setOpacity(0.9); m_ditchGroup->addToGroup(ditch);
    // ── 1b. Severity reference dashed lines inside ditch ────────────────
    double ditchHRef = y_channelBottom - y_channelTop;
    QPen dashPenRef(QColor(160, 175, 200, 180), 1.5, Qt::DashLine);
    double y33 = y_channelBottom - ditchHRef / 3.0;
    double y66 = y_channelBottom - ditchHRef * 2.0 / 3.0;
    auto* ref33 = new QGraphicsLineItem(toX(0), y33, toX(m_channelLength), y33);
    ref33->setPen(dashPenRef); m_ditchGroup->addToGroup(ref33);
    auto* ref66 = new QGraphicsLineItem(toX(0), y66, toX(m_channelLength), y66);
    ref66->setPen(dashPenRef); m_ditchGroup->addToGroup(ref66);

    // ── 1c. Left-side scale labels ───────────────────────────────────────
    double scaleXpos = mapLeft - 12.0;
    auto scaleLabel = [&](double y, const QString& text) {
        auto* item = new QGraphicsTextItem(text);
        item->setDefaultTextColor(QColor("#1E293B"));
        QFont sf; sf.setPointSize(11); sf.setBold(true); item->setFont(sf);
        item->setPos(scaleXpos - item->boundingRect().width(), y - item->boundingRect().height()/2.0);
        m_labelGroup->addToGroup(item);
    };
    // Top = 100%, 2/3 line, 1/3 line
    scaleLabel(y_channelTop,    QStringLiteral("重度 (100%)"));
    scaleLabel(y66,             QStringLiteral("中度 (66%)"));
    scaleLabel(y33,             QStringLiteral("轻度 (33%)"));

    // ── 2. Obstacle rects (Z=10) ────────────────────────────────────────
    m_filtered.clear();
    for (const auto& o : m_obstacles) {
        bool inc = false;
        switch (m_filterMode) {
        case 0: inc=true; break; case 1: inc=(o.severity==QStringLiteral("重度")); break;
        case 2: inc=(o.severity==QStringLiteral("中度")); break; case 3: inc=(o.severity==QStringLiteral("轻度")); break;
        }
        if (inc) m_filtered.append(o);
    }
    m_totalCount=m_obstacles.size(); m_filteredCount=m_filtered.size();

    auto groups = groupOverlapping(m_filtered);


    for (const auto& g : groups) {
        double s=g.start_m, e=std::min(g.end_m,m_channelLength);
        if (s>=m_channelLength) continue;
        QColor c = typeColor(g.typeNames);
        double hFrac = severityHeightFrac(g.severities);
        double obsH = ditchH * hFrac;
        // Draw from ditch bottom upward
        auto* rect = new QGraphicsRectItem(toX(s), y_channelBottom - obsH,
            toX(e)-toX(s), obsH);
        rect->setBrush(c); rect->setPen(QPen(c,1)); rect->setOpacity(1.0);
        m_obstacleGroup->addToGroup(rect);
    }

    // ── 3. Labels — transparent text, NO setHtml, NO background:white ──
    const double STAGGER_PX = 25.0;
    QVector<LabelLayout> typeLayouts, mileLayouts;
    // Ceiling clamp for mileage labels: must stay below title bottom
    QFont titleFont; titleFont.setPointSize(18); titleFont.setBold(true);
    QFontMetrics titleFm(titleFont);
    double mileCeiling = 30.0;  // generous ceiling near scene top, never reached by bottom labels
    computeMileLabelLayouts(groups,mapLeft,scaleX,
        y_mileBase_actual,STAGGER_PX,m_labelFontPixelH,mileCeiling,mileLayouts);


    for (int i=0; i<mileLayouts.size()&&i<groups.size(); ++i) {
        const auto& ll=mileLayouts[i]; const auto& g=groups[i];
        auto* item=new QGraphicsTextItem(
            QStringLiteral("%1-%2m").arg(g.start_m,0,'f',1).arg(g.end_m,0,'f',1));
        item->setDefaultTextColor(QColor("#0F172A"));
        QFont f; f.setPointSize(13); f.setBold(true); item->setFont(f);
        item->setPos(ll.x-item->boundingRect().width()/2.0, ll.y);
        m_labelGroup->addToGroup(item);
    }

        // ── 3b. Trajectory background — flat 2D CAD engineering style ──────
    double trajX = toX(0);
    double trajWidth = toX(m_channelLength) - trajX;
    const double wallThickness = 35.0;

    // 1. Water body (flat light blue, no gradient)
    auto* waterBody = new QGraphicsRectItem(trajX, y_trajAreaTop, trajWidth, trajAreaH);
    waterBody->setBrush(QColor(225, 240, 250));
    waterBody->setPen(Qt::NoPen);
    m_trajectoryGroup->addToGroup(waterBody);

    // 2+3. Concrete walls with clipRect-safe 45° hatching
    QRectF topWallR(trajX, y_trajAreaTop, trajWidth, wallThickness);
    QRectF botWallR(trajX, y_trajAreaBot - wallThickness, trajWidth, wallThickness);
    auto* topHatch = new HatchedWallItem(topWallR, QColor(212, 220, 228), QColor(130, 142, 158), 12, wallThickness);
    m_trajectoryGroup->addToGroup(topHatch);
    auto* botHatch = new HatchedWallItem(botWallR, QColor(212, 220, 228), QColor(130, 142, 158), 12, wallThickness);
    m_trajectoryGroup->addToGroup(botHatch);

    // 4. Border lines (outer frame + inner wall dividing lines)
    QPen borderPen(QColor(70, 90, 115), 1.5);
    auto* outerFrame = new QGraphicsRectItem(trajX, y_trajAreaTop, trajWidth, trajAreaH);
    outerFrame->setPen(borderPen); outerFrame->setBrush(Qt::NoBrush);
    m_trajectoryGroup->addToGroup(outerFrame);

    auto* topDiv = new QGraphicsLineItem(trajX, y_trajAreaTop + wallThickness, trajX + trajWidth, y_trajAreaTop + wallThickness);
    topDiv->setPen(borderPen); m_trajectoryGroup->addToGroup(topDiv);
    auto* botDiv = new QGraphicsLineItem(trajX, y_trajAreaBot - wallThickness, trajX + trajWidth, y_trajAreaBot - wallThickness);
    botDiv->setPen(borderPen); m_trajectoryGroup->addToGroup(botDiv);

    // ── Wall projection strips ──────────────────────────────────────────
    // Strict 1:1 projection of the bar chart below: iterate the exact same
    // grouped obstacle ranges and use the same color mapping as the bars.
    {
        double innerWallY = y_trajAreaBot - wallThickness;   // inner edge of bottom concrete wall
        for (const auto& g : groups) {
            double s = std::max(g.start_m, 0.0);
            double e = std::min(g.end_m, m_channelLength);
            double x1 = toX(s);
            double x2 = toX(e);
            double w  = std::max(x2 - x1, 2.0);
            auto* strip = new QGraphicsRectItem(x1, innerWallY - 18.0, w, 18.0);
            strip->setBrush(typeColor(g.typeNames));
            strip->setPen(Qt::NoPen);
            m_trajectoryGroup->addToGroup(strip);
        }
    }

    // "── 清洗轨迹" legend → upper-right of trajectory area
    auto* legend = new QGraphicsTextItem(QStringLiteral("── 清洗轨迹"));
    legend->setDefaultTextColor(TRAJECTORY_COLOR());
    QFont lf; lf.setPointSize(11); lf.setBold(true); legend->setFont(lf);
    legend->setPos(toX(m_channelLength) - 120, y_trajAreaTop + 8);
    m_labelGroup->addToGroup(legend);
    double trajY = y_traj;   // Y-center of trajectory (for dash-line & head icon)
    // Fine dense short-dash trajectory (2px, 6px-on / 4px-off, start→headPos)
    auto* dashLine = new QGraphicsLineItem(toX(0), trajY, toX(m_channelLength), trajY);
    QPen dashPen;
    dashPen.setColor(QColor(0, 122, 204));
    dashPen.setWidthF(2.0);
    { QVector<qreal> dp; dp << 3.0 << 2.0; dashPen.setDashPattern(dp); }
    dashPen.setCapStyle(Qt::FlatCap);
    dashLine->setPen(dashPen);
    m_trajectoryGroup->addToGroup(dashLine);

    // ── 5. Cleaning head (Z=100) ────────────────────────────────────────
    double hx = toX(m_headPos);
    if (!m_headIcon.isNull()) {
        auto* hpi = new HeadIconItem(m_headIcon);
        hpi->setPos(hx, trajY);   // centered on trajectory line
        m_headItem = hpi;          // P0-1: cache for fast position-only updates
        m_headGroup->addToGroup(hpi);
    } else {
        double ts=12.0; QPolygonF tri;
        tri<<QPointF(hx,trajY-iSz/2.0)<<QPointF(hx-ts,trajY+iSz/2.0-ts*1.5)
           <<QPointF(hx+ts,trajY+iSz/2.0-ts*1.5);
        auto* fb=new QGraphicsPolygonItem(tri);
        fb->setBrush(QColor("#0284C7")); fb->setPen(QPen(QColor("#0284C7"),1));
        m_headGroup->addToGroup(fb);
    }

    auto* hl = new QGraphicsTextItem(
        QStringLiteral("清洗头当前位置 %1m").arg(m_headPos,0,'f',1));
    hl->setDefaultTextColor(HEAD_LABEL_COLOR());
    QFont hf; hf.setPointSize(13); hf.setBold(true); hl->setFont(hf);
    double hlCx=hx, hlMinCx=mapLeft+4.0+hl->boundingRect().width()/2.0;
    if (hlCx<hlMinCx) hlCx=hlMinCx;
    hl->setPos(hlCx-hl->boundingRect().width()/2.0, y_headLabel);
    m_headGroup->addToGroup(hl);

    // ── 6a. Upper X-axis — below trajectory bottom, in clean blank area ──────
    {
        double axisY = y_trajAreaBot;   // baseline at traj bottom edge (沟渠最底外边线)
        auto toXloc = [&](double m) { return mapLeft + m * scaleX; };

        // 1. Axis baseline (沟渠底边外框线)
        auto* axisLine = new QGraphicsLineItem(toXloc(0), axisY, toXloc(m_channelLength), axisY);
        axisLine->setPen(QPen(QColor(60, 80, 105), 1));
        m_trajectoryGroup->addToGroup(axisLine);

        // 2. Downward ticks + labels (在底部混凝土墙下方的空白区域)
        double tickStep = std::max(5.0, std::round(m_channelLength / 10.0));
        QFont axFont; axFont.setPixelSize(15);
        for (double val = 0; val <= m_channelLength + 0.001; val += tickStep)
        {
            double tx = toXloc(val);
            // Tick goes DOWN from axisY by 4px
            auto* tick = new QGraphicsLineItem(tx, axisY, tx, axisY + 4.0);
            tick->setPen(QPen(QColor(60, 80, 105), 1));
            m_trajectoryGroup->addToGroup(tick);

            // Label at axisY+5 ~ axisY+19 (clean blank area, no interference)
            auto* lbl = new QGraphicsTextItem(QString::number(static_cast<int>(val)));
            lbl->setDefaultTextColor(QColor(40, 50, 65));
            lbl->setFont(axFont);
            lbl->setPos(tx - lbl->boundingRect().width() / 2.0, axisY + 5.0);
            m_trajectoryGroup->addToGroup(lbl);
        }
    }

    // ── 6b. Bottom X-axis (下横轴) — below trajectory ─────────────────────
    drawXAxis(y_lowerAxis, y_lowerTick, mapLeft, scaleX);

    // X-axis title
    auto* xT = new QGraphicsTextItem(QStringLiteral("里程 (m)"));
    xT->setDefaultTextColor(QColor("#475569"));
    QFont xtf; xtf.setPointSize(14); xtf.setBold(true); xT->setFont(xtf);
    xT->setPos(toX(m_channelLength/2.0)-xT->boundingRect().width()/2.0, y_axisTitle);
    m_labelGroup->addToGroup(xT);

    // ── 7. Legend ──────────────────────────────────────────────────────
    double legX = toX(m_channelLength) + TRAJECTORY_LEGEND_OFFSET_X;


    // ── 8. Title ────────────────────────────────────────────────────────
    auto* title = new QGraphicsTextItem(QStringLiteral("清洗轨迹与障碍物分布"));
    title->setDefaultTextColor(QColor("#0F172A"));
    QFont ttf; ttf.setPointSize(18); ttf.setBold(true); title->setFont(ttf);
    title->setPos(toX(m_channelLength/2.0)-title->boundingRect().width()/2.0, 0.0);
    m_labelGroup->addToGroup(title);
    // ── 8b. Right legend: 【障碍物类型】 ────────────────────────────────
    double legRX = mapRight + 15.0;
    double legTop = y_lowerAxis - 128.0;    // keep close to original, bottom just above X-axis ticks

    // ── Pre-calculate card bounds via QFontMetrics ─────────────────────
    QFont ttf2; ttf2.setPointSize(16); ttf2.setBold(true);
    QFont sf2;  sf2.setPointSize(15);
    QFontMetrics tfm2(ttf2), sfm2(sf2);

    double titleCardW = tfm2.horizontalAdvance(QStringLiteral("【障碍物类型】"));
    double rowCardW1  = 24.0 + sfm2.horizontalAdvance(QStringLiteral("石头 / 泥块"));
    double rowCardW2  = 24.0 + sfm2.horizontalAdvance(QStringLiteral("树枝 / 泥沙 / 积水"));
    double rowCardW3  = 24.0 + sfm2.horizontalAdvance(QStringLiteral("垃圾"));
    double maxContentW = std::max({titleCardW, rowCardW1, rowCardW2, rowCardW3});

    const double cardPad = 10.0;
    double cardX = legRX - cardPad;
    double cardY = legTop - cardPad;
    double cardW = maxContentW + 2.0 * cardPad;
    double cardH = 108.0 + 2.0 * cardPad;   // title(0) + 3x30px + last swatch 18px

    // ── Draw white rounded card background (added first = behind text) ──
    QPainterPath cardPath;
    cardPath.addRoundedRect(cardX, cardY, cardW, cardH, 8, 8);
    auto* card = new QGraphicsPathItem(cardPath);
    card->setBrush(QColor(255, 255, 255, 230));
    card->setPen(QPen(QColor(220, 220, 220), 1));
    m_labelGroup->addToGroup(card);

    double rly = legTop;

    auto* typeTitle = new QGraphicsTextItem(QStringLiteral("【障碍物类型】"));
    typeTitle->setDefaultTextColor(QColor("#0F172A"));
    typeTitle->setFont(ttf2);
    typeTitle->setPos(legRX, rly);
    m_labelGroup->addToGroup(typeTitle);

    auto swatch = [&](double y, const QColor& c, const QString& label) {
        auto* sq = new QGraphicsRectItem(legRX, y, 18, 18);
        sq->setBrush(c); sq->setPen(QPen(c,1));
        m_labelGroup->addToGroup(sq);
        auto* txt = new QGraphicsTextItem(label);
        txt->setDefaultTextColor(QColor("#334155"));
        txt->setFont(sf2);
        txt->setPos(legRX + 24, y - 2);
        m_labelGroup->addToGroup(txt);
    };

    swatch(rly+30, getObstacleColor(QStringLiteral("石头")), QStringLiteral("石头 / 泥块"));
    swatch(rly+60, getObstacleColor(QStringLiteral("树枝")), QStringLiteral("树枝 / 泥沙 / 积水"));
    swatch(rly+90, getObstacleColor(QStringLiteral("垃圾")), QStringLiteral("垃圾"));
// ══════════════════════════════════════════════════════════════════
    // CRITICAL: Compute scene rect dynamically from actual content bounds.
    // This guarantees X-axis and all labels are VISIBLE regardless of
    // viewport size — no more fixed padding that clips bottom elements.
    // ══════════════════════════════════════════════════════════════════
    double contentTop    = 0.0;  // scene top
    double contentBottom = y_axisTitle + 20.0;
    m_contentBottom = contentBottom;

    double margin2 = 20.0;
    QRectF finalSr(-margin2, contentTop - margin2,
                   vpW + 2*margin2 + 220.0,  // +220px for legend panel
                   contentBottom - contentTop + 2*margin2);

    m_scene->setSceneRect(finalSr);
    fitInView(finalSr, Qt::KeepAspectRatio);
    updateGeometry();
}






















































