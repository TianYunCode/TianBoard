#include "whiteboard/GraphicsStroke.h"

#include "whiteboard/GraphicsPolygonItem.h"

#include "whiteboard/BoardController.h"
#include "core/Application.h"
#include "whiteboard/GraphicsScene.h"

#include "utils/GeometryUtils.h"

typedef QPair<QPointF, qreal> strokePoint;

GraphicsStroke::GraphicsStroke(GraphicsScene *scene) : mScene(scene) {
    mAntiScaleRatio = 1. / (Application::boardController->systemScaleFactor() *
                            Application::boardController->currentZoom());
}

GraphicsStroke::~GraphicsStroke() {
    foreach (GraphicsPolygonItem *poly, mPolygons)
        poly->setStroke(NULL);

    mPolygons.clear();
}

void GraphicsStroke::addPolygon(GraphicsPolygonItem *pol) {
    remove(pol);
    mPolygons << pol;
}

void GraphicsStroke::remove(GraphicsPolygonItem *polygonItem) {
    int n = mPolygons.indexOf(polygonItem);
    if (n >= 0)
        mPolygons.removeAt(n);
}

QList<GraphicsPolygonItem *> GraphicsStroke::polygons() const {
    return mPolygons;
}

QList<QPair<QPointF, qreal>> GraphicsStroke::addPoint(const QPointF &point, qreal width, bool interpolate) {
    strokePoint newPoint(point, width);

    int n = mReceivedPoints.size();

    if (n == 0) {
        mReceivedPoints << newPoint;
        mDrawnPoints << newPoint;
        return QList<strokePoint>() << newPoint;
    }

    if (!interpolate) {
        strokePoint lastPoint = mReceivedPoints.last();
        mReceivedPoints << newPoint;
        mDrawnPoints << newPoint;
        return QList<strokePoint>() << lastPoint << newPoint;
    }

    else {
        // 平滑插值不是在两个真实落点之间做，而是在“倒数第二点—最后点”的中点
        // 与“最后点—当前点”的中点之间做，这样每一段贝塞尔的端点都是已画位置。

        // 第一没有“上一段”可接，直接一条直线连到第一个中点
        if (n == 1) {
            QPointF lastPoint = mReceivedPoints[0].first;
            qreal lastWidth = mReceivedPoints[0].second;
            strokePoint p(((lastPoint + point) / 2.0), (lastWidth + width) / 2.0);
            mReceivedPoints << newPoint;
            mDrawnPoints << p;

            return QList<strokePoint>() << mReceivedPoints[0] << p;
        }

        QPointF p0 = mReceivedPoints[mReceivedPoints.size() - 2].first;
        QPointF p1 = mReceivedPoints[mReceivedPoints.size() - 1].first;
        QPointF p2 = point;

        QPointF startPoint = (p1 + p0) / 2.0;
        QPointF endPoint = (p2 + p1) / 2.0;

        QList<QPointF> calculated = GeometryUtils::quadraticBezier(startPoint, p1, endPoint, 10);
        QList<strokePoint> newPoints;

        qreal startWidth = mDrawnPoints.last().second;

        for (int i(0); i < calculated.size(); ++i) {
            qreal w = startWidth + (qreal(i) / qreal(calculated.size() - 1)) * (width - startWidth);
            newPoints << strokePoint(calculated[i], w);
        }

        // 上一段的终点就是这一段的起点，去掉重复点，否则多边形会退化
        if (newPoints.first().first == mDrawnPoints.last().first)
            mDrawnPoints.removeLast();

        foreach (strokePoint p, newPoints)
            mDrawnPoints << p;

        mReceivedPoints << strokePoint(point, width);
        return newPoints;
    }

    return QList<strokePoint>();
}

bool GraphicsStroke::hasPressure() {
    if (mPolygons.count() > 2) {
        qreal nominalWidth = mPolygons.at(0)->originalWidth();

        foreach (GraphicsPolygonItem *pol, mPolygons) {
            if (!pol->isNominalLine() || pol->originalWidth() != nominalWidth)
                return true;
        }
        return false;
    } else
        return true;
}


GraphicsStroke *GraphicsStroke::deepCopy() {
    GraphicsStroke *clone = new GraphicsStroke();

    return clone;
}

bool GraphicsStroke::hasAlpha() const {
    if (mPolygons.length() > 0 && mPolygons.at(0)->color().alphaF() != 1.0) {
        return true;
    } else {
        return false;
    }
}

void GraphicsStroke::clear() {
    if (!mPolygons.empty()) {
        mPolygons.clear();
    }
}

