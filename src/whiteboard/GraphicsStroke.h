#pragma once

#include <QtWidgets>

#include "utils/Enums.h"

class GraphicsPolygonItem;
class GraphicsScene;

class GraphicsStroke {
    friend class GraphicsPolygonItem;

public:
    GraphicsStroke(GraphicsScene *scene = NULL);
    virtual ~GraphicsStroke();

    bool hasPressure();

    QList<GraphicsPolygonItem *> polygons() const;

    void remove(GraphicsPolygonItem *polygonItem);

    GraphicsStroke *deepCopy();

    bool hasAlpha() const;

    void clear();

    QList<QPair<QPointF, qreal>> addPoint(const QPointF &point, qreal width, bool interpolate = false);

    const QList<QPair<QPointF, qreal>> &points() { return mDrawnPoints; }

    GraphicsStroke *simplify();

protected:
    void addPolygon(GraphicsPolygonItem *pol);

private:
    GraphicsScene *mScene;

    QList<GraphicsPolygonItem *> mPolygons;

    QList<QPair<QPointF, qreal>> mReceivedPoints;

    QList<QPair<QPointF, qreal>> mDrawnPoints;

    qreal mAntiScaleRatio;
};
