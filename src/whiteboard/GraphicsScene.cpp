#include "whiteboard/GraphicsScene.h"
#include "model/PackageLayout.hpp"
#include "utils/UserNotices.h"

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QtSvg>
#include <QtSvgWidgets>
#include <QGraphicsView>
#include <QGraphicsVideoItem>

#include "core/Application.h"
#include "utils/GeometryUtils.h"
#include "utils/Snapping.h"

#include "settings/Settings.h"
#include "core/PersistenceManager.h"
#include "utils/TextTools.h"

#include "whiteboard/Magnifier.h"
#include "whiteboard/ToolWidget.h"
#include "model/Resources.h"

#include "whiteboard/GraphicsRuler.h"
#include "whiteboard/GraphicsProtractor.h"
#include "whiteboard/GraphicsAxes.h"
#include "whiteboard/GraphicsCompass.h"
#include "whiteboard/GraphicsTriangle.h"
#include "whiteboard/GraphicsCurtainItem.h"
#include "whiteboard/GraphicsCache.h"

#include "model/DocumentProxy.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"
#include "whiteboard/BoardView.h"

#include "whiteboard/GraphicsItemUndoCommand.h"
#include "whiteboard/GraphicsItemGroupUndoCommand.h"
#include "whiteboard/GraphicsTextItemUndoCommand.h"
#include "whiteboard/GraphicsProxyWidget.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsPolygonItem.h"
#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsWidgetItem.h"
#include "whiteboard/GraphicsPDFItem.h"
#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsStrokesGroup.h"
#include "whiteboard/SelectionFrame.h"
#include "whiteboard/GraphicsItemZLevelUndoCommand.h"

#include "whiteboard/GraphicsGroupContainerItem.h"

#include "whiteboard/GraphicsStroke.h"

#define DEFAULT_Z_VALUE 0.0

qreal ZLayerController::errorNumber = -20000001.0;

ZLayerController::ZLayerController(QGraphicsScene *scene) : mScene(scene)

{
    scopeMap.insert(itemLayerType::NoLayer, ItemLayerTypeData(errorNumber, errorNumber));
    scopeMap.insert(itemLayerType::BackgroundItem, ItemLayerTypeData(-1000000.0, -1000000.0));
    // 对象层刻意不包含 DEFAULT_Z_VALUE：留着 0 这个值才能一眼认出"还没分配过 Z 值"的新图元
    scopeMap.insert(itemLayerType::ObjectItem, ItemLayerTypeData(-1000000.0, DEFAULT_Z_VALUE - 1.0));
    scopeMap.insert(itemLayerType::DrawingItem, ItemLayerTypeData(DEFAULT_Z_VALUE + 1.0, 1000000.0));
    scopeMap.insert(itemLayerType::ToolItem, ItemLayerTypeData(1000000.0, 1000100.0));
    scopeMap.insert(itemLayerType::CppTool, ItemLayerTypeData(1000100.0, 1000200.0));
    scopeMap.insert(itemLayerType::Curtain, ItemLayerTypeData(1000200.0, 1001000.0));
    scopeMap.insert(itemLayerType::Eraiser, ItemLayerTypeData(1001000.0, 1001100.0));
    scopeMap.insert(itemLayerType::Pointer, ItemLayerTypeData(1001100.0, 1001200.0));
    scopeMap.insert(itemLayerType::Cache, ItemLayerTypeData(1001300.0, 1001400.0));

    scopeMap.insert(itemLayerType::SelectedItem, ItemLayerTypeData(1001000.0, 1001000.0));
    scopeMap.insert(itemLayerType::SelectionFrame, ItemLayerTypeData(1010000.0, 1010000.0));
}

qreal ZLayerController::generateZLevel(itemLayerType::Enum key)
{
    if (!scopeMap.contains(key))
    {
        qDebug() << "Number is out of layer scope";
        return errorNumber;
    }

    qreal result           = scopeMap.value(key).curValue;
    qreal top              = scopeMap.value(key).topLimit;
    qreal incrementalStep  = scopeMap.value(key).incStep;

    result                += incrementalStep;
    if (result >= top)
    {
        // 只有区间上下限不重合（即该层能容纳多个值）时，用尽才值得告警
        if (scopeMap.value(key).topLimit != scopeMap.value(key).bottomLimit)
        {
            qDebug() << "new values are over for the scope" << key;
        }
        result = top - incrementalStep;
    }

    scopeMap[key].curValue = result;

    return result;
}
qreal ZLayerController::generateZLevel(QGraphicsItem *item)
{
    qreal               result = errorNumber;
    itemLayerType::Enum type   = static_cast<itemLayerType::Enum>(item->data(GraphicsItemData::itemLayerType).toInt());

    if (validLayerType(type))
    {
        result = generateZLevel(type);
    }

    return result;
}

qreal ZLayerController::changeZLevelTo(QGraphicsItem *item, moveDestination dest)
{
    itemLayerType::Enum curItemLayerType = typeForData(item);
    if (curItemLayerType == itemLayerType::NoLayer)
    {
        qDebug() << "item's layer is out of the scope. Can't implement z-layer changing operation";
        return errorNum();
    }

    // 只挑与目标图元同层的图元，按"自身 Z 值"塞进有序 QMultiMap
    QMultiMap<qreal, QGraphicsItem *> sortedItems;
    if (mScene->items().count())
    {
        foreach (QGraphicsItem *tmpItem, mScene->items())
        {
            if (typeForData(tmpItem) == curItemLayerType)
            {
                sortedItems.insert(tmpItem->data(GraphicsItemData::ItemOwnZValue).toReal(), tmpItem);
            }
        }
    }

    // 层里只有它自己时无事可做，直接返回其自身 Z 值
    if (sortedItems.count() == 1 && sortedItems.values().first() == item)
    {
        qDebug() << "only one item exists in layer. Have nothing to change";
        return item->data(GraphicsItemData::ItemOwnZValue).toReal();
    }

    QMultiMapIterator<qreal, QGraphicsItem *> iCurElement(sortedItems);

    if (dest == up)
    {
        qDebug() << "item data zvalue= " << item->data(GraphicsItemData::ItemOwnZValue).toReal();
        if (iCurElement.findNext(item))
        {
            if (iCurElement.hasNext())
            {
                qreal nextZ = iCurElement.peekNext().value()->data(GraphicsItemData::ItemOwnZValue).toReal();
                GraphicsItem::assignZValue(iCurElement.peekNext().value(), item->data(GraphicsItemData::ItemOwnZValue).toReal());
                GraphicsItem::assignZValue(item, nextZ);

                iCurElement.next();

                while (iCurElement.hasNext() && iCurElement.peekNext().value()->data(GraphicsItemData::ItemOwnZValue).toReal() == nextZ)
                {
                    GraphicsItem::assignZValue(iCurElement.next().value(), nextZ);
                }
            }
        }
    }
    else if (dest == top)
    {
        if (iCurElement.findNext(item))
        {
            if (iCurElement.hasNext())
            {
                GraphicsItem::assignZValue(item, generateZLevel(item));
            }
        }
    }
    else if (dest == down)
    {
        iCurElement.toBack();
        if (iCurElement.findPrevious(item))
        {
            if (iCurElement.hasPrevious())
            {
                qreal nextZ = iCurElement.peekPrevious().value()->data(GraphicsItemData::ItemOwnZValue).toReal();
                GraphicsItem::assignZValue(iCurElement.peekPrevious().value(), item->data(GraphicsItemData::ItemOwnZValue).toReal());
                GraphicsItem::assignZValue(item, nextZ);

                while (iCurElement.hasNext() && iCurElement.peekNext().value()->data(GraphicsItemData::ItemOwnZValue).toReal() == nextZ)
                {
                    GraphicsItem::assignZValue(iCurElement.next().value(), nextZ);
                }
            }
        }
    }
    else if (dest == bottom)
    {
        iCurElement.toBack();
        if (iCurElement.findPrevious(item))
        {
            if (iCurElement.hasPrevious())
            {
                qreal oldz = item->data(GraphicsItemData::ItemOwnZValue).toReal();
                iCurElement.toFront();
                qreal nextZ                            = iCurElement.next().value()->data(GraphicsItemData::ItemOwnZValue).toReal();

                ItemLayerTypeData curItemLayerTypeData = scopeMap.value(curItemLayerType);

                // 该层最底下的图元与层下界之间还有空档，就把新值贴着层内第一个图元插进去
                if (nextZ > curItemLayerTypeData.bottomLimit + curItemLayerTypeData.incStep)
                {
                    qreal result = nextZ - curItemLayerTypeData.incStep;
                    GraphicsItem::assignZValue(item, result);
                }
                else
                {
                    GraphicsItem::assignZValue(item, nextZ);

                    bool doubleGap = false; // 记录是否找到过空档：有空档就能把图元直接插进去，不必整层往下挪

                    while (iCurElement.peekNext().value() != item)
                    {
                        qreal curZ     = iCurElement.value()->data(GraphicsItemData::ItemOwnZValue).toReal();
                        qreal curNextZ = iCurElement.peekNext().value()->data(GraphicsItemData::ItemOwnZValue).toReal();
                        if (curNextZ - curZ >= 2 * curItemLayerTypeData.incStep)
                        {
                            GraphicsItem::assignZValue(iCurElement.value(), curZ + curItemLayerTypeData.incStep);
                            doubleGap = true;
                            break;
                        }
                        else
                        {
                            GraphicsItem::assignZValue(iCurElement.value(), curNextZ);
                            iCurElement.next();
                        }
                    }
                    if (!doubleGap)
                    {
                        GraphicsItem::assignZValue(iCurElement.value(), oldz);

                        while (iCurElement.hasNext() && (iCurElement.peekNext().value()->data(GraphicsItemData::ItemOwnZValue).toReal() == oldz))
                        {
                            GraphicsItem::assignZValue(iCurElement.next().value(), oldz);
                        }
                    }
                }
            }
        }
    }

    // 先取消选中再重新选中，逼一次 selectionChanged，选中框才会按新的 Z 序重画。
    item->scene()->clearSelection();
    item->setSelected(true);

    item->setZValue(item->data(GraphicsItemData::ItemOwnZValue).toReal());

    return item->data(GraphicsItemData::ItemOwnZValue).toReal();
}

itemLayerType::Enum ZLayerController::typeForData(QGraphicsItem *item) const
{
    itemLayerType::Enum result = static_cast<itemLayerType::Enum>(item->data(GraphicsItemData::itemLayerType).toInt());

    if (!scopeMap.contains(result))
    {
        result = itemLayerType::NoLayer;
    }

    return result;
}

void ZLayerController::setLayerType(QGraphicsItem *pItem, itemLayerType::Enum pNewType)
{
    pItem->setData(GraphicsItemData::itemLayerType, QVariant(pNewType));
}

void ZLayerController::shiftStoredZValue(QGraphicsItem *item, qreal zValue)
{
    itemLayerType::Enum type = typeForData(item);

    if (validLayerType(type))
    {
        ItemLayerTypeData typeData = scopeMap.value(type);
        if (typeData.curValue < zValue)
        {
            scopeMap[type].curValue = zValue;
        }
    }
}

bool ZLayerController::zLevelAvailable(qreal z)
{
    foreach (QGraphicsItem *it, dynamic_cast<GraphicsScene *>(mScene)->getFastAccessItems())
    {
        if (it->zValue() == z)
        {
            return false;
        }
    }

    return true;
}

GraphicsScene::GraphicsScene(DocumentProxy *parent, bool enableUndoRedoStack)
    : CoreGraphicsScene(parent), mEraser(0), mPointer(0), mMarkerCircle(0), mPenCircle(0), mDocument(parent), mDarkBackground(false),
      mPageBackground(PageBackground::plain), mIsDesktopMode(false), mZoomFactor(1), mBackgroundObject(0), mPreviousWidth(0),
      mDistanceFromLastStrokePoint(0), mInputDeviceIsPressed(false), mArcPolygonItem(0), mRenderingContext(Screen), mCurrentStroke(0), mItemCount(0),
      mUndoRedoStackEnabled(enableUndoRedoStack), magnifierControlViewWidget(0), magnifierDisplayViewWidget(0),
      mZLayerController(new ZLayerController(this)), mpLastPolygon(NULL), mCurrentPolygon(0), mTempPolygon(NULL), mSelectionFrame(0)
{
    CoreGraphicsScene::setObjectName("BoardScene");
    setItemIndexMethod(BspTreeIndex);

    setUuid(QUuid::createUuid());
    setDocument(parent);
    createEraiser();
    createPointer();
    createMarkerCircle();
    createPenCircle();

    if (Application::applicationController)
    {
        setViewState(SceneViewState(1, Application::applicationController->initialHScroll(), Application::applicationController->initialVScroll()));
    }

    mBackgroundGridSize = Settings::settings()->crossSize;

    connect(BoardController::undoStack(), &QUndoStack::indexChanged, this, &GraphicsScene::updateSelectionFrameWrapper);
}