// 返回一条点更少、多边形也更少的等价笔迹。少于三个点没什么可简化的，返回空指针，
// 调用方保留原笔迹。
GraphicsStroke *GraphicsStroke::simplify() {
    if (mDrawnPoints.size() < 3)
        return NULL;

    GraphicsStroke *newStroke = new GraphicsStroke();
    newStroke->mDrawnPoints = QList<strokePoint>(mDrawnPoints);

    QList<strokePoint> &points = newStroke->mDrawnPoints;
    // 简化算法：把当前点与其后两点记作 A、B、C。(AB) 与 (BC) 的夹角小于阈值就认为三点共线，
    // 删掉中间的 B；然后 A 不变，用接下来的点当新的 B、C 继续判，直到不共线为止，
    // 此时当前的 B 升级为新的 A，如此往下推。
    // A 与 B 之间笔宽差得太多时即使共线也不删 B，否则粗细过渡会被抹平成一段直边。
    // TODO: 目前只会删“近似直线”上的点，弯弧段还简不掉，需要换更聪明的算法

    // AB 与 BC 的夹角（度）低于它就判定三点共线
    qreal thresholdAngle = Settings::settings()->boardSimplifyPenStrokesThresholdAngle->get().toReal();

    // 相邻两点（A 与 B）笔宽的相对比值低于它才算粗细一致
    qreal thresholdWidthDifference =
        Settings::settings()->boardSimplifyPenStrokesThresholdWidthDifference->get().toReal();

    QList<strokePoint>::iterator it = points.begin();
    QList<QList<strokePoint>::iterator> toDelete;

    while (it + 2 != points.end()) {
        // it、b_it、b_it+1 分别就是上面的 A、B、C
        QList<strokePoint>::iterator b_it(it + 1);

        while (b_it + 1 != points.end()) {
            qreal angle = qFabs(180 - (GeometryUtils::angle(it->first, b_it->first, (b_it + 1)->first)));
            qreal widthRatio = qMax(it->second, b_it->second) / qMin(it->second, b_it->second);

            if (angle < thresholdAngle && widthRatio < thresholdWidthDifference)
                b_it = points.erase(b_it);
            else
                break;
        }

        if (b_it + 1 == points.end())
            break;
        else
            it = b_it;
    }

    // 再走一遍简化后的点，用 drawCurve 标记的地方切段，每段生成一个多边形。

    QList<GraphicsPolygonItem *> newPolygons;
    QList<strokePoint> newStrokePoints;
    int i(0);

    while (i < points.size()) {
        bool drawCurve = false;

        newStrokePoints << points[i];

        // 限制单个多边形的规模、遇到急折角就另起一段，解决两件事：
        // 1. 半透明多边形自叠时，Qt 有时会填错，出现不该有的深色块；
        // 2. 简化后的急角本来是尖的，切开之后每段各自平滑，转角才是圆的而不是刀尖。

        if (newStrokePoints.size() > 1 && i < points.size() - 1) {
            qreal angle =
                qFabs(GeometryUtils::angle(points[i - 1].first, points[i].first, points[i + 1].first));
            if (angle < 150) // 经验阈值：小于 150 度算急折，需要可调再进设置
                drawCurve = true;
        }

        if (hasAlpha() && newStrokePoints.size() % 20 == 0)
            drawCurve = true;

        if (drawCurve) {
            GraphicsPolygonItem *poly =
                mScene->polygonToPolygonItem(GeometryUtils::curveToPolygon(newStrokePoints, true, true));

            // 半透明笔迹：把前面已经画过的段从本段里减掉，避免自叠处变深
            if (!poly->brush().isOpaque()) {
                foreach (GraphicsPolygonItem *prev, newPolygons)
                    poly->subtract(prev);
            }

            newPolygons << poly;
            newStrokePoints.clear();
            --i;
        }

        ++i;
    }

    if (newStrokePoints.size() > 0) {
        GraphicsPolygonItem *poly =
            mScene->polygonToPolygonItem(GeometryUtils::curveToPolygon(newStrokePoints, true, true));

        if (!poly->brush().isOpaque()) {
            foreach (GraphicsPolygonItem *prev, newPolygons)
                poly->subtract(prev);
        }

        newPolygons << poly;
    }


    newStroke->mPolygons = QList<GraphicsPolygonItem *>(newPolygons);

    foreach (GraphicsPolygonItem *poly, newStroke->mPolygons) {
        poly->setFillRule(Qt::WindingFill);
        poly->setStroke(newStroke);
    }

    return newStroke;
}
