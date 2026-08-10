// =============================================================================
// ChannelMapWidget.h — 2D channel digital twin using QGraphicsView
// =============================================================================
#pragma once
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPixmap>
#include <QVector>
#include <QFont>
#include "../models/DeviceState.h"

class ChannelMapWidget : public QGraphicsView {
    Q_OBJECT
public:
    explicit ChannelMapWidget(QWidget* parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void setChannelLength(double length);
    void setObstacles(const QVector<Obstacle>& obstacles);
    void setHeadPosition(double pos);
    void setDisplayFilter(int filterMode);

    int filteredCount() const { return m_filteredCount; }
    int totalCount() const    { return m_totalCount; }

    // Pre-computed label layout for collision avoidance
    struct LabelLayout {
        double x = 0.0;        // scene X center
        double y = 0.0;        // scene Y top
        double w = 0.0;        // pixel width
        double h = 0.0;        // pixel height
        double left()  const { return x - w/2.0; }
        double right() const { return x + w/2.0; }
        double top()   const { return y; }
        double bottom()const { return y + h; }
        bool overlaps(const LabelLayout& o) const {
            return left() < o.right() && right() > o.left()
                && top() < o.bottom() && bottom() > o.top();
        }
    };

    // Merged obstacle group for overlapping intervals
    struct GroupedObstacle {
        double start_m, end_m;
        QStringList typeNames;
        QStringList severities;
        double length() const { return end_m - start_m; }
    };

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void rebuildScene();
    QRectF computeSceneRect() const;

    // Label helpers
    QVector<GroupedObstacle> groupOverlapping(const QVector<Obstacle>& raw) const;
    void drawXAxis(double yLine, double yLabel, double mapLeft, double scaleX);
public:
    static QString mergeTypeNames(const QStringList& names);
    static QString mergeSeverityColor(const QStringList& sevs);
private:
    void placeLabelsBelowDitch(
        const QVector<GroupedObstacle>& groups,
        double mapLeft, double scaleX,
        double baseY, double ditchBottomY, double trajectoryY,
        double staggerDelta,
        QVector<LabelLayout>& outLayouts);
    void placeLabelsAboveDitch(
        const QVector<GroupedObstacle>& groups,
        double mapLeft, double scaleX,
        double baseY, double ditchTopY,
        double staggerDelta,
        QVector<LabelLayout>& outLayouts);

    QGraphicsScene* m_scene         = nullptr;
    double          m_channelLength = 100.0;
    QVector<Obstacle> m_obstacles;
    QVector<Obstacle> m_filtered;
    double          m_headPos       = 0.0;
    int             m_filterMode    = 0;
    int             m_filteredCount = 0;
    int             m_totalCount    = 0;

    QGraphicsItemGroup* m_ditchGroup    = nullptr;
    QGraphicsItemGroup* m_obstacleGroup = nullptr;
    QGraphicsItemGroup* m_trajectoryGroup = nullptr;
    QGraphicsItemGroup* m_headGroup     = nullptr;
    QGraphicsItemGroup* m_labelGroup    = nullptr;

    // Cached head icon loaded from Qt resources
    QPixmap m_headIcon;
    class HeadIconItem* m_headItem  = nullptr;   // P0-1: persistent head item (no full rebuild on every tick)
    double  m_prevChanLen = 0.0;                   // P0-2: skip rebuild when value unchanged
    int     m_obsCheckSum = 0;                     // P0-2: obstacle data checksum for change detection

    // Cached font metrics for label pre-computation
    QFont m_labelFont;
    double m_labelFontPixelH = 0.0;

    double m_contentBottom = 0.0;
};