GraphicsScene::~GraphicsScene()
{
    if (mCurrentStroke && mCurrentStroke->polygons().empty())
    {
        delete mCurrentStroke;
        mCurrentStroke = NULL;
    }

    if (mZLayerController)
    {
        delete mZLayerController;
    }
}

void GraphicsScene::selectionChangedProcessing()
{
    if (selectedItems().count())
    {
        UserNotices::presentStatusMessage("ZValue is " + QString::number(selectedItems().first()->zValue(), 'f') + "own z value is " +
                                          QString::number(selectedItems().first()->data(GraphicsItemData::ItemOwnZValue).toReal(), 'f'));
    }
}

void GraphicsScene::setLastCenter(QPointF center)
{
    mViewState.setLastSceneCenter(center);
}

QPointF GraphicsScene::lastCenter()
{
    return mViewState.lastSceneCenter();
}

bool GraphicsScene::inputDevicePress(const QPointF &scenePos, const qreal &pressure)
{
    bool accepted = false;

    if (mInputDeviceIsPressed)
    {
        qDebug() << "scene received input device pressed, without input device release, muting event as "
                    "input device move";
        accepted = inputDeviceMove(scenePos, pressure);
    }
    else
    {
        mInputDeviceIsPressed        = true;

        StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

        if (DrawingController::drawingController()->isDrawingTool())
        {
            // 走到这里说明用的是画笔、记号笔或直线工具。
            qreal width = 0;

            // 上一笔还没落到任何多边形上，先把它丢掉
            if (mCurrentStroke && mCurrentStroke->polygons().empty())
            {
                delete mCurrentStroke;
                mCurrentStroke = NULL;
            }

            // 收起记号笔的粗细预览圈
            if (currentTool == StylusTool::Marker)
            {
                hideMarkerCircle();
            }

            // 收起画笔的粗细预览圈
            if (currentTool == StylusTool::Pen)
            {
                hidePenCircle();
            }

            // 开一条新笔迹：一笔就是一组多边形，落点逐渐追加进去
            mCurrentStroke = new GraphicsStroke(this);

            if (currentTool != StylusTool::Line)
            {
                // 笔宽随压感变化
                width = DrawingController::drawingController()->currentToolWidth() * pressure;
            }
            else
            {
                // 直线工具不吃压感，否则画不出等宽直线
                width = DrawingController::drawingController()->currentToolWidth();
            }

            width /= BoardController::instance()->systemScaleFactor();
            width /= BoardController::instance()->currentZoom();

            mAddedItems.clear();
            mRemovedItems.clear();

            if (DrawingController::drawingController()->mActiveRuler)
            {
                DrawingController::drawingController()->mActiveRuler->StartLine(scenePos, width);
            }
            else
            {
                moveTo(scenePos);
                drawLineTo(scenePos, width, DrawingController::drawingController()->stylusTool() == StylusTool::Line);

                mCurrentStroke->addPoint(scenePos, width);
            }
            accepted = true;
        }
        else if (currentTool == StylusTool::Eraser)
        {
            mAddedItems.clear();
            mRemovedItems.clear();
            moveTo(scenePos);

            qreal eraserWidth  = Settings::settings()->currentEraserWidth();
            eraserWidth       /= BoardController::instance()->systemScaleFactor();
            eraserWidth       /= BoardController::instance()->currentZoom();

            eraseLineTo(scenePos, eraserWidth);
            drawEraser(scenePos, mInputDeviceIsPressed);

            accepted = true;
        }
        else if (currentTool == StylusTool::Pointer)
        {
            drawPointer(scenePos, true);
            accepted = true;
        }
    }

    if (mCurrentStroke && mCurrentStroke->polygons().empty())
    {
        delete mCurrentStroke;
        mCurrentStroke = NULL;
    }

    return accepted;
}

bool GraphicsScene::inputDeviceMove(const QPointF &scenePos, const qreal &pressure)
{
    bool accepted                  = false;

    DrawingController *dc          = DrawingController::drawingController();
    StylusTool::Enum   currentTool = (StylusTool::Enum)dc->stylusTool();

    QPointF position               = QPointF(scenePos);

    if (currentTool == StylusTool::Eraser)
    {
        drawEraser(position, mInputDeviceIsPressed);
        accepted = true;
    }

    else if (currentTool == StylusTool::Marker)
    {
        if (mInputDeviceIsPressed)
        {
            hideMarkerCircle();
        }
        else
        {
            drawMarkerCircle(position);
            accepted = true;
        }
    }

    else if (currentTool == StylusTool::Pen)
    {
        if (mInputDeviceIsPressed)
        {
            hidePenCircle();
        }
        else
        {
            drawPenCircle(position);
            accepted = true;
        }
    }

    if (mInputDeviceIsPressed)
    {
        if (dc->isDrawingTool())
        {
            qreal width = 0;

            if (currentTool != StylusTool::Line)
            {
                // 压感下限取 0.2：抬笔时压感归零会画出看不见的一笔
                width = dc->currentToolWidth() * qMax(pressure, 0.2);
            }
            else
            {
                // 直线工具不吃压感
                width = dc->currentToolWidth();
            }

            width /= BoardController::instance()->systemScaleFactor();
            width /= BoardController::instance()->currentZoom();

            if (currentTool == StylusTool::Line || dc->mActiveRuler)
            {
                if (DrawingController::drawingController()->stylusTool() != StylusTool::Marker)
                {
                    if (NULL != mpLastPolygon && NULL != mCurrentStroke && mAddedItems.size() > 0)
                    {
                        CoreGraphicsScene::removeItemFromDeletion(mpLastPolygon);
                        mAddedItems.remove(mpLastPolygon);
                        mCurrentStroke->remove(mpLastPolygon);
                        if (mCurrentStroke->polygons().empty())
                        {
                            delete mCurrentStroke;
                            mCurrentStroke = NULL;
                        }
                        removeItem(mpLastPolygon);
                        mPreviousPolygonItems.removeAll(mpLastPolygon);
                    }
                }

                // 直线吸附到 45 度整数倍上，手抖也能画出水平/垂直/对角线

                QLineF radius(mPreviousPoint, position);
                qreal  angle         = radius.angle();
                angle                = qRound(angle / 45) * 45;
                qreal   radiusLength = radius.length();
                QPointF newPosition(
                    mPreviousPoint.x() + radiusLength * cos((angle * PI) / 180), mPreviousPoint.y() - radiusLength * sin((angle * PI) / 180));
                QLineF chord(position, newPosition);
                if (chord.length() < qMin((int)16, (int)(radiusLength / 20)))
                {
                    position = newPosition;
                }
            }

            if (!mCurrentStroke)
            {
                mCurrentStroke = new GraphicsStroke(this);
            }

            if (dc->mActiveRuler)
            {
                dc->mActiveRuler->DrawLine(position, width);
            }

            else if (currentTool == StylusTool::Line)
            {
                drawLineTo(position, width, true);
            }

            else
            {
                bool interpolate = false;

                if ((currentTool == StylusTool::Pen && Settings::settings()->boardInterpolatePenStrokes->get().toBool()) ||
                    (currentTool == StylusTool::Marker && Settings::settings()->boardInterpolateMarkerStrokes->get().toBool()))
                {
                    interpolate = true;
                }

                // 太短的一段不生成多边形：少画几块是一方面，主要是短段的斜接看着毛刺。

                qreal antiScaleRatio          = 1. / (BoardController::instance()->systemScaleFactor() * BoardController::instance()->currentZoom());
                // 阈值按缩放反算，屏幕上始终约 10 像素；取值是经验值，需要可调再进设置。
                qreal MIN_DISTANCE            = 10 * antiScaleRatio;
                qreal distance                = QLineF(mPreviousPoint, scenePos).length();

                mDistanceFromLastStrokePoint += distance;

                if (mDistanceFromLastStrokePoint > MIN_DISTANCE)
                {
                    QList<QPair<QPointF, qreal>> newPoints = mCurrentStroke->addPoint(scenePos, width, interpolate);
                    if (newPoints.length() > 1)
                    {
                        drawCurve(newPoints);
                    }

                    mDistanceFromLastStrokePoint = 0;
                }

                if (interpolate)
                {
                    // 贝塞尔只画到前后两点的中点，笔尖当前位置还没画出来，跟手会觉得慢半拍。
                    // 所以补一段临时多边形连到当前点；下一段生成时删掉它，停笔时正式并入笔迹。

                    if (mTempPolygon)
                    {
                        removeItem(mTempPolygon);
                        mTempPolygon = NULL;
                    }

                    QPointF lastDrawnPoint = mCurrentStroke->points().last().first;

                    mTempPolygon           = lineToPolygonItem(QLineF(lastDrawnPoint, scenePos), mPreviousWidth, width);
                    addItem(mTempPolygon);
                }
            }
        }
        else if (currentTool == StylusTool::Eraser)
        {
            qreal eraserWidth  = Settings::settings()->currentEraserWidth();
            eraserWidth       /= BoardController::instance()->systemScaleFactor();
            eraserWidth       /= BoardController::instance()->currentZoom();

            eraseLineTo(position, eraserWidth);
        }
        else if (currentTool == StylusTool::Pointer)
        {
            drawPointer(position);
        }

        accepted = true;
    }

    return accepted;
}

bool GraphicsScene::inputDeviceRelease()
{
    bool accepted = false;

    if (mPointer)
    {
        mPointer->hide();
        accepted = true;
    }

    StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

    if (currentTool == StylusTool::Eraser)
    {
        redrawEraser(false);
    }

    DrawingController *dc = DrawingController::drawingController();

    if (dc->isDrawingTool() || mDrawWithCompass)
    {
        if (mArcPolygonItem)
        {
            GraphicsStrokesGroup *pStrokes = new GraphicsStrokesGroup();

            // 弧本身并入这组笔迹
            mAddedItems.remove(mArcPolygonItem);
            removeItem(mArcPolygonItem);
            CoreGraphicsScene::removeItemFromDeletion(mArcPolygonItem);
            mArcPolygonItem->setStrokesGroup(pStrokes);
            pStrokes->addToGroup(mArcPolygonItem);

            // 圆心的十字标记也归进同一组，删一个就删整组
            foreach (QGraphicsItem *item, mAddedItems)
            {
                mAddedItems.remove(item);
                removeItem(item);
                CoreGraphicsScene::removeItemFromDeletion(item);
                GraphicsPolygonItem *pi = qgraphicsitem_cast<GraphicsPolygonItem *>(item);
                if (pi)
                {
                    pi->setStrokesGroup(pStrokes);
                }
                pStrokes->addToGroup(item);
            }

            mAddedItems.clear();
            mAddedItems << pStrokes;
            addItem(pStrokes);

            mDrawWithCompass = false;
        }
        else if (mCurrentStroke)
        {
            // 抬笔点那一小段只存在于 mTempPolygon 里：插值时贝塞尔只画到相邻落点的中点，
            // 真正的落点从没进过 mCurrentStroke 的点表。所以它必须在简化**之后**才并入笔迹——
            // simplify() 是按点表重建多边形的，先并进去就会被它整批扔掉，每一笔的尾巴都短一截
            // （--eraser-selftest 画一笔再量抬笔点有没有几何，量的就是这件事）。
            GraphicsPolygonItem *tailPolygon = mTempPolygon ? dynamic_cast<GraphicsPolygonItem *>(mTempPolygon->deepCopy()) : nullptr;

            if (mTempPolygon)
            {
                removeItem(mTempPolygon);
                mTempPolygon = NULL;
            }

            // 按设置把这一笔换成简化版（点更少、多边形更少）
            if ((currentTool == StylusTool::Pen && Settings::settings()->boardSimplifyPenStrokes->get().toBool()) ||
                (currentTool == StylusTool::Marker && Settings::settings()->boardSimplifyMarkerStrokes->get().toBool()))
            {
                simplifyCurrentStroke();
            }

            if (tailPolygon)
            {
                addPolygonItemToCurrentStroke(tailPolygon);
            }

            GraphicsStrokesGroup *pStrokes = new GraphicsStrokesGroup();

            // 把刚才逐段画出来的临时多边形摘下来，收进一个笔迹组当作“一笔”
            foreach (GraphicsPolygonItem *poly, mCurrentStroke->polygons())
            {
                mPreviousPolygonItems.removeAll(poly);
                removeItem(poly);
                CoreGraphicsScene::removeItemFromDeletion(poly);
                poly->setStrokesGroup(pStrokes);
                pStrokes->addToGroup(poly);
            }

            // TODO: 压感插值不够平滑，起笔收笔处会出现明显的宽度台阶

            mAddedItems.clear();
            mAddedItems << pStrokes;
            addItem(pStrokes);

            if (mCurrentStroke->polygons().empty())
            {
                delete mCurrentStroke;
                mCurrentStroke = 0;
            }
            mCurrentPolygon = 0;
        }
    }

    if (mRemovedItems.size() > 0 || mAddedItems.size() > 0)
    {
        if (mUndoRedoStackEnabled)
        {                                                                                                   // 等场景自带撤销栈之后这段就该整体删掉
            GraphicsItemUndoCommand *udcmd = new GraphicsItemUndoCommand(this, mRemovedItems, mAddedItems); // deleted by the undoStack

            if (BoardController::undoStack())
            {
                BoardController::undoStack()->push(udcmd);
            }
        }

        mRemovedItems.clear();
        mAddedItems.clear();
        accepted = true;
    }

    mInputDeviceIsPressed = false;

    setDocumentUpdated();

    if (mCurrentStroke && mCurrentStroke->polygons().empty())
    {
        delete mCurrentStroke;
    }

    mCurrentStroke = NULL;
    return accepted;
}

void GraphicsScene::drawEraser(const QPointF &pPoint, bool pressed)
{
    if (mEraser)
    {
        qreal eraserWidth   = Settings::settings()->currentEraserWidth();
        eraserWidth        /= BoardController::instance()->systemScaleFactor();
        eraserWidth        /= BoardController::instance()->currentZoom();

        qreal eraserRadius  = eraserWidth / 2;

        mEraser->setRect(QRectF(pPoint.x() - eraserRadius, pPoint.y() - eraserRadius, eraserWidth, eraserWidth));
        redrawEraser(pressed);
    }
}

void GraphicsScene::redrawEraser(bool pressed)
{
    if (mEraser)
    {
        QPen pen = mEraser->pen();

        if (pressed)
        {
            pen.setStyle(Qt::SolidLine);
        }
        else
        {
            pen.setStyle(Qt::DotLine);
        }

        mEraser->setPen(pen);
        mEraser->show();
    }
}

void GraphicsScene::hideEraser()
{
    if (mEraser)
    {
        mEraser->hide();
    }
}

void GraphicsScene::drawPointer(const QPointF &pPoint, bool isFirstDraw)
{
    qreal pointerDiameter = Settings::pointerDiameter / BoardController::instance()->currentZoom();
    qreal pointerRadius   = pointerDiameter / 2;

    if (mPointer)
    {
        mPointer->setRect(QRectF(pPoint.x() - pointerRadius, pPoint.y() - pointerRadius, pointerDiameter, pointerDiameter));
        if (isFirstDraw)
        {
            mPointer->show();
        }
    }
}

void GraphicsScene::drawMarkerCircle(const QPointF &pPoint)
{
    if (mMarkerCircle)
    {
        qreal markerDiameter  = Settings::settings()->currentMarkerWidth();
        markerDiameter       /= BoardController::instance()->systemScaleFactor();
        markerDiameter       /= BoardController::instance()->currentZoom();
        qreal markerRadius    = markerDiameter / 2;

        mMarkerCircle->setRect(QRectF(pPoint.x() - markerRadius, pPoint.y() - markerRadius, markerDiameter, markerDiameter));
        mMarkerCircle->show();
    }
}

void GraphicsScene::drawPenCircle(const QPointF &pPoint)
{
    if (mPenCircle && Settings::settings()->showPenPreviewCircle->get().toBool() &&
        Settings::settings()->currentPenWidth() >= Settings::settings()->penPreviewFromSize->get().toInt())
    {
        qreal penDiameter  = Settings::settings()->currentPenWidth();
        penDiameter       /= BoardController::instance()->systemScaleFactor();
        penDiameter       /= BoardController::instance()->currentZoom();
        qreal penRadius    = penDiameter / 2;

        mPenCircle->setRect(QRectF(pPoint.x() - penRadius, pPoint.y() - penRadius, penDiameter, penDiameter));

        if (controlView())
        {
            if (controlView()->viewport())
            {
                controlView()->viewport()->setCursor(QCursor(Qt::BlankCursor));
            }
        }

        mPenCircle->show();
    }
    else
    {
        // 这一支也要把圈收掉：图元一直在场景里，只显示与否是每次悬笔重新算的，
        // 不算干净的话，取消勾选之后圈还挂在最后一次显示的位置上，要等指针离开这一页才收。
        hidePenCircle();

        if (controlView())
        {
            if (controlView()->viewport())
            {
                controlView()->viewport()->setCursor(Resources::resources()->penCursor);
            }
        }
    }
}

void GraphicsScene::hideMarkerCircle()
{
    if (mMarkerCircle)
    {
        mMarkerCircle->hide();
    }
}

void GraphicsScene::hidePenCircle()
{
    if (mPenCircle)
    {
        mPenCircle->hide();
    }
}

// 放大镜模式下松开鼠标时调用：临时控件用完就得拆掉
void GraphicsScene::DisposeMagnifierQWidgets()
{
    if (magnifierControlViewWidget)
    {
        magnifierControlViewWidget->hide();
        magnifierControlViewWidget->setParent(0);
        delete magnifierControlViewWidget;
        magnifierControlViewWidget = NULL;
    }

    if (magnifierDisplayViewWidget)
    {
        magnifierDisplayViewWidget->hide();
        magnifierDisplayViewWidget->setParent(0);
        delete magnifierDisplayViewWidget;
        magnifierDisplayViewWidget = NULL;
    }
    // 关程序时本函数可能从析构里被调到，那一刻 Application 已经没了；
    // 这里宁可吞掉异常，也不能让退出流程崩在恢复光标上。
    try
    {
        Application::app()->restoreOverrideCursor();
    }
    catch (...)
    {
    }
}

void GraphicsScene::moveTo(const QPointF &pPoint)
{
    mPreviousPoint = pPoint;
    mPreviousWidth = -1.0;
    mPreviousPolygonItems.clear();
    mArcPolygonItem  = 0;
    mDrawWithCompass = false;
}
void GraphicsScene::drawLineTo(const QPointF &pEndPoint, const qreal &pWidth, bool bLineStyle)
{
    drawLineTo(pEndPoint, pWidth, pWidth, bLineStyle);
}

void GraphicsScene::drawLineTo(const QPointF &pEndPoint, const qreal &startWidth, const qreal &endWidth, bool bLineStyle)
{
    if (mPreviousWidth == -1.0)
    {
        mPreviousWidth = startWidth;
    }

    qreal initialWidth = startWidth;
    if (initialWidth == endWidth)
    {
        initialWidth = mPreviousWidth;
    }

    if (bLineStyle)
    {
        QSetIterator<QGraphicsItem *> itItems(mAddedItems);

        while (itItems.hasNext())
        {
            QGraphicsItem *item = itItems.next();
            removeItem(item);
        }
        mAddedItems.clear();
    }

    GraphicsPolygonItem *polygonItem = lineToPolygonItem(QLineF(mPreviousPoint, pEndPoint), initialWidth, endWidth);
    addPolygonItemToCurrentStroke(polygonItem);

    if (!bLineStyle)
    {
        mPreviousPoint = pEndPoint;
        mPreviousWidth = endWidth;
    }
}

void GraphicsScene::drawCurve(const QList<QPair<QPointF, qreal>> &points)
{
    GraphicsPolygonItem *polygonItem = curveToPolygonItem(points);
    addPolygonItemToCurrentStroke(polygonItem);

    mPreviousPoint = points.last().first;
    mPreviousWidth = points.last().second;
}

void GraphicsScene::drawCurve(const QList<QPointF> &points, qreal startWidth, qreal endWidth)
{
    GraphicsPolygonItem *polygonItem = curveToPolygonItem(points, startWidth, endWidth);
    addPolygonItemToCurrentStroke(polygonItem);

    mPreviousWidth = endWidth;
    mPreviousPoint = points.last();
}

void GraphicsScene::addPolygonItemToCurrentStroke(GraphicsPolygonItem *polygonItem)
{
    if (!polygonItem->brush().isOpaque())
    {
        // 半透明笔迹自叠处会被重画成更深的一块，所以把先前那几段从新段里减掉，
        // 保证任何一点的覆盖层数都只有一次。
        for (int i = 0; i < mPreviousPolygonItems.size(); i++)
        {
            GraphicsPolygonItem *previous = mPreviousPolygonItems.value(i);
            polygonItem->subtract(previous);
        }
    }

    mpLastPolygon = polygonItem;
    mAddedItems.insert(polygonItem);

    addItem(polygonItem);
    if (!mCurrentStroke)
    {
        mCurrentStroke = new GraphicsStroke(this);
    }

    polygonItem->setStroke(mCurrentStroke);

    mPreviousPolygonItems.append(polygonItem);
}

void GraphicsScene::eraseLineTo(const QPointF &pEndPoint, const qreal &pWidth)
{
    const QLineF line(mPreviousPoint, pEndPoint);
    mPreviousPoint                     = pEndPoint;

    const QPolygonF eraserPolygon      = GeometryUtils::lineToPolygon(line, pWidth);
    const QRectF    eraserBoundingRect = eraserPolygon.boundingRect();

    QPainterPath eraserPath;
    eraserPath.addPolygon(eraserPolygon);

    // 先用包围矩形粗筛，再在下面按路径精确求交
    QList<QGraphicsItem *> collidItems = items(eraserBoundingRect, Qt::IntersectsItemBoundingRect);

    QList<GraphicsPolygonItem *> intersectedItems;

    typedef QList<QPolygonF> POLYGONSLIST;
    QList<POLYGONSLIST>      intersectedPolygons;

    // 这个循环就是串行的：整个构建没有配 OpenMP（CMake 里搜不到 openmp，exe 的字节里连
    // vcomp140.dll 这个名字都没有），原先那三处 #pragma omp 不起作用，留着会让人以为擦除是并行跑的。
    for (int i = 0; i < collidItems.size(); i++)
    {
        GraphicsPolygonItem *pi = qgraphicsitem_cast<GraphicsPolygonItem *>(collidItems[i]);
        if (pi == NULL)
        {
            continue;
        }

        QPainterPath itemPainterPath;
        itemPainterPath.addPolygon(pi->sceneTransform().map(pi->polygon()));

        if (eraserPath.contains(itemPainterPath))
        {
            // 整块都被擦掉了，不需要留残余
            intersectedItems << pi;
            intersectedPolygons << QList<QPolygonF>();
        }
        else if (eraserPath.intersects(itemPainterPath))
        {
            itemPainterPath.setFillRule(Qt::WindingFill);
            QPainterPath newPath = itemPainterPath.subtracted(eraserPath);

            intersectedItems << pi;
            intersectedPolygons << newPath.simplified().toFillPolygons(pi->sceneTransform().inverted());
        }
    }

    for (int i = 0; i < intersectedItems.size(); i++)
    {
        GraphicsPolygonItem *intersectedPolygonItem = intersectedItems[i];

        if (!intersectedPolygons[i].empty())
        {
            // toFillPolygons() 给的是擦除后剩下的若干块：被擦中间的图元会裂成两块以上，
            // 每块都要变成独立图元顶替原来那一个。
            for (int j = 0; j < intersectedPolygons[i].size(); j++)
            {
                GraphicsPolygonItem *polygonItem = new GraphicsPolygonItem(intersectedPolygons[i][j], intersectedPolygonItem->parentItem());

                intersectedPolygonItem->copyItemParameters(polygonItem);
                polygonItem->setNominalLine(false);
                polygonItem->setStroke(intersectedPolygonItem->stroke());
                polygonItem->setStrokesGroup(intersectedPolygonItem->strokesGroup());
                intersectedPolygonItem->strokesGroup()->addToGroup(polygonItem);
                mAddedItems << polygonItem;
            }
        }

        // 原图元整块删除，由上面那几块残片接棒，视觉上就是“被擦掉一段”
        mRemovedItems << intersectedPolygonItem;

        QTransform t;
        bool       bApplyTransform = false;
        if (intersectedPolygonItem->strokesGroup())
        {
            if (intersectedPolygonItem->strokesGroup()->parentItem())
            {
                bApplyTransform = true;
                t               = intersectedPolygonItem->sceneTransform();
            }
            intersectedPolygonItem->strokesGroup()->removeFromGroup(intersectedPolygonItem);
        }
        removeItem(intersectedPolygonItem);
        if (bApplyTransform)
        {
            intersectedPolygonItem->setTransform(t);
        }
    }

    if (!intersectedItems.empty())
    {
        setModified(true);
    }
}

void GraphicsScene::drawArcTo(const QPointF &pCenterPoint, qreal pSpanAngle)
{
    mDrawWithCompass = true;
    if (mArcPolygonItem)
    {
        mAddedItems.remove(mArcPolygonItem);
        removeItem(mArcPolygonItem);
        mArcPolygonItem = 0;
    }
    qreal penWidth   = Settings::settings()->currentPenWidth();
    penWidth        /= BoardController::instance()->systemScaleFactor();
    penWidth        /= BoardController::instance()->currentZoom();

    mArcPolygonItem  = arcToPolygonItem(QLineF(pCenterPoint, mPreviousPoint), pSpanAngle, penWidth);
    mArcPolygonItem->setFillRule(Qt::WindingFill);
    mArcPolygonItem->setStroke(mCurrentStroke);
    mAddedItems.insert(mArcPolygonItem);
    addItem(mArcPolygonItem);

    setDocumentUpdated();
}

void GraphicsScene::setBackground(bool pIsDark, PageBackground pBackground)
{
    bool needRepaint = false;

    if (mDarkBackground != pIsDark)
    {
        mDarkBackground = pIsDark;

        updateEraserColor();
        updateMarkerCircleColor();
        updatePenCircleColor();
        recolorAllItems();

        needRepaint = true;
        setModified(true);
    }

    if (mPageBackground != pBackground)
    {
        mPageBackground = pBackground;
        needRepaint     = true;
        setModified(true);
    }

    if (needRepaint)
    {
        foreach (QGraphicsView *view, views())
        {
            view->resetCachedContent();
        }
    }
}

void GraphicsScene::setBackgroundZoomFactor(qreal zoom)
{
    mZoomFactor = zoom;
}

void GraphicsScene::setBackgroundGridSize(int pSize)
{
    if (pSize > 0)
    {
        mBackgroundGridSize = pSize;
        setModified(true);

        foreach (QGraphicsView *view, views())
        {
            view->resetCachedContent();
        }
    }
}

void GraphicsScene::setDrawingMode(bool bModeDesktop)
{
    mIsDesktopMode = bModeDesktop;
}

void GraphicsScene::recolorAllItems()
{
    QMap<QGraphicsView *, QGraphicsView::ViewportUpdateMode> previousUpdateModes;
    foreach (QGraphicsView *view, views())
    {
        previousUpdateModes.insert(view, view->viewportUpdateMode());
        view->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    }

    bool currentIslight = isLightBackground();
    foreach (QGraphicsItem *item, items())
    {
        if (item->type() == GraphicsStrokesGroup::Type)
        {
            GraphicsStrokesGroup *curGroup = static_cast<GraphicsStrokesGroup *>(item);
            QColor                compareColor =
                curGroup->color(currentIslight ? GraphicsStrokesGroup::colorOnDarkBackground : GraphicsStrokesGroup::colorOnLightBackground);

            if (curGroup->color() == compareColor)
            {
                QColor newColor =
                    curGroup->color(!currentIslight ? GraphicsStrokesGroup::colorOnDarkBackground : GraphicsStrokesGroup::colorOnLightBackground);
                curGroup->setColor(newColor);
            }
        }

        if (item->type() == GraphicsTextItem::Type)
        {
            GraphicsTextItem *textItem = static_cast<GraphicsTextItem *>(item);
            textItem->recolor();
        }
    }

    foreach (QGraphicsView *view, views())
    {
        view->setViewportUpdateMode(previousUpdateModes.value(view));
    }
}

GraphicsPolygonItem *GraphicsScene::lineToPolygonItem(const QLineF &pLine, const qreal &pWidth)
{
    GraphicsPolygonItem *polygonItem = new GraphicsPolygonItem(pLine, pWidth);

    initPolygonItem(polygonItem);

    return polygonItem;
}

GraphicsPolygonItem *GraphicsScene::lineToPolygonItem(const QLineF &pLine, const qreal &pStartWidth, const qreal &pEndWidth)
{
    GraphicsPolygonItem *polygonItem = new GraphicsPolygonItem(pLine, pStartWidth, pEndWidth);

    initPolygonItem(polygonItem);

    return polygonItem;
}

void GraphicsScene::initPolygonItem(GraphicsPolygonItem *polygonItem)
{
    QColor colorOnDarkBG;
    QColor colorOnLightBG;

    if (DrawingController::drawingController()->stylusTool() == StylusTool::Marker)
    {
        colorOnDarkBG  = BoardController::instance()->markerColorOnDarkBackground();
        colorOnLightBG = BoardController::instance()->markerColorOnLightBackground();
    }
    else // 判的是“当前工具等于画笔”，再兜一层底，取不到工具时不误判
    {
        colorOnDarkBG  = BoardController::instance()->penColorOnDarkBackground();
        colorOnLightBG = BoardController::instance()->penColorOnLightBackground();
    }

    if (mDarkBackground)
    {
        polygonItem->setColor(colorOnDarkBG);
    }
    else
    {
        polygonItem->setColor(colorOnLightBG);
    }

    polygonItem->setColorOnDarkBackground(colorOnDarkBG);
    polygonItem->setColorOnLightBackground(colorOnLightBG);

    polygonItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Graphic));
}

GraphicsPolygonItem *GraphicsScene::arcToPolygonItem(const QLineF &pStartRadius, qreal pSpanAngle, qreal pWidth)
{
    QPolygonF polygon = GeometryUtils::arcToPolygon(pStartRadius, pSpanAngle, pWidth);

    return polygonToPolygonItem(polygon);
}

GraphicsPolygonItem *GraphicsScene::curveToPolygonItem(const QList<QPair<QPointF, qreal>> &points)
{
    QPolygonF polygon = GeometryUtils::curveToPolygon(points, false, true);

    return polygonToPolygonItem(polygon);
}

GraphicsPolygonItem *GraphicsScene::curveToPolygonItem(const QList<QPointF> &points, qreal startWidth, qreal endWidth)
{
    QPolygonF polygon = GeometryUtils::curveToPolygon(points, startWidth, endWidth);

    return polygonToPolygonItem(polygon);
}

void GraphicsScene::clearSelectionFrame()
{
    if (mSelectionFrame)
    {
        mSelectionFrame->setEnclosedItems(QList<QGraphicsItem *>());
    }
}

BoardView *GraphicsScene::controlView()
{
    BoardView *result = 0;
    foreach (QGraphicsView *view, views())
    {
        if (view->objectName() == CONTROLVIEW_OBJ_NAME)
        {
            result = static_cast<BoardView *>(view);
        }
    }

    return result;
}

void GraphicsScene::notifyZChanged(QGraphicsItem *item, qreal zValue)
{
    mZLayerController->shiftStoredZValue(item, zValue);
}

void GraphicsScene::updateSelectionFrame()
{
    if (!mSelectionFrame)
    {
        mSelectionFrame = new SelectionFrame();
        addItem(mSelectionFrame);
    }

    QList<QGraphicsItem *> selItems = selectedItems();
    switch (selItems.count())
    {
        case 0:
        {
            mSelectionFrame->setVisible(false);
            mSelectionFrame->setEnclosedItems(selItems);
        }
        break;
        case 1:
        {
            mSelectionFrame->setVisible(false);
            mSelectionFrame->setEnclosedItems(QList<QGraphicsItem *>());

            // 选择框（SelectionFrame）本身也能被选中，但它没有 Delegate；
            // 不判空的话单选到它时这里直接崩。
            if (GraphicsItemDelegate *itemDelegate = GraphicsItem::Delegate(selItems.first()))
            {
                itemDelegate->createControls();
                selItems.first()->setVisible(true);
                itemDelegate->showControls();
            }
        }
        break;
        default:
        {
            mSelectionFrame->setVisible(true);
            mSelectionFrame->setEnclosedItems(selItems);
        }
        break;
    }
}

void GraphicsScene::updateSelectionFrameWrapper(int)
{
    updateSelectionFrame();
}

GraphicsPolygonItem *GraphicsScene::polygonToPolygonItem(const QPolygonF pPolygon)
{
    GraphicsPolygonItem *polygonItem = new GraphicsPolygonItem(pPolygon);

    initPolygonItem(polygonItem);

    return polygonItem;
}

void GraphicsScene::hideTool()
{
    hideEraser();
    hideMarkerCircle();
    hidePenCircle();
}

void GraphicsScene::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    hideTool();
}

GraphicsScene *GraphicsScene::sceneDeepCopy() const
{
    GraphicsScene *copy = new GraphicsScene(this->document(), this->mUndoRedoStackEnabled);

    copy->setBackground(this->isDarkBackground(), mPageBackground);
    copy->setBackgroundGridSize(mBackgroundGridSize);
    copy->setSceneRect(this->sceneRect());

    if (this->mNominalSize.isValid())
    {
        copy->setNominalSize(this->mNominalSize);
    }

    QListIterator<QGraphicsItem *>           itItems(this->mFastAccessItems);
    QMap<GraphicsStroke *, GraphicsStroke *> groupClone;

    while (itItems.hasNext())
    {
        QGraphicsItem              *item      = itItems.next();
        QGraphicsItem              *cloneItem = 0;
        Item                       *boardItem = dynamic_cast<Item *>(item);
        GraphicsStroke             *stroke    = dynamic_cast<GraphicsStroke *>(item);
        GraphicsGroupContainerItem *group     = dynamic_cast<GraphicsGroupContainerItem *>(item);

        if (group)
        {
            GraphicsGroupContainerItem *groupCloned = group->deepCopyNoChildDuplication();
            groupCloned->resetTransform();
            groupCloned->setPos(0, 0);
            bool locked = groupCloned->Delegate()->isLocked();

            foreach (QGraphicsItem *eachItem, group->childItems())
            {
                QGraphicsItem *copiedChild = dynamic_cast<QGraphicsItem *>(dynamic_cast<Item *>(eachItem)->deepCopy());
                copy->addItem(copiedChild);
                groupCloned->addToGroup(copiedChild);
            }

            if (locked)
            {
                groupCloned->setData(GraphicsItemData::ItemLocked, QVariant(true));
            }

            copy->addItem(groupCloned);
            groupCloned->setTransform(group->transform());
            groupCloned->setTransform(QTransform::fromTranslate(group->pos().x(), group->pos().y()));
            groupCloned->setTransform(group->transform(), true);
        }

        if (boardItem && !stroke && !group && item->isVisible())
        {
            cloneItem = dynamic_cast<QGraphicsItem *>(boardItem->deepCopy());
        }

        if (cloneItem)
        {
            copy->addItem(cloneItem);

            if (isBackgroundObject(item))
            {
                copy->setAsBackgroundObject(cloneItem);
            }

            if (this->mTools.contains(item))
            {
                copy->mTools << cloneItem;
            }

            GraphicsPolygonItem *polygon = dynamic_cast<GraphicsPolygonItem *>(item);

            if (polygon)
            {
                // 多边形挂在哪一笔上要看父图元，与循环开头那个 stroke 不是一回事，故另起一名。
                GraphicsStroke *parentStroke = dynamic_cast<GraphicsStroke *>(item->parentItem());

                if (parentStroke)
                {
                    GraphicsStroke *cloneStroke = groupClone.value(parentStroke);

                    if (!cloneStroke)
                    {
                        cloneStroke = parentStroke->deepCopy();
                        groupClone.insert(parentStroke, cloneStroke);
                    }

                    polygon->setStroke(cloneStroke);
                }
            }
        }
    }

    return copy;
}

Item *GraphicsScene::deepCopy() const
{
    return sceneDeepCopy();
}

void GraphicsScene::clearContent(clearCase pCase)
{
    QSet<QGraphicsItem *>                   removedItems;
    GraphicsItemUndoCommand::GroupDataTable groupsMap;

    switch (pCase)
    {
        case clearBackground:
            if (mBackgroundObject)
            {
                removeItem(mBackgroundObject);
                removedItems << mBackgroundObject;
            }
            break;

        case clearItemsAndAnnotations:
        case clearItems:
        case clearAnnotations:
            foreach (QGraphicsItem *item, items())
            {
                GraphicsGroupContainerItem *itemGroup = item->parentItem() ? qgraphicsitem_cast<GraphicsGroupContainerItem *>(item->parentItem()) : 0;
                GraphicsItemDelegate       *curDelegate = GraphicsItem::Delegate(item);
                if (!curDelegate)
                {
                    continue;
                }

                bool isGroup        = item->type() == GraphicsGroupContainerItem::Type;
                bool isStrokesGroup = item->type() == GraphicsStrokesGroup::Type;

                bool shouldDelete   = false;
                switch (static_cast<int>(pCase))
                {
                    case clearAnnotations:
                        shouldDelete = isStrokesGroup;
                        break;
                    case clearItems:
                        shouldDelete = !isGroup && !isBackgroundObject(item) && !isStrokesGroup;
                        break;
                    case clearItemsAndAnnotations:
                        shouldDelete = !isGroup && !isBackgroundObject(item);
                        break;
                }

                if (shouldDelete)
                {
                    if (itemGroup)
                    {
                        itemGroup->removeFromGroup(item);

                        groupsMap.insert(itemGroup, GraphicsItem::getOwnUuid(item));
                        if (itemGroup->childItems().count() == 1)
                        {
                            groupsMap.insert(itemGroup, GraphicsItem::getOwnUuid(itemGroup->childItems().first()));
                            QGraphicsItem *lastItem   = itemGroup->childItems().first();
                            bool           isSelected = itemGroup->isSelected();
                            itemGroup->destroy(false);
                            lastItem->setSelected(isSelected);
                        }
                        itemGroup->Delegate()->update();
                    }

                    curDelegate->remove(false);
                    removedItems << item;
                }
            }
            break;
    }

    // 整场景强制重画：只标脏改动区域的话，Qt 合并脏矩形会拖很久才刷新
    update(sceneRect());

    if (mUndoRedoStackEnabled)
    { // 等场景自带撤销栈之后这段就该整体删掉

        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, removedItems, QSet<QGraphicsItem *>(), groupsMap);
        BoardController::undoStack()->push(uc);
    }

    if (pCase == clearBackground)
    {
        mBackgroundObject = 0;
    }

    setDocumentUpdated();
}

GraphicsPixmapItem *GraphicsScene::addPixmap(
    const QPixmap &pPixmap, QGraphicsItem *replaceFor, const QPointF &pPos, qreal pScaleFactor, bool pUseAnimation, bool useProxyForDocumentPath)
{
    GraphicsPixmapItem *pixmapItem = new GraphicsPixmapItem();

    pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    pixmapItem->setPixmap(pPixmap);

    QPointF half(pPixmap.width() * pScaleFactor / 2, pPixmap.height() * pScaleFactor / 2);
    pixmapItem->setPos(pPos - half);

    addItem(pixmapItem);

    if (mUndoRedoStackEnabled)
    { // 等场景自带撤销栈之后这段就该整体删掉
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, replaceFor, pixmapItem);
        BoardController::undoStack()->push(uc);
    }

    pixmapItem->setTransform(QTransform::fromScale(pScaleFactor, pScaleFactor), true);

    if (pUseAnimation)
    {
        pixmapItem->setOpacity(0);

        QPropertyAnimation *animation = new QPropertyAnimation(pixmapItem, "opacity");
        animation->setDuration(1000);
        animation->setStartValue(0.0);
        animation->setEndValue(1.0);

        animation->start();
    }

    pixmapItem->show();
    setDocumentUpdated();

    QString documentPath;
    if (useProxyForDocumentPath)
    {
        documentPath = this->document()->persistencePath();
    }
    else
    {
        documentPath = BoardController::instance()->selectedDocument()->persistencePath();
    }

    QString fileName = PackageLayout::imageDirectory + "/" + pixmapItem->uuid().toString() + ".png";

    QString path     = documentPath + "/" + fileName;

    if (!QFile::exists(path))
    {
        QDir dir;
        dir.mkdir(documentPath + "/" + PackageLayout::imageDirectory);

        pixmapItem->pixmap().toImage().save(path, "PNG");
    }

    return pixmapItem;
}

void GraphicsScene::textUndoCommandAdded(GraphicsTextItem *textItem)
{
    if (mUndoRedoStackEnabled)
    { // 等场景自带撤销栈之后这段就该整体删掉
        GraphicsTextItemUndoCommand *uc = new GraphicsTextItemUndoCommand(textItem);
        BoardController::undoStack()->push(uc);
    }
}
GraphicsMediaItem *GraphicsScene::addMedia(const QUrl &pMediaFileUrl, bool shouldPlayAsap, const QPointF &pPos)
{
    qDebug() << pMediaFileUrl.toLocalFile();

    // 本地路径与 URL 两种写法都试过再放弃（网络地址只后一种有意义）
    if (!QFile::exists(pMediaFileUrl.toLocalFile()) && !QFile::exists(pMediaFileUrl.toString()))
    {
        return NULL;
    }

    GraphicsMediaItem *mediaItem = GraphicsMediaItem::createMediaItem(pMediaFileUrl);

    // createMediaItem 对不支持的媒体类型返回空；原来只在 connect 前判空，
    // 紧接着就无条件 setPos，等于拖一个不支持的文件进来直接崩。
    if (!mediaItem)
    {
        return NULL;
    }

    connect(BoardController::instance(), &BoardController::activeSceneChanged, mediaItem, &GraphicsMediaItem::activeSceneChanged);

    mediaItem->setPos(pPos);

    mediaItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    mediaItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    addItem(mediaItem);

    mediaItem->show();

    if (mUndoRedoStackEnabled)
    { // 等场景自带撤销栈之后这段就该整体删掉
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, mediaItem);
        BoardController::undoStack()->push(uc);
    }

    if (shouldPlayAsap)
    {
        mediaItem->play();
    }

    setDocumentUpdated();

    return mediaItem;
}

GraphicsMediaItem *GraphicsScene::addVideo(const QUrl &pVideoFileUrl, bool shouldPlayAsap, const QPointF &pPos)
{
    return addMedia(pVideoFileUrl, shouldPlayAsap, pPos);
}

GraphicsMediaItem *GraphicsScene::addAudio(const QUrl &pAudioFileUrl, bool shouldPlayAsap, const QPointF &pPos)
{
    return addMedia(pAudioFileUrl, shouldPlayAsap, pPos);
}

GraphicsWidgetItem *GraphicsScene::addWidget(const QUrl &pWidgetUrl, const QPointF &pPos)
{
    int widgetType = GraphicsWidgetItem::widgetType(pWidgetUrl);

    if (widgetType == WidgetType::Apple)
    {
        return addAppleWidget(pWidgetUrl, pPos);
    }
    else if (widgetType == WidgetType::Web)
    {
        return addWebWidget(pWidgetUrl, pPos);
    }
    else
    {
        qDebug() << "GraphicsScene::addWidget: Unknown widget Type";
        return 0;
    }
}

GraphicsAppleWidgetItem *GraphicsScene::addAppleWidget(const QUrl &pWidgetUrl, const QPointF &pPos)
{
    GraphicsAppleWidgetItem *appleWidget = new GraphicsAppleWidgetItem(pWidgetUrl);

    addGraphicsWidget(appleWidget, pPos);

    return appleWidget;
}

GraphicsWebWidgetItem *GraphicsScene::addWebWidget(const QUrl &pWidgetUrl, const QPointF &pPos)
{
    GraphicsWebWidgetItem *webWidget = new GraphicsWebWidgetItem(pWidgetUrl, 0);

    addGraphicsWidget(webWidget, pPos);

    return webWidget;
}

void GraphicsScene::addGraphicsWidget(GraphicsWidgetItem *graphicsWidget, const QPointF &pPos)
{
    graphicsWidget->setFlag(QGraphicsItem::ItemIsSelectable, true);

    addItem(graphicsWidget);

    qreal ssf = 1 / BoardController::instance()->systemScaleFactor();

    graphicsWidget->setTransform(QTransform::fromScale(ssf, ssf), true);

    graphicsWidget->setPos(QPointF(pPos.x() - graphicsWidget->boundingRect().width() / 2, pPos.y() - graphicsWidget->boundingRect().height() / 2));

    if (graphicsWidget->canBeContent())
    {
        graphicsWidget->loadMainHtml();

        graphicsWidget->setSelected(true);
        if (mUndoRedoStackEnabled)
        { // 等场景自带撤销栈之后这段就该整体删掉
            GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, graphicsWidget);
            BoardController::undoStack()->push(uc);
        }

        setDocumentUpdated();
    }
    else
    {
        BoardController::instance()->moveGraphicsWidgetToControlView(graphicsWidget);
    }

    BoardController::instance()->controlView()->setFocus();
}

GraphicsGroupContainerItem *GraphicsScene::createGroup(QList<QGraphicsItem *> items)
{
    GraphicsGroupContainerItem *groupItem = new GraphicsGroupContainerItem();

    addItem(groupItem);
    foreach (QGraphicsItem *item, items)
    {
        if (!item)
        {
            // 调用方可能按 UUID 找图元，找不到就是 0；这里必须挡住，否则下面 item->type() 直接崩。
            qWarning() << "createGroup: ignoring a null member item";
            continue;
        }

        if (item->type() == GraphicsGroupContainerItem::Type)
        {
            QList<QGraphicsItem *>      childItems   = item->childItems();
            GraphicsGroupContainerItem *currentGroup = dynamic_cast<GraphicsGroupContainerItem *>(item);
            if (currentGroup)
            {
                currentGroup->destroy();
            }
            foreach (QGraphicsItem *chItem, childItems)
            {
                groupItem->addToGroup(chItem);
                mFastAccessItems.removeAll(chItem);
            }
        }
        else
        {
            groupItem->addToGroup(item);
            mFastAccessItems.removeAll(item);
        }
    }

    groupItem->setVisible(true);
    groupItem->setFocus();

    if (mUndoRedoStackEnabled)
    { // 等场景自带撤销栈之后这段就该整体删掉
        GraphicsItemGroupUndoCommand *uc = new GraphicsItemGroupUndoCommand(this, groupItem);
        BoardController::undoStack()->push(uc);
    }

    setDocumentUpdated();

    return groupItem;
}

void GraphicsScene::addGroup(GraphicsGroupContainerItem *groupItem)
{
    addItem(groupItem);
    for (int i = 0; i < groupItem->childItems().count(); i++)
    {
        QGraphicsItem *it = qgraphicsitem_cast<QGraphicsItem *>(groupItem->childItems().at(i));
        if (it)
        {
            mFastAccessItems.removeAll(it);
        }
    }

    groupItem->setVisible(true);
    groupItem->setFocus();

    if (groupItem->uuid().isNull())
    {
        groupItem->setUuid(QUuid::createUuid());
    }

    if (mUndoRedoStackEnabled)
    { // 等场景自带撤销栈之后这段就该整体删掉
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, groupItem);
        BoardController::undoStack()->push(uc);
    }

    setDocumentUpdated();
}

GraphicsSvgItem *GraphicsScene::addSvg(const QUrl &pSvgFileUrl, const QPointF &pPos, const QByteArray pData)
{
    QString path = pSvgFileUrl.toLocalFile();

    GraphicsSvgItem *svgItem;
    if (pData.isNull())
    {
        svgItem = new GraphicsSvgItem(path);
    }
    else
    {
        svgItem = new GraphicsSvgItem(pData);
    }

    svgItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    svgItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    qreal sscale = 1 / BoardController::instance()->systemScaleFactor();
    svgItem->setTransform(QTransform::fromScale(sscale, sscale), true);

    QPointF half(svgItem->boundingRect().width() / 2, svgItem->boundingRect().height() / 2);
    svgItem->setPos(pPos - half);
    svgItem->show();

    // 先把内嵌 svg 落盘再入场景：写失败时若已经 addItem 并压了 undo 命令，
    // 场景里会留下一条永远不会被持久化的条目，调用方拿到的却还是 NULL。
    // 落点必须按"这一页属于哪份文档"算：导入别的文档时画板当前选中的是另一份，
    // 跟着选中走会把资源写进无关文档，这一页再读就找不到图元（实测过）。
    DocumentProxy *ownerDocument = mDocument ? mDocument : BoardController::instance()->selectedDocument();
    QString        documentPath  = ownerDocument ? ownerDocument->persistencePath() : QString();

    QString fileName             = PackageLayout::imageDirectory + "/" + svgItem->uuid().toString() + ".svg";

    QString completePath         = documentPath + "/" + fileName;

    if (!documentPath.isEmpty() && !QFile::exists(completePath))
    {
        QDir dir;
        dir.mkdir(documentPath + "/" + PackageLayout::imageDirectory);

        QFile file(completePath);
        if (!file.open(QIODevice::WriteOnly))
        {
            qWarning() << "内嵌 SVG 没法落盘，图元将被丢弃:" << completePath;
            delete svgItem;
            return nullptr;
        }

        file.write(svgItem->fileData());
        file.close();
    }
    else if (documentPath.isEmpty())
    {
        qWarning() << "场景没有归属文档，内嵌 SVG 只能留在内存里，保存后这一页会丢图元";
    }

    addItem(svgItem);

    if (mUndoRedoStackEnabled)
    { // 等场景自带撤销栈之后这段就该整体删掉
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, svgItem);
        BoardController::undoStack()->push(uc);
    }

    setDocumentUpdated();

    return svgItem;
}

GraphicsTextItem *GraphicsScene::addText(const QString &pString, const QPointF &pTopLeft)
{
    return addTextWithFont(pString, pTopLeft, Settings::settings()->fontPixelSize(), Settings::settings()->fontFamily(),
        Settings::settings()->isBoldFont(), Settings::settings()->isItalicFont());
}

GraphicsTextItem *GraphicsScene::addTextWithFont(
    const QString &pString, const QPointF &pTopLeft, int pointSize, const QString &fontFamily, bool bold, bool italic)
{
    GraphicsTextItem *textItem = new GraphicsTextItem();
    textItem->setPlainText(pString);

    QFont font = textItem->font();

    if (fontFamily == "")
    {
        font = QFont(Settings::settings()->fontFamily());
    }
    else
    {
        font = QFont(fontFamily);
    }

    if (pointSize < 1)
    {
        font.setPixelSize(Settings::settings()->fontPixelSize());
    }
    else
    {
        font.setPointSize(pointSize);
    }

    font.setBold(bold);
    font.setItalic(italic);
    textItem->setFont(font);

    QFontMetrics fi(font);
    QRect        br = fi.boundingRect(pString);

    textItem->setTextWidth(qMax((qreal)br.width() + 50, (qreal)200));
    textItem->setTextHeight(br.height());

    addItem(textItem);

    textItem->setPos(pTopLeft);

    textItem->show();

    if (mUndoRedoStackEnabled)
    { // 等场景自带撤销栈之后这段就该整体删掉
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, textItem);
        BoardController::undoStack()->push(uc);
    }

    connect(textItem, &GraphicsTextItem::textUndoCommandAdded, this, &GraphicsScene::textUndoCommandAdded);

    textItem->setSelected(true);
    textItem->setFocus();

    setDocumentUpdated();

    return textItem;
}

GraphicsTextItem *GraphicsScene::addTextHtml(const QString &pString, const QPointF &pTopLeft)
{
    GraphicsTextItem *textItem = new GraphicsTextItem();
    textItem->setPlainText("");
    textItem->setHtml(TextTools::cleanHtml(pString));

    addItem(textItem);
    textItem->show();

    if (mUndoRedoStackEnabled)
    { // 等场景自带撤销栈之后这段就该整体删掉
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, textItem);
        BoardController::undoStack()->push(uc);
    }

    connect(textItem, &GraphicsTextItem::textUndoCommandAdded, this, &GraphicsScene::textUndoCommandAdded);

    textItem->setFocus();

    setDocumentUpdated();
    textItem->setPos(pTopLeft);

    return textItem;
}

void GraphicsScene::addItem(QGraphicsItem *item)
{
    CoreGraphicsScene::addItem(item);

    // 带着默认 Z 进来的（例如从 SVG 读入）或值已被占用的都要重新分层；
    // 已经分好层的不必再动，否则导入文档的叠放次序会被打乱。
    if (item->zValue() == DEFAULT_Z_VALUE || item->zValue() == ZLayerController::errorNum() || !mZLayerController->zLevelAvailable(item->zValue()))
    {
        qreal zvalue = mZLayerController->generateZLevel(item);
        GraphicsItem::assignZValue(item, zvalue);
    }

    else
    {
        notifyZChanged(item, item->zValue());
    }

    if (!mTools.contains(item))
    {
        ++mItemCount;
    }

    mFastAccessItems << item;
}

void GraphicsScene::addItems(const QSet<QGraphicsItem *> &items)
{
    foreach (QGraphicsItem *item, items)
    {
        CoreGraphicsScene::addItem(item);
        GraphicsItem::assignZValue(item, mZLayerController->generateZLevel(item));
    }

    mItemCount       += items.size();

    mFastAccessItems += items.values();
}

void GraphicsScene::removeItem(QGraphicsItem *item)
{
    item->setSelected(false);
    CoreGraphicsScene::removeItem(item);
    BoardController::instance()->freezeWebWidget(item, true);

    if (!mTools.contains(item))
    {
        --mItemCount;
    }

    mFastAccessItems.removeAll(item);
    // 缓存图元是单例式的：只从场景摘掉不够，必须真删，否则再加载同一份缓存拿到的是旧对象。
    if (dynamic_cast<GraphicsCache *>(item))
    {
        CoreGraphicsScene::deleteItem(item);
    }
}

void GraphicsScene::removeItems(const QSet<QGraphicsItem *> &items)
{
    foreach (QGraphicsItem *item, items)
    {
        CoreGraphicsScene::removeItem(item);
    }

    mItemCount -= items.size();

    foreach (QGraphicsItem *item, items)
    {
        mFastAccessItems.removeAll(item);
    }
}

void GraphicsScene::deselectAllItems()
{
    foreach (QGraphicsItem *gi, selectedItems())
    {
        gi->clearFocus();
        gi->setSelected(false);
        // 取消选中要把选中框一起收掉，否则按钮还挂在空选中上
        if (mSelectionFrame)
        {
            mSelectionFrame->setEnclosedItems(QList<QGraphicsItem *>());
        }
        GraphicsTextItem *textItem = dynamic_cast<GraphicsTextItem *>(gi);
        if (textItem)
        {
            textItem->activateTextEditor(false);
        }
    }
}

void GraphicsScene::deselectAllItemsExcept(QGraphicsItem *item)
{
    foreach (QGraphicsItem *eachItem, selectedItems())
    {
        if (eachItem != item)
        {
            eachItem->setSelected(false);

            GraphicsTextItem *textItem = dynamic_cast<GraphicsTextItem *>(eachItem);
            if (textItem)
            {
                textItem->activateTextEditor(false);
            }
        }
    }
}

// 只算批注内容的包围盒：尺子、圆规这类工具不属于页面内容，不该把盒子撑大。
QRectF GraphicsScene::annotationsBoundingRect() const
{
    QRectF boundingRect;

    foreach (QGraphicsItem *item, items())
    {
        if (!mTools.contains(rootItem(item)))
        {
            boundingRect |= item->sceneBoundingRect();
        }
    }

    return boundingRect;
}

bool GraphicsScene::isEmpty() const
{
    return mItemCount == 0;
}

QGraphicsItem *GraphicsScene::setAsBackgroundObject(QGraphicsItem *item, bool pAdaptTransformation, bool pExpand)
{
    if (mBackgroundObject)
    {
        removeItem(mBackgroundObject);
        mBackgroundObject = 0;
    }

    if (item)
    {
        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        item->setFlag(QGraphicsItem::ItemIsMovable, false);
        item->setAcceptedMouseButtons(Qt::NoButton);
        item->setData(GraphicsItemData::ItemLayerType, ItemLayerType::FixedBackground);

        if (pAdaptTransformation)
        {
            item = scaleToFitDocumentSize(item, true, 0, pExpand);
        }

        if (item->scene() != this)
        {
            addItem(item);
        }

        mZLayerController->setLayerType(item, itemLayerType::BackgroundItem);
        GraphicsItem::assignZValue(item, mZLayerController->generateZLevel(item));

        mBackgroundObject = item;
    }

    return item;
}

void GraphicsScene::unsetBackgroundObject()
{
    if (!mBackgroundObject)
    {
        return;
    }

    mBackgroundObject->setFlag(QGraphicsItem::ItemIsSelectable, true);
    mBackgroundObject->setFlag(QGraphicsItem::ItemIsMovable, true);
    mBackgroundObject->setAcceptedMouseButtons(Qt::LeftButton);

    // 层级与图层类型留给调用方决定：同一个图元回到普通层还是背景层，取决于它原来在哪。
    mBackgroundObject = 0;
}

QRectF GraphicsScene::normalizedSceneRect(qreal ratio)
{
    QRectF normalizedRect(nominalSize().width() / -2, nominalSize().height() / -2, nominalSize().width(), nominalSize().height());

    foreach (QGraphicsItem *gi, mFastAccessItems)
    {
        if (gi && gi->isVisible() && !mTools.contains(gi))
        {
            normalizedRect = normalizedRect.united(gi->sceneBoundingRect());
        }
    }

    if (ratio > 0.0)
    {
        qreal normalizedRectRatio = normalizedRect.width() / normalizedRect.height();

        if (normalizedRectRatio > ratio)
        {
            // 归一化后的框太宽：按目标比例补高，左右不动，上下对称外扩
            qreal newHeight = normalizedRect.width() / ratio;
            qreal offset    = (newHeight - normalizedRect.height()) / 2;
            normalizedRect.setY(normalizedRect.y() - offset);
            normalizedRect.setHeight(newHeight);
        }
        else if (normalizedRectRatio < ratio)
        {
            // 太窄：反过来补宽
            qreal newWidth = normalizedRect.height() * ratio;
            qreal offset   = (newWidth - normalizedRect.width()) / 2;
            normalizedRect.setX(normalizedRect.x() - offset);
            normalizedRect.setWidth(newWidth);
        }
    }

    return normalizedRect;
}

QGraphicsItem *GraphicsScene::itemForUuid(QUuid uuid)
{
    QGraphicsItem *result = 0;
    QString        ui     = uuid.toString();

    // 线性扫一遍：图元多了之后该换按 UUID 索引的容器，现在还没有
    foreach (QGraphicsItem *item, items())
    {
        if (GraphicsScene::getPersonalUuid(item) == uuid && !uuid.isNull())
        {
            result = item;
        }
    }

    return result;
}

void GraphicsScene::setDocument(DocumentProxy *pDocument)
{
    if (pDocument != mDocument)
    {
        if (mDocument)
        {
            setModified(true);
        }

        mDocument = pDocument;
        setParent(pDocument);
    }
}

QGraphicsItem *GraphicsScene::scaleToFitDocumentSize(QGraphicsItem *item, bool center, int margin, bool expand)
{
    int maxWidth  = mNominalSize.width() - (margin * 2);
    int maxHeight = mNominalSize.height() - (margin * 2);

    QRectF size   = item->sceneBoundingRect();

    if (expand || size.width() > maxWidth || size.height() > maxHeight)
    {
        qreal ratio = qMin(maxWidth / size.width(), maxHeight / size.height());

        item->setTransform(QTransform::fromScale(ratio, ratio), true);

        if (center)
        {
            item->setPos(item->sceneBoundingRect().width() / -2.0, item->sceneBoundingRect().height() / -2.0);
        }
    }

    return item;
}

void GraphicsScene::addRuler(QPointF center)
{
    GraphicsRuler *ruler = new GraphicsRuler(); // 内存交给场景，随场景一起销毁
    mTools << ruler;
    QRectF rect = ruler->rect();
    ruler->setRect(center.x() - rect.width() / 2, center.y() - rect.height() / 2, rect.width(), rect.height());

    ruler->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    addItem(ruler);

    ruler->setVisible(true);
}

void GraphicsScene::addProtractor(QPointF center)
{
    // 量角器是工具图元：由场景持有并销毁，同时记进 mTools 以便整体显隐
    GraphicsProtractor *protractor = new GraphicsProtractor();
    mTools << protractor;

    protractor->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    addItem(protractor);

    QPointF itemSceneCenter = protractor->sceneBoundingRect().center();
    protractor->moveBy(center.x() - itemSceneCenter.x(), center.y() - itemSceneCenter.y());

    protractor->setVisible(true);
}

void GraphicsScene::addTriangle(QPointF center)
{
    // 三角板同上：场景持有，算作工具
    GraphicsTriangle *triangle = new GraphicsTriangle();
    mTools << triangle;

    triangle->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    addItem(triangle);

    QPointF itemSceneCenter = triangle->sceneBoundingRect().center();
    triangle->moveBy(center.x() - itemSceneCenter.x(), center.y() - itemSceneCenter.y());

    triangle->setVisible(true);
}

void GraphicsScene::addMagnifier(MagnifierParams params)
{
    // 放大镜同时只能有一个，重复调用直接忽略
    if (magnifierControlViewWidget)
    {
        return;
    }

    QWidget       *cContainer = (QWidget *)(BoardController::instance()->controlContainer());
    QGraphicsView *cView      = (QGraphicsView *)BoardController::instance()->controlView();
    QGraphicsView *dView      = (QGraphicsView *)BoardController::instance()->displayView();

    QPoint dvZeroPoint        = dView->mapToGlobal(QPoint(0, 0));

    int   cvW                 = cView->width();
    int   dvW                 = dView->width();
    qreal wCoeff              = (qreal)dvW / (qreal)cvW;

    int   cvH                 = cView->height();
    int   dvH                 = dView->height();
    qreal hCoeff              = (qreal)dvH / (qreal)cvH;

    QPoint ccPoint(params.x, params.y);
    QPoint globalPoint = cContainer->mapToGlobal(ccPoint);
    QPoint cvPoint     = cView->mapFromGlobal(globalPoint);
    QPoint dvPoint(cvPoint.x() * wCoeff + dvZeroPoint.x(), cvPoint.y() * hCoeff + dvZeroPoint.y());

    magnifierControlViewWidget = new Magnifier((QWidget *)(BoardController::instance()->controlContainer()), true);
    magnifierControlViewWidget->setGrabView((QGraphicsView *)BoardController::instance()->controlView());
    magnifierControlViewWidget->setMoveView((QWidget *)(BoardController::instance()->controlContainer()));
    magnifierControlViewWidget->setSize(params.sizePercentFromScene);
    magnifierControlViewWidget->setZoom(params.zoom);

    magnifierDisplayViewWidget = new Magnifier((QWidget *)(BoardController::instance()->displayView()), false);
    magnifierDisplayViewWidget->setGrabView((QGraphicsView *)BoardController::instance()->controlView());
    magnifierDisplayViewWidget->setMoveView((QGraphicsView *)BoardController::instance()->displayView());
    magnifierDisplayViewWidget->setSize(params.sizePercentFromScene);
    magnifierDisplayViewWidget->setZoom(params.zoom);

    magnifierControlViewWidget->grabNMove(globalPoint, globalPoint, true);
    magnifierDisplayViewWidget->grabNMove(globalPoint, dvPoint, true);
    magnifierControlViewWidget->show();
    magnifierDisplayViewWidget->show();

    connect(magnifierControlViewWidget, &Magnifier::magnifierMoved_Signal, this,
        [this](QPoint newPos)
        {
            moveMagnifier(newPos);
        });
    connect(magnifierControlViewWidget, &Magnifier::magnifierClose_Signal, this, &GraphicsScene::closeMagnifier);
    connect(magnifierControlViewWidget, &Magnifier::magnifierZoomIn_Signal, this, &GraphicsScene::zoomInMagnifier);
    connect(magnifierControlViewWidget, &Magnifier::magnifierZoomOut_Signal, this, &GraphicsScene::zoomOutMagnifier);
    connect(magnifierControlViewWidget, &Magnifier::magnifierDrawingModeChange_Signal, this, &GraphicsScene::changeMagnifierMode);
    connect(magnifierControlViewWidget, &Magnifier::magnifierResized_Signal, this, &GraphicsScene::resizedMagnifier);

    setModified(true);
}

void GraphicsScene::moveMagnifier()
{
    if (magnifierControlViewWidget)
    {
        QPoint magnifierPos = QPoint(magnifierControlViewWidget->pos().x() + magnifierControlViewWidget->size().width() / 2,
            magnifierControlViewWidget->pos().y() + magnifierControlViewWidget->size().height() / 2);
        moveMagnifier(magnifierPos, true);
        setModified(true);
    }
}

void GraphicsScene::moveMagnifier(QPoint newPos, bool forceGrab)
{
    QWidget       *cContainer = (QWidget *)(BoardController::instance()->controlContainer());
    QGraphicsView *cView      = (QGraphicsView *)BoardController::instance()->controlView();
    QGraphicsView *dView      = (QGraphicsView *)BoardController::instance()->displayView();

    QPoint dvZeroPoint        = dView->mapToGlobal(QPoint(0, 0));

    int   cvW                 = cView->width();
    int   dvW                 = dView->width();
    qreal wCoeff              = (qreal)dvW / (qreal)cvW;

    int   cvH                 = cView->height();
    int   dvH                 = dView->height();
    qreal hCoeff              = (qreal)dvH / (qreal)cvH;

    QPoint globalPoint        = cContainer->mapToGlobal(newPos);
    QPoint cvPoint            = cView->mapFromGlobal(globalPoint);
    QPoint dvPoint(cvPoint.x() * wCoeff + dvZeroPoint.x(), cvPoint.y() * hCoeff + dvZeroPoint.y());

    magnifierControlViewWidget->grabNMove(globalPoint, globalPoint, forceGrab, false);
    magnifierDisplayViewWidget->grabNMove(globalPoint, dvPoint, forceGrab, true);

    setModified(true);
}

void GraphicsScene::closeMagnifier()
{
    DisposeMagnifierQWidgets();
    setModified(true);
}

void GraphicsScene::zoomInMagnifier()
{
    if (magnifierControlViewWidget->zoom() < 8)
    {
        magnifierControlViewWidget->setZoom(magnifierControlViewWidget->zoom() + 0.5);
        magnifierDisplayViewWidget->setZoom(magnifierDisplayViewWidget->zoom() + 0.5);
    }
}

void GraphicsScene::zoomOutMagnifier()
{
    if (magnifierControlViewWidget->zoom() > 1)
    {
        magnifierControlViewWidget->setZoom(magnifierControlViewWidget->zoom() - 0.5);
        magnifierDisplayViewWidget->setZoom(magnifierDisplayViewWidget->zoom() - 0.5);
        setModified(true);
    }
}

void GraphicsScene::changeMagnifierMode(int mode)
{
    if (magnifierControlViewWidget)
    {
        magnifierControlViewWidget->setDrawingMode(mode);
    }
}

void GraphicsScene::resizedMagnifier(qreal newPercent)
{
    if (newPercent > 18 && newPercent < 50)
    {
        magnifierControlViewWidget->setSize(newPercent);
        magnifierControlViewWidget->grabPoint();
        magnifierDisplayViewWidget->setSize(newPercent);
        magnifierDisplayViewWidget->grabPoint();
        setModified(true);
    }
}

bool GraphicsScene::isSnapping() const
{
    return BoardController::instance() && BoardController::instance()->isSnapping();
}

QPointF GraphicsScene::snapToGrid(const QPointF &point) const
{
    if (!isSnapping())
    {
        return QPointF();
    }

    // 横线稿纸（ruled）上只有横线，吸 X 会把图元推到没有线可对的位置上。
    const bool snapX = (pageBackground() != PageBackground::ruled);

    return Snapping::toGrid(point, backgroundGridSize(), snapX).offset;
}

QPointF GraphicsScene::snapCorners(const QList<QPointF> &corners, int *snapIndex) const
{
    if (!isSnapping())
    {
        if (snapIndex)
        {
            *snapIndex = -1;
        }
        return QPointF();
    }

    const bool             snapX = (pageBackground() != PageBackground::ruled);
    const Snapping::Result best  = Snapping::bestOfCorners(corners, backgroundGridSize(), snapX);

    if (snapIndex)
    {
        *snapIndex = best.corner;
    }

    return best.offset;
}

QPointF GraphicsScene::snapToGrid(const QRectF &rect) const
{
    QList<QPointF> corners;
    corners << rect.topLeft() << rect.topRight() << rect.bottomLeft() << rect.bottomRight();
    return snapCorners(corners);
}

QRectF GraphicsScene::itemRectForSnapping(const QGraphicsItem *item)
{
    QRectF bounds                               = item->boundingRect();

    const QAbstractGraphicsShapeItem *shapeItem = dynamic_cast<const QAbstractGraphicsShapeItem *>(item);
    if (shapeItem && shapeItem->pen().style() != Qt::NoPen)
    {
        bounds -= QMarginsF(
            shapeItem->pen().widthF() / 2.0, shapeItem->pen().widthF() / 2.0, shapeItem->pen().widthF() / 2.0, shapeItem->pen().widthF() / 2.0);
    }

    // 一笔只有一段而且是一条名义直线时，包围盒是那条斜线的方形外框，按端点取框才对得上网格。
    const GraphicsStrokesGroup *strokesGroup = dynamic_cast<const GraphicsStrokesGroup *>(item);
    if (strokesGroup && strokesGroup->childItems().count() == 1)
    {
        const GraphicsPolygonItem *polygonItem = dynamic_cast<const GraphicsPolygonItem *>(strokesGroup->childItems().at(0));
        if (polygonItem && polygonItem->isNominalLine())
        {
            const QLineF line = polygonItem->originalLine();
            bounds            = QRectF(line.p1(), line.p2());
        }
    }

    return bounds;
}

void GraphicsScene::addAxes(QPointF center)
{
    GraphicsAxes *axes = new GraphicsAxes(); // 内存交给场景，随场景一起销毁
    mTools << axes;
    addItem(axes);

    QRectF rect = axes->bounds();
    axes->setRect(center.x() - rect.width() / 2, center.y() - rect.height() / 2, rect.width(), rect.height());

    axes->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    axes->setVisible(true);
}

void GraphicsScene::addCompass(QPointF center)
{
    GraphicsCompass *compass = new GraphicsCompass(); // 内存交给场景，随场景一起销毁
    mTools << compass;
    addItem(compass);

    QRectF rect = compass->rect();
    compass->setRect(center.x() - rect.width() / 2, center.y() - rect.height() / 2, rect.width(), rect.height());

    compass->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    compass->setVisible(true);
}

void GraphicsScene::addCache()
{
    GraphicsCache *cache = GraphicsCache::instance(this);
    if (!items().contains(cache))
    {
        addItem(cache);

        cache->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

        cache->setVisible(true);
        cache->setSelected(true);
        notifyCacheEnabled();
        BoardController::instance()->notifyPageChanged();
    }
}

void GraphicsScene::notifyCacheEnabled()
{
    BoardController::instance()->notifyCache(true);
}

void GraphicsScene::addMask(const QPointF &center)
{
    GraphicsCurtainItem *curtain = new GraphicsCurtainItem(); // 内存交给场景，随场景一起销毁
    mTools << curtain;

    addItem(curtain);

    QRectF rect = BoardController::instance()->activeScene()->normalizedSceneRect();
    rect.setRect(center.x() - rect.width() / 4, center.y() - rect.height() / 4, rect.width() / 2, rect.height() / 2);
    curtain->setRect(rect);
    curtain->setVisible(true);
    curtain->setSelected(true);
}

void GraphicsScene::setRenderingQuality(Item::RenderingQuality pRenderingQuality)
{
    QListIterator<QGraphicsItem *> itItems(mFastAccessItems);

    while (itItems.hasNext())
    {
        QGraphicsItem *gItem = itItems.next();

        Item *boardItem      = dynamic_cast<Item *>(gItem);

        if (boardItem)
        {
            boardItem->setRenderingQuality(pRenderingQuality);
        }
    }
}

QList<QUrl> GraphicsScene::relativeDependencies() const
{
    QList<QUrl> relativePathes;

    QListIterator<QGraphicsItem *> itItems(mFastAccessItems);

    while (itItems.hasNext())
    {
        QGraphicsItem *item          = itItems.next();

        GraphicsVideoItem *videoItem = qgraphicsitem_cast<GraphicsVideoItem *>(item);
        if (videoItem)
        {
            QString completeFileName = QFileInfo(videoItem->mediaFileUrl().toLocalFile()).fileName();
            QString path             = PackageLayout::videoDirectory + "/";
            relativePathes << QUrl(path + completeFileName);
            continue;
        }

        GraphicsAudioItem *audioItem = qgraphicsitem_cast<GraphicsAudioItem *>(item);
        if (audioItem)
        {
            QString completeFileName = QFileInfo(audioItem->mediaFileUrl().toLocalFile()).fileName();
            QString path             = PackageLayout::audioDirectory + "/";
            relativePathes << QUrl(path + completeFileName);
            continue;
        }

        GraphicsWidgetItem *widget = qgraphicsitem_cast<GraphicsWidgetItem *>(item);
        if (widget)
        {
            QString widgetPath     = PackageLayout::widgetDirectory + "/" + widget->uuid().toString() + ".wgt";
            QString screenshotPath = PackageLayout::widgetDirectory + "/" + widget->uuid().toString().remove("{").remove("}") + ".png";
            relativePathes << QUrl(widgetPath);
            relativePathes << QUrl(screenshotPath);
            continue;
        }

        GraphicsPixmapItem *pixmapItem = qgraphicsitem_cast<GraphicsPixmapItem *>(item);
        if (pixmapItem)
        {
            relativePathes << QUrl(PackageLayout::imageDirectory + "/" + pixmapItem->uuid().toString() + ".png");
            continue;
        }

        GraphicsSvgItem *svgItem = qgraphicsitem_cast<GraphicsSvgItem *>(item);
        if (svgItem)
        {
            relativePathes << QUrl(PackageLayout::imageDirectory + "/" + svgItem->uuid().toString() + ".svg");
            continue;
        }
    }

    return relativePathes;
}

QSize GraphicsScene::nominalSize()
{
    if (mDocument && !mNominalSize.isValid())
    {
        mNominalSize = mDocument->defaultDocumentSize();
    }

    return mNominalSize;
}

// 场景的实际边界：有背景图元（例如 PDF 页当背景）时以它的包围尺寸为准，
// 因为页面大小是被背景撑出来的，而不是名义尺寸；没有背景才回落 nominalSize()。
QSize GraphicsScene::sceneSize()
{
    GraphicsPDFItem *pdfItem = qgraphicsitem_cast<GraphicsPDFItem *>(backgroundObject());

    if (pdfItem)
    {
        QRectF targetRect = pdfItem->sceneBoundingRect();
        return targetRect.size().toSize();
    }

    else
    {
        return nominalSize();
    }
}

void GraphicsScene::setNominalSize(const QSize &pSize)
{
    if (nominalSize() != pSize)
    {
        mNominalSize = pSize;

        if (mDocument)
        {
            mDocument->setDefaultDocumentSize(pSize);
        }
    }
}

void GraphicsScene::setNominalSize(int pWidth, int pHeight)
{
    setNominalSize(QSize(pWidth, pHeight));
}

void GraphicsScene::setSelectedZLevel(QGraphicsItem *item)
{
    item->setZValue(mZLayerController->generateZLevel(itemLayerType::SelectedItem));
}

void GraphicsScene::setOwnZlevel(QGraphicsItem *item)
{
    item->setZValue(item->data(GraphicsItemData::ItemOwnZValue).toReal());
}

QUuid GraphicsScene::getPersonalUuid(QGraphicsItem *item)
{
    QString idCandidate = item->data(GraphicsItemData::ItemUuid).toString();
    return idCandidate == QUuid().toString() ? QUuid() : QUuid(idCandidate);
}

qreal GraphicsScene::changeZLevelTo(QGraphicsItem *item, ZLayerController::moveDestination dest, bool addUndo)
{
    qreal previousZVal = item->data(GraphicsItemData::ItemOwnZValue).toReal();
    qreal res          = mZLayerController->changeZLevelTo(item, dest);

    if (addUndo)
    {
        GraphicsItemZLevelUndoCommand *uc = new GraphicsItemZLevelUndoCommand(this, item, previousZVal, dest);
        BoardController::undoStack()->push(uc);
    }

    return res;
}

QGraphicsItem *GraphicsScene::rootItem(QGraphicsItem *item) const
{
    QGraphicsItem *root = item;

    while (root->parentItem())
    {
        root = root->parentItem();
    }

    return root;
}

void GraphicsScene::drawItems(QPainter *painter, int numItems, QGraphicsItem *items[], const QStyleOptionGraphicsItem options[], QWidget *widget)
{
    if (mRenderingContext == NonScreen || mRenderingContext == PdfExport)
    {
        int count                                 = 0;

        QGraphicsItem           **itemsFiltered   = new QGraphicsItem *[numItems];
        QStyleOptionGraphicsItem *optionsFiltered = new QStyleOptionGraphicsItem[numItems];

        for (int i = 0; i < numItems; i++)
        {
            if (!mTools.contains(rootItem(items[i])))
            {
                bool isPdfItem = qgraphicsitem_cast<GraphicsPDFItem *>(items[i]) != NULL;
                if (!isPdfItem || mRenderingContext == NonScreen)
                {
                    itemsFiltered[count]   = items[i];
                    optionsFiltered[count] = options[i];
                    count++;
                }
            }
        }

        QGraphicsScene::drawItems(painter, count, itemsFiltered, optionsFiltered, widget);

        delete[] optionsFiltered;
        delete[] itemsFiltered;
    }
    else if (mRenderingContext == Recording)
    {
        int count                                 = 0;

        QGraphicsItem           **itemsFiltered   = new QGraphicsItem *[numItems];
        QStyleOptionGraphicsItem *optionsFiltered = new QStyleOptionGraphicsItem[numItems];

        for (int i = 0; i < numItems; i++)
        {
            bool ok;
            int  itemLayerType = items[i]->data(GraphicsItemData::ItemLayerType).toInt(&ok);
            if (ok && (itemLayerType >= ItemLayerType::FixedBackground && itemLayerType <= ItemLayerType::Tool))
            {
                itemsFiltered[count]   = items[i];
                optionsFiltered[count] = options[i];
                count++;
            }
        }

        QGraphicsScene::drawItems(painter, count, itemsFiltered, optionsFiltered, widget);

        delete[] optionsFiltered;
        delete[] itemsFiltered;
    }
    else
    {
        QGraphicsScene::drawItems(painter, numItems, items, options, widget);
    }
}

void GraphicsScene::drawBackground(QPainter *painter, const QRectF &rect)
{
    if (mIsDesktopMode)
    {
        QGraphicsScene::drawBackground(painter, rect);
        return;
    }
    bool darkBackground = isDarkBackground();

    if (darkBackground)
    {
        painter->fillRect(rect, QBrush(QColor(Qt::black)));
    }
    else
    {
        painter->fillRect(rect, QBrush(QColor(Qt::white)));
    }

    if (mZoomFactor > 0.5)
    {
        QColor bgCrossColor;

        if (darkBackground)
        {
            bgCrossColor = QColor(Settings::settings()->boardCrossColorDarkBackground->get().toString());
        }
        else
        {
            bgCrossColor = QColor(Settings::settings()->boardCrossColorLightBackground->get().toString());
        }
        if (mZoomFactor < 0.7)
        {
            int alpha = 255 * mZoomFactor / 2;
            bgCrossColor.setAlpha(alpha); // 缩小后十字线显得过粗，按缩放比例淡一些压住视觉重量
        }

        painter->setPen(bgCrossColor);

        if (mPageBackground == PageBackground::crossed)
        {
            qreal firstY = ((int)(rect.y() / backgroundGridSize())) * backgroundGridSize();

            for (qreal yPos = firstY; yPos < rect.y() + rect.height(); yPos += backgroundGridSize())
            {
                painter->drawLine(rect.x(), yPos, rect.x() + rect.width(), yPos);
            }

            qreal firstX = ((int)(rect.x() / backgroundGridSize())) * backgroundGridSize();

            for (qreal xPos = firstX; xPos < rect.x() + rect.width(); xPos += backgroundGridSize())
            {
                painter->drawLine(xPos, rect.y(), xPos, rect.y() + rect.height());
            }
        }

        else if (mPageBackground == PageBackground::ruled)
        {
            qreal firstY = ((int)(rect.y() / backgroundGridSize())) * backgroundGridSize();

            for (qreal yPos = firstY; yPos < rect.y() + rect.height(); yPos += backgroundGridSize())
            {
                painter->drawLine(rect.x(), yPos, rect.x() + rect.width(), yPos);
            }
        }
    }
}

void GraphicsScene::keyReleaseEvent(QKeyEvent *keyEvent)
{
    QList<QGraphicsItem *> si = selectedItems();

    if (keyEvent->matches(QKeySequence::SelectAll))
    {
        QListIterator<QGraphicsItem *> itItems(this->mFastAccessItems);

        while (itItems.hasNext())
        {
            itItems.next()->setSelected(true);
        }

        keyEvent->accept();
        return;
    }

    if ((si.size() > 0) && (keyEvent->isAccepted()))
    {
        if (keyEvent->matches(QKeySequence::Delete))
        {
            QVector<GraphicsItem *>  itemsToRemove;
            QVector<QGraphicsItem *> itemToRemove;

            bool bRemoveOk = true;

            foreach (QGraphicsItem *item, si)
            {
                switch (item->type())
                {
                    case GraphicsWidgetItem::Type:
                    {
                        GraphicsWebWidgetItem *wc3_widget = dynamic_cast<GraphicsWebWidgetItem *>(item);
                        if (0 != wc3_widget)
                        {
                            if (!wc3_widget->hasFocus())
                            {
                                itemsToRemove << wc3_widget;
                            }
                        }
                        break;
                    }
                    case GraphicsTextItem::Type:
                    {
                        GraphicsTextItem *text_item = dynamic_cast<GraphicsTextItem *>(item);
                        if (0 != text_item)
                        {
                            if (!text_item->hasFocus())
                            {
                                itemsToRemove << text_item;
                            }
                            else
                            {
                                bRemoveOk = false;
                            }
                        }
                        break;
                    }

                    case GraphicsGroupContainerItem::Type:
                    {
                        GraphicsGroupContainerItem *group_item = dynamic_cast<GraphicsGroupContainerItem *>(item);
                        if (NULL != group_item)
                        {
                            if (!hasTextItemWithFocus(group_item))
                            {
                                itemsToRemove << group_item;
                            }
                            else
                            {
                                bRemoveOk = false;
                            }
                        }
                        break;
                    }

                    default:
                    {
                        GraphicsItem *ubgi = dynamic_cast<GraphicsItem *>(item);
                        if (0 != ubgi)
                        {
                            itemsToRemove << ubgi;
                        }
                        else
                        {
                            itemToRemove << item;
                        }
                    }
                }
            }

            if (bRemoveOk)
            {
                foreach (GraphicsItem *item, itemsToRemove)
                {
                    item->remove();
                }
                foreach (QGraphicsItem *pItem, itemToRemove)
                {
                    CoreGraphicsScene::removeItem(pItem);
                }
            }
        }

        keyEvent->accept();
    }

    QGraphicsScene::keyReleaseEvent(keyEvent);
}

bool GraphicsScene::hasTextItemWithFocus(GraphicsGroupContainerItem *item)
{
    bool bHasFocus = false;

    foreach (QGraphicsItem *pItem, item->childItems())
    {
        GraphicsTextItem *text_item = dynamic_cast<GraphicsTextItem *>(pItem);
        if (NULL != text_item)
        {
            if (text_item->hasFocus())
            {
                bHasFocus = true;
                break;
            }
        }
    }

    return bHasFocus;
}

void GraphicsScene::simplifyCurrentStroke()
{
    if (!mCurrentStroke)
    {
        return;
    }

    GraphicsStroke *simplerStroke = mCurrentStroke->simplify();
    if (!simplerStroke)
    {
        return;
    }

    foreach (GraphicsPolygonItem *poly, mCurrentStroke->polygons())
    {
        mPreviousPolygonItems.removeAll(poly);
        removeItem(poly);
    }

    mCurrentStroke = simplerStroke;

    foreach (GraphicsPolygonItem *poly, mCurrentStroke->polygons())
    {
        addItem(poly);
        mPreviousPolygonItems.append(poly);
    }
}

void GraphicsScene::setDocumentUpdated()
{
    if (document())
    {
        document()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
    }
}

void GraphicsScene::createEraiser()
{
    if (Settings::settings()->showEraserPreviewCircle->get().toBool())
    {
        mEraser = new QGraphicsEllipseItem(); // 内存交给场景，随场景一起销毁
        mEraser->setRect(QRect(0, 0, 0, 0));
        mEraser->setVisible(false);

        updateEraserColor();

        mEraser->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));
        mEraser->setData(GraphicsItemData::itemLayerType,
            QVariant(itemLayerType::Eraiser)); // 必须设好图层类型，Z 值才会被分到正确的层

        mTools << mEraser;
        addItem(mEraser);
    }
}

void GraphicsScene::createPointer()
{
    mPointer = new QGraphicsEllipseItem(); // 内存交给场景，随场景一起销毁
    mPointer->setRect(QRect(0, 0, 20, 20));
    mPointer->setVisible(false);

    mPointer->setPen(Qt::NoPen);
    mPointer->setBrush(QBrush(QColor(255, 0, 0, 186)));

    mPointer->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));
    mPointer->setData(GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::Pointer)); // 必须设好图层类型，Z 值才会被分到正确的层

    mTools << mPointer;
    addItem(mPointer);
}

void GraphicsScene::createMarkerCircle()
{
    if (Settings::settings()->showMarkerPreviewCircle->get().toBool())
    {
        mMarkerCircle = new QGraphicsEllipseItem();

        mMarkerCircle->setRect(QRect(0, 0, 0, 0));
        mMarkerCircle->setVisible(false);

        mMarkerCircle->setPen(Qt::DotLine);
        updateMarkerCircleColor();

        mMarkerCircle->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));
        mMarkerCircle->setData(GraphicsItemData::itemLayerType, QVariant(itemLayerType::Eraiser));

        mTools << mMarkerCircle;
        addItem(mMarkerCircle);
    }
}

void GraphicsScene::createPenCircle()
{
    // 建不建这个图元不看设置：设置是可以在运行中改的，而这一页建好之后不会再建第二次。
    // 圈显不显示归 drawPenCircle 每次悬笔按设置算，改完偏好下一次悬笔就落下来（--preflive-selftest）。
    mPenCircle = new QGraphicsEllipseItem(); // 内存交给场景，随场景一起销毁

    mPenCircle->setRect(QRect(0, 0, 0, 0));
    mPenCircle->setVisible(false);

    mPenCircle->setPen(Qt::DotLine);
    updatePenCircleColor();

    mPenCircle->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));
    mPenCircle->setData(GraphicsItemData::itemLayerType, QVariant(itemLayerType::Eraiser));

    mTools << mPenCircle;
    addItem(mPenCircle);
}

void GraphicsScene::updateEraserColor()
{
    if (!mEraser)
    {
        return;
    }

    if (mDarkBackground)
    {
        mEraser->setBrush(Settings::eraserBrushDarkBackground);
        mEraser->setPen(Settings::eraserPenDarkBackground);
    }

    else
    {
        mEraser->setBrush(Settings::eraserBrushLightBackground);
        mEraser->setPen(Settings::eraserPenLightBackground);
    }
}

void GraphicsScene::updateMarkerCircleColor()
{
    if (!mMarkerCircle)
    {
        return;
    }

    QPen mcPen = mMarkerCircle->pen();

    if (mDarkBackground)
    {
        mcPen.setColor(Settings::markerCirclePenColorDarkBackground);
        mMarkerCircle->setBrush(Settings::markerCircleBrushColorDarkBackground);
    }

    else
    {
        mcPen.setColor(Settings::markerCirclePenColorLightBackground);
        mMarkerCircle->setBrush(Settings::markerCircleBrushColorLightBackground);
    }

    mcPen.setStyle(Qt::DotLine);
    mMarkerCircle->setPen(mcPen);
}

void GraphicsScene::updatePenCircleColor()
{
    if (!mPenCircle)
    {
        return;
    }

    QPen mcPen = mPenCircle->pen();

    if (mDarkBackground)
    {
        mcPen.setColor(Settings::penCirclePenColorDarkBackground);
        mPenCircle->setBrush(Settings::penCircleBrushColorDarkBackground);
    }

    else
    {
        mcPen.setColor(Settings::penCirclePenColorLightBackground);
        mPenCircle->setBrush(Settings::penCircleBrushColorLightBackground);
    }

    mcPen.setStyle(Qt::DotLine);
    mPenCircle->setPen(mcPen);
}

void GraphicsScene::setToolCursor(int tool)
{
    if (tool == (int)StylusTool::Selector || tool == (int)StylusTool::Text || tool == (int)StylusTool::Play)
    {
        deselectAllItems();
        hideMarkerCircle();
        hidePenCircle();
    }

    if (mCurrentStroke && mCurrentStroke->polygons().empty())
    {
        delete mCurrentStroke;
        mCurrentStroke = NULL;
    }
}

void GraphicsScene::initStroke()
{
    mCurrentStroke = new GraphicsStroke(this);
}
