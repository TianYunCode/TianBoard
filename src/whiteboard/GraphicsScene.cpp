#include "whiteboard/GraphicsScene.h"

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QtSvg>
#include <QtSvgWidgets>
#include <QGraphicsView>
#include <QGraphicsVideoItem>

#include "utils/GeometryUtils.h"
#include "platform/PlatformUtils.h"

#include "core/Application.h"
#include "settings/Settings.h"
#include "core/ApplicationController.h"
#include "core/DisplayManager.h"
#include "core/PersistenceManager.h"
#include "core/TextTools.h"

#include "ui/Magnifer.h"
#include "ui/MainWindow.h"
#include "ui/ToolWidget.h"
#include "ui/Resources.h"

#include "whiteboard/GraphicsRuler.h"
#include "whiteboard/GraphicsProtractor.h"
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

ZLayerController::ZLayerController(QGraphicsScene *scene)
    : mScene(scene)

{
    scopeMap.insert(itemLayerType::NoLayer, ItemLayerTypeData(errorNumber, errorNumber));
    scopeMap.insert(itemLayerType::BackgroundItem, ItemLayerTypeData(-1000000.0, -1000000.0));
    // DEFAULT_Z_VALUE isn't used because it allows to easily identify new objects
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

qreal ZLayerController::generateZLevel(itemLayerType::Enum key) {
    if (!scopeMap.contains(key)) {
        qDebug() << "Number is out of layer scope";
        return errorNumber;
    }

    qreal result = scopeMap.value(key).curValue;
    qreal top = scopeMap.value(key).topLimit;
    qreal incrementalStep = scopeMap.value(key).incStep;

    result += incrementalStep;
    if (result >= top) {
        // If not only one variable presents in the scope, notify that values for scope are over
        if (scopeMap.value(key).topLimit != scopeMap.value(key).bottomLimit) {
            qDebug() << "new values are over for the scope" << key;
        }
        result = top - incrementalStep;
    }

    scopeMap[key].curValue = result;

    return result;
}
qreal ZLayerController::generateZLevel(QGraphicsItem *item) {
    qreal result = errorNumber;
    itemLayerType::Enum type =
        static_cast<itemLayerType::Enum>(item->data(GraphicsItemData::itemLayerType).toInt());

    if (validLayerType(type)) {
        result = generateZLevel(type);
    }

    return result;
}

qreal ZLayerController::changeZLevelTo(QGraphicsItem *item, moveDestination dest) {
    itemLayerType::Enum curItemLayerType = typeForData(item);
    if (curItemLayerType == itemLayerType::NoLayer) {
        qDebug() << "item's layer is out of the scope. Can't implement z-layer changing operation";
        return errorNum();
    }

    //select only items wiht the same z-level as item's one and push it to sortedItems QMultiMap
    QMultiMap<qreal, QGraphicsItem *> sortedItems;
    if (mScene->items().count()) {
        foreach (QGraphicsItem *tmpItem, mScene->items()) {
            if (typeForData(tmpItem) == curItemLayerType) {
                sortedItems.insert(tmpItem->data(GraphicsItemData::ItemOwnZValue).toReal(), tmpItem);
            }
        }
    }

    //If only one item itself - do nothing, return it's z-value
    if (sortedItems.count() == 1 && sortedItems.values().first() == item) {
        qDebug() << "only one item exists in layer. Have nothing to change";
        return item->data(GraphicsItemData::ItemOwnZValue).toReal();
    }

    QMultiMapIterator<qreal, QGraphicsItem *> iCurElement(sortedItems);

    if (dest == up) {
        qDebug() << "item data zvalue= " << item->data(GraphicsItemData::ItemOwnZValue).toReal();
        if (iCurElement.findNext(item)) {
            if (iCurElement.hasNext()) {
                qreal nextZ = iCurElement.peekNext().value()->data(GraphicsItemData::ItemOwnZValue).toReal();
                GraphicsItem::assignZValue(iCurElement.peekNext().value(),
                                           item->data(GraphicsItemData::ItemOwnZValue).toReal());
                GraphicsItem::assignZValue(item, nextZ);

                iCurElement.next();

                while (iCurElement.hasNext() &&
                       iCurElement.peekNext().value()->data(GraphicsItemData::ItemOwnZValue).toReal() ==
                           nextZ) {
                    GraphicsItem::assignZValue(iCurElement.next().value(), nextZ);
                }
            }
        }

    } else if (dest == top) {
        if (iCurElement.findNext(item)) {
            if (iCurElement.hasNext()) {
                GraphicsItem::assignZValue(item, generateZLevel(item));
            }
        }

    } else if (dest == down) {
        iCurElement.toBack();
        if (iCurElement.findPrevious(item)) {
            if (iCurElement.hasPrevious()) {
                qreal nextZ =
                    iCurElement.peekPrevious().value()->data(GraphicsItemData::ItemOwnZValue).toReal();
                GraphicsItem::assignZValue(iCurElement.peekPrevious().value(),
                                           item->data(GraphicsItemData::ItemOwnZValue).toReal());
                GraphicsItem::assignZValue(item, nextZ);

                while (iCurElement.hasNext() &&
                       iCurElement.peekNext().value()->data(GraphicsItemData::ItemOwnZValue).toReal() ==
                           nextZ) {
                    GraphicsItem::assignZValue(iCurElement.next().value(), nextZ);
                }
            }
        }

    } else if (dest == bottom) {
        iCurElement.toBack();
        if (iCurElement.findPrevious(item)) {
            if (iCurElement.hasPrevious()) {
                qreal oldz = item->data(GraphicsItemData::ItemOwnZValue).toReal();
                iCurElement.toFront();
                qreal nextZ = iCurElement.next().value()->data(GraphicsItemData::ItemOwnZValue).toReal();

                ItemLayerTypeData curItemLayerTypeData = scopeMap.value(curItemLayerType);

                //if we have some free space between lowest graphics item and layer's bottom bound,
                //insert element close to first element in layer
                if (nextZ > curItemLayerTypeData.bottomLimit + curItemLayerTypeData.incStep) {
                    qreal result = nextZ - curItemLayerTypeData.incStep;
                    GraphicsItem::assignZValue(item, result);
                } else {
                    GraphicsItem::assignZValue(item, nextZ);

                    bool doubleGap =
                        false; //to detect if we can finish rundown since we can insert item to the free space

                    while (iCurElement.peekNext().value() != item) {
                        qreal curZ = iCurElement.value()->data(GraphicsItemData::ItemOwnZValue).toReal();
                        qreal curNextZ =
                            iCurElement.peekNext().value()->data(GraphicsItemData::ItemOwnZValue).toReal();
                        if (curNextZ - curZ >= 2 * curItemLayerTypeData.incStep) {
                            GraphicsItem::assignZValue(iCurElement.value(),
                                                       curZ + curItemLayerTypeData.incStep);
                            doubleGap = true;
                            break;
                        } else {
                            GraphicsItem::assignZValue(iCurElement.value(), curNextZ);
                            iCurElement.next();
                        }
                    }
                    if (!doubleGap) {
                        GraphicsItem::assignZValue(iCurElement.value(), oldz);

                        while (
                            iCurElement.hasNext() &&
                            (iCurElement.peekNext().value()->data(GraphicsItemData::ItemOwnZValue).toReal() ==
                             oldz)) {
                            GraphicsItem::assignZValue(iCurElement.next().value(), oldz);
                        }
                    }
                }
            }
        }
    }


    //clear selection of the item and then select it again to activate selectionChangeProcessing()
    item->scene()->clearSelection();
    item->setSelected(true);

    foreach (QGraphicsItem *iitem, sortedItems.values()) {
        if (iitem)
            iitem != item ? qDebug() << "current value" << iitem->zValue()
                          : qDebug() << "marked value" << QString::number(iitem->zValue(), 'f');
    }

    //Return new z value assigned to item

    // experimental
    item->setZValue(item->data(GraphicsItemData::ItemOwnZValue).toReal());

    return item->data(GraphicsItemData::ItemOwnZValue).toReal();
}

itemLayerType::Enum ZLayerController::typeForData(QGraphicsItem *item) const {
    itemLayerType::Enum result =
        static_cast<itemLayerType::Enum>(item->data(GraphicsItemData::itemLayerType).toInt());

    if (!scopeMap.contains(result)) {
        result = itemLayerType::NoLayer;
    }

    return result;
}

void ZLayerController::setLayerType(QGraphicsItem *pItem, itemLayerType::Enum pNewType) {
    pItem->setData(GraphicsItemData::itemLayerType, QVariant(pNewType));
}

void ZLayerController::shiftStoredZValue(QGraphicsItem *item, qreal zValue) {
    itemLayerType::Enum type = typeForData(item);

    if (validLayerType(type)) {
        ItemLayerTypeData typeData = scopeMap.value(type);
        if (typeData.curValue < zValue) {
            scopeMap[type].curValue = zValue;
        }
    }
}

/**
 * @brief Returns true if the zLevel is not used by any item on the scene, or false if so.
 */
bool ZLayerController::zLevelAvailable(qreal z) {
    foreach (QGraphicsItem *it, dynamic_cast<GraphicsScene *>(mScene)->getFastAccessItems()) {
        if (it->zValue() == z)
            return false;
    }

    return true;
}

GraphicsScene::GraphicsScene(DocumentProxy *parent, bool enableUndoRedoStack)
    : CoreGraphicsScene(parent),
      mEraser(0),
      mPointer(0),
      mMarkerCircle(0),
      mPenCircle(0),
      mDocument(parent),
      mDarkBackground(false),
      mPageBackground(PageBackground::plain),
      mIsDesktopMode(false),
      mZoomFactor(1),
      mBackgroundObject(0),
      mPreviousWidth(0),
      mDistanceFromLastStrokePoint(0),
      mInputDeviceIsPressed(false),
      mArcPolygonItem(0),
      mRenderingContext(Screen),
      mCurrentStroke(0),
      mItemCount(0),
      mUndoRedoStackEnabled(enableUndoRedoStack),
      magniferControlViewWidget(0),
      magniferDisplayViewWidget(0),
      mZLayerController(new ZLayerController(this)),
      mpLastPolygon(NULL),
      mCurrentPolygon(0),
      mTempPolygon(NULL),
      mSelectionFrame(0) {
    CoreGraphicsScene::setObjectName("BoardScene");
    setItemIndexMethod(BspTreeIndex);

    setUuid(QUuid::createUuid());
    setDocument(parent);
    createEraiser();
    createPointer();
    createMarkerCircle();
    createPenCircle();

    if (Application::applicationController) {
        setViewState(SceneViewState(1, Application::applicationController->initialHScroll(),
                                    Application::applicationController->initialVScroll()));
    }

    mBackgroundGridSize = Settings::settings()->crossSize;

    //    Just for debug. Do not delete please
    //    connect(this, SIGNAL(selectionChanged()), this, SLOT(selectionChangedProcessing()));
    connect(Application::undoStack.data(), &QUndoStack::indexChanged, this,
            &GraphicsScene::updateSelectionFrameWrapper);
}

GraphicsScene::~GraphicsScene() {
    if (mCurrentStroke && mCurrentStroke->polygons().empty()) {
        delete mCurrentStroke;
        mCurrentStroke = NULL;
    }

    if (mZLayerController)
        delete mZLayerController;
}

void GraphicsScene::selectionChangedProcessing() {
    if (selectedItems().count()) {
        Application::showMessage(
            "ZValue is " + QString::number(selectedItems().first()->zValue(), 'f') + "own z value is " +
            QString::number(selectedItems().first()->data(GraphicsItemData::ItemOwnZValue).toReal(), 'f'));
    }
}

void GraphicsScene::setLastCenter(QPointF center) {
    mViewState.setLastSceneCenter(center);
}

QPointF GraphicsScene::lastCenter() {
    return mViewState.lastSceneCenter();
}

bool GraphicsScene::inputDevicePress(const QPointF &scenePos, const qreal &pressure) {
    bool accepted = false;

    if (mInputDeviceIsPressed) {
        qWarning() << "scene received input device pressed, without input device release, muting event as "
                      "input device move";
        accepted = inputDeviceMove(scenePos, pressure);
    } else {
        mInputDeviceIsPressed = true;

        StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

        if (DrawingController::drawingController()->isDrawingTool()) {
            // -----------------------------------------------------------------
            // We fall here if we are using the Pen, the Marker or the Line tool
            // -----------------------------------------------------------------
            qreal width = 0;

            // delete current stroke, if not assigned to any polygon
            if (mCurrentStroke && mCurrentStroke->polygons().empty()) {
                delete mCurrentStroke;
                mCurrentStroke = NULL;
            }

            // hide the marker preview circle
            if (currentTool == StylusTool::Marker)
                hideMarkerCircle();

            // hide the pen preview circle
            if (currentTool == StylusTool::Pen)
                hidePenCircle();

            // ---------------------------------------------------------------
            // Create a new Stroke. A Stroke is a collection of QGraphicsLines
            // ---------------------------------------------------------------
            mCurrentStroke = new GraphicsStroke(this);

            if (currentTool != StylusTool::Line) {
                // Handle the pressure
                width = DrawingController::drawingController()->currentToolWidth() * pressure;
            } else {
                // Ignore pressure for the line tool
                width = DrawingController::drawingController()->currentToolWidth();
            }

            width /= Application::boardController->systemScaleFactor();
            width /= Application::boardController->currentZoom();

            mAddedItems.clear();
            mRemovedItems.clear();

            if (DrawingController::drawingController()->mActiveRuler)
                DrawingController::drawingController()->mActiveRuler->StartLine(scenePos, width);
            else {
                moveTo(scenePos);
                drawLineTo(scenePos, width,
                           DrawingController::drawingController()->stylusTool() == StylusTool::Line);

                mCurrentStroke->addPoint(scenePos, width);
            }
            accepted = true;
        } else if (currentTool == StylusTool::Eraser) {
            mAddedItems.clear();
            mRemovedItems.clear();
            moveTo(scenePos);

            qreal eraserWidth = Settings::settings()->currentEraserWidth();
            eraserWidth /= Application::boardController->systemScaleFactor();
            eraserWidth /= Application::boardController->currentZoom();

            eraseLineTo(scenePos, eraserWidth);
            drawEraser(scenePos, mInputDeviceIsPressed);

            accepted = true;
        } else if (currentTool == StylusTool::Pointer) {
            drawPointer(scenePos, true);
            accepted = true;
        }
    }

    if (mCurrentStroke && mCurrentStroke->polygons().empty()) {
        delete mCurrentStroke;
        mCurrentStroke = NULL;
    }

    return accepted;
}

bool GraphicsScene::inputDeviceMove(const QPointF &scenePos, const qreal &pressure) {
    bool accepted = false;

    DrawingController *dc = DrawingController::drawingController();
    StylusTool::Enum currentTool = (StylusTool::Enum)dc->stylusTool();

    QPointF position = QPointF(scenePos);

    if (currentTool == StylusTool::Eraser) {
        drawEraser(position, mInputDeviceIsPressed);
        accepted = true;
    }

    else if (currentTool == StylusTool::Marker) {
        if (mInputDeviceIsPressed)
            hideMarkerCircle();
        else {
            drawMarkerCircle(position);
            accepted = true;
        }
    }

    else if (currentTool == StylusTool::Pen) {
        if (mInputDeviceIsPressed)
            hidePenCircle();
        else {
            drawPenCircle(position);
            accepted = true;
        }
    }

    if (mInputDeviceIsPressed) {
        if (dc->isDrawingTool()) {
            qreal width = 0;

            if (currentTool != StylusTool::Line) {
                // Handle the pressure
                width = dc->currentToolWidth() * qMax(pressure, 0.2);
            } else {
                // Ignore pressure for line tool
                width = dc->currentToolWidth();
            }

            width /= Application::boardController->systemScaleFactor();
            width /= Application::boardController->currentZoom();

            if (currentTool == StylusTool::Line || dc->mActiveRuler) {
                if (DrawingController::drawingController()->stylusTool() != StylusTool::Marker)
                    if (NULL != mpLastPolygon && NULL != mCurrentStroke && mAddedItems.size() > 0) {
                        CoreGraphicsScene::removeItemFromDeletion(mpLastPolygon);
                        mAddedItems.remove(mpLastPolygon);
                        mCurrentStroke->remove(mpLastPolygon);
                        if (mCurrentStroke->polygons().empty()) {
                            delete mCurrentStroke;
                            mCurrentStroke = NULL;
                        }
                        removeItem(mpLastPolygon);
                        mPreviousPolygonItems.removeAll(mpLastPolygon);
                    }

                // ------------------------------------------------------------------------
                // Here we wanna make sure that the Line will 'grip' at i*45, i*90 degrees
                // ------------------------------------------------------------------------

                QLineF radius(mPreviousPoint, position);
                qreal angle = radius.angle();
                angle = qRound(angle / 45) * 45;
                qreal radiusLength = radius.length();
                QPointF newPosition(mPreviousPoint.x() + radiusLength * cos((angle * PI) / 180),
                                    mPreviousPoint.y() - radiusLength * sin((angle * PI) / 180));
                QLineF chord(position, newPosition);
                if (chord.length() < qMin((int)16, (int)(radiusLength / 20)))
                    position = newPosition;
            }

            if (!mCurrentStroke)
                mCurrentStroke = new GraphicsStroke(this);

            if (dc->mActiveRuler) {
                dc->mActiveRuler->DrawLine(position, width);
            }

            else if (currentTool == StylusTool::Line) {
                drawLineTo(position, width, true);
            }

            else {
                bool interpolate = false;

                if ((currentTool == StylusTool::Pen &&
                     Settings::settings()->boardInterpolatePenStrokes->get().toBool()) ||
                    (currentTool == StylusTool::Marker &&
                     Settings::settings()->boardInterpolateMarkerStrokes->get().toBool())) {
                    interpolate = true;
                }


                // Don't draw segments smaller than a certain length. This can help with performance
                // (less polygons to draw) but mostly with making the curve look smooth.

                qreal antiScaleRatio = 1. / (Application::boardController->systemScaleFactor() *
                                             Application::boardController->currentZoom());
                qreal MIN_DISTANCE = 10 * antiScaleRatio; // arbitrary. Move to settings if relevant.
                qreal distance = QLineF(mPreviousPoint, scenePos).length();

                mDistanceFromLastStrokePoint += distance;

                if (mDistanceFromLastStrokePoint > MIN_DISTANCE) {
                    QList<QPair<QPointF, qreal>> newPoints =
                        mCurrentStroke->addPoint(scenePos, width, interpolate);
                    if (newPoints.length() > 1)
                        drawCurve(newPoints);

                    mDistanceFromLastStrokePoint = 0;
                }

                if (interpolate) {
                    // Bezier curves aren't drawn all the way to the scenePos (they stop halfway between the previous and
                    // current scenePos), so we add a line from the last drawn position in the stroke and the
                    // scenePos, to make the drawing feel more responsive. This line is then deleted if a new segment is
                    // added to the stroke. (Or it is added to the stroke when we stop drawing)

                    if (mTempPolygon) {
                        removeItem(mTempPolygon);
                        mTempPolygon = NULL;
                    }

                    QPointF lastDrawnPoint = mCurrentStroke->points().last().first;

                    mTempPolygon = lineToPolygonItem(QLineF(lastDrawnPoint, scenePos), mPreviousWidth, width);
                    addItem(mTempPolygon);
                }
            }
        } else if (currentTool == StylusTool::Eraser) {
            qreal eraserWidth = Settings::settings()->currentEraserWidth();
            eraserWidth /= Application::boardController->systemScaleFactor();
            eraserWidth /= Application::boardController->currentZoom();

            eraseLineTo(position, eraserWidth);
        } else if (currentTool == StylusTool::Pointer) {
            drawPointer(position);
        }

        accepted = true;
    }

    return accepted;
}

bool GraphicsScene::inputDeviceRelease() {
    bool accepted = false;

    if (mPointer) {
        mPointer->hide();
        accepted = true;
    }

    StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

    if (currentTool == StylusTool::Eraser)
        redrawEraser(false);


    DrawingController *dc = DrawingController::drawingController();

    if (dc->isDrawingTool() || mDrawWithCompass) {
        if (mArcPolygonItem) {
            GraphicsStrokesGroup *pStrokes = new GraphicsStrokesGroup();

            // Add the arc
            mAddedItems.remove(mArcPolygonItem);
            removeItem(mArcPolygonItem);
            CoreGraphicsScene::removeItemFromDeletion(mArcPolygonItem);
            mArcPolygonItem->setStrokesGroup(pStrokes);
            pStrokes->addToGroup(mArcPolygonItem);

            // Add the center cross
            foreach (QGraphicsItem *item, mAddedItems) {
                mAddedItems.remove(item);
                removeItem(item);
                CoreGraphicsScene::removeItemFromDeletion(item);
                GraphicsPolygonItem *pi = qgraphicsitem_cast<GraphicsPolygonItem *>(item);
                if (pi)
                    pi->setStrokesGroup(pStrokes);
                pStrokes->addToGroup(item);
            }

            mAddedItems.clear();
            mAddedItems << pStrokes;
            addItem(pStrokes);

            mDrawWithCompass = false;
        } else if (mCurrentStroke) {
            if (mTempPolygon) {
                GraphicsPolygonItem *poly = dynamic_cast<GraphicsPolygonItem *>(mTempPolygon->deepCopy());
                removeItem(mTempPolygon);
                mTempPolygon = NULL;
                addPolygonItemToCurrentStroke(poly);
            }

            // replace the stroke by a simplified version of it
            if ((currentTool == StylusTool::Pen &&
                 Settings::settings()->boardSimplifyPenStrokes->get().toBool()) ||
                (currentTool == StylusTool::Marker &&
                 Settings::settings()->boardSimplifyMarkerStrokes->get().toBool())) {
                simplifyCurrentStroke();
            }


            GraphicsStrokesGroup *pStrokes = new GraphicsStrokesGroup();

            // Remove the strokes that were just drawn here and replace them by a stroke item
            foreach (GraphicsPolygonItem *poly, mCurrentStroke->polygons()) {
                mPreviousPolygonItems.removeAll(poly);
                removeItem(poly);
                CoreGraphicsScene::removeItemFromDeletion(poly);
                poly->setStrokesGroup(pStrokes);
                pStrokes->addToGroup(poly);
            }

            // TODO LATER : Generate well pressure-interpolated polygons and create the line group with them

            mAddedItems.clear();
            mAddedItems << pStrokes;
            addItem(pStrokes);

            if (mCurrentStroke->polygons().empty()) {
                delete mCurrentStroke;
                mCurrentStroke = 0;
            }
            mCurrentPolygon = 0;
        }
    }

    if (mRemovedItems.size() > 0 || mAddedItems.size() > 0) {
        if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
            GraphicsItemUndoCommand *udcmd =
                new GraphicsItemUndoCommand(this, mRemovedItems, mAddedItems); //deleted by the undoStack

            if (Application::undoStack)
                Application::undoStack->push(udcmd);
        }

        mRemovedItems.clear();
        mAddedItems.clear();
        accepted = true;
    }

    mInputDeviceIsPressed = false;

    setDocumentUpdated();

    if (mCurrentStroke && mCurrentStroke->polygons().empty()) {
        delete mCurrentStroke;
    }

    mCurrentStroke = NULL;
    return accepted;
}

void GraphicsScene::drawEraser(const QPointF &pPoint, bool pressed) {
    if (mEraser) {
        qreal eraserWidth = Settings::settings()->currentEraserWidth();
        eraserWidth /= Application::boardController->systemScaleFactor();
        eraserWidth /= Application::boardController->currentZoom();

        qreal eraserRadius = eraserWidth / 2;

        mEraser->setRect(
            QRectF(pPoint.x() - eraserRadius, pPoint.y() - eraserRadius, eraserWidth, eraserWidth));
        redrawEraser(pressed);
    }
}

void GraphicsScene::redrawEraser(bool pressed) {
    if (mEraser) {
        QPen pen = mEraser->pen();

        if (pressed)
            pen.setStyle(Qt::SolidLine);
        else
            pen.setStyle(Qt::DotLine);

        mEraser->setPen(pen);
        mEraser->show();
    }
}

void GraphicsScene::hideEraser() {
    if (mEraser)
        mEraser->hide();
}

void GraphicsScene::drawPointer(const QPointF &pPoint, bool isFirstDraw) {
    qreal pointerDiameter = Settings::pointerDiameter / Application::boardController->currentZoom();
    qreal pointerRadius = pointerDiameter / 2;

    if (mPointer) {
        mPointer->setRect(
            QRectF(pPoint.x() - pointerRadius, pPoint.y() - pointerRadius, pointerDiameter, pointerDiameter));
        if (isFirstDraw) {
            mPointer->show();
        }
    }
}

void GraphicsScene::drawMarkerCircle(const QPointF &pPoint) {
    if (mMarkerCircle) {
        qreal markerDiameter = Settings::settings()->currentMarkerWidth();
        markerDiameter /= Application::boardController->systemScaleFactor();
        markerDiameter /= Application::boardController->currentZoom();
        qreal markerRadius = markerDiameter / 2;

        mMarkerCircle->setRect(
            QRectF(pPoint.x() - markerRadius, pPoint.y() - markerRadius, markerDiameter, markerDiameter));
        mMarkerCircle->show();
    }
}

void GraphicsScene::drawPenCircle(const QPointF &pPoint) {
    if (mPenCircle && Settings::settings()->showPenPreviewCircle->get().toBool() &&
        Settings::settings()->currentPenWidth() >= Settings::settings()->penPreviewFromSize->get().toInt()) {
        qreal penDiameter = Settings::settings()->currentPenWidth();
        penDiameter /= Application::boardController->systemScaleFactor();
        penDiameter /= Application::boardController->currentZoom();
        qreal penRadius = penDiameter / 2;

        mPenCircle->setRect(QRectF(pPoint.x() - penRadius, pPoint.y() - penRadius, penDiameter, penDiameter));

        if (controlView())
            if (controlView()->viewport())
                controlView()->viewport()->setCursor(QCursor(Qt::BlankCursor));

        mPenCircle->show();
    } else {
        if (controlView())
            if (controlView()->viewport())
                controlView()->viewport()->setCursor(Resources::resources()->penCursor);
    }
}

void GraphicsScene::hideMarkerCircle() {
    if (mMarkerCircle) {
        mMarkerCircle->hide();
    }
}

void GraphicsScene::hidePenCircle() {
    if (mPenCircle)
        mPenCircle->hide();
}

// call this function when user release mouse button in Magnifier mode
void GraphicsScene::DisposeMagnifierQWidgets() {
    if (magniferControlViewWidget) {
        magniferControlViewWidget->hide();
        magniferControlViewWidget->setParent(0);
        delete magniferControlViewWidget;
        magniferControlViewWidget = NULL;
    }

    if (magniferDisplayViewWidget) {
        magniferDisplayViewWidget->hide();
        magniferDisplayViewWidget->setParent(0);
        delete magniferDisplayViewWidget;
        magniferDisplayViewWidget = NULL;
    }
    // some time have crash here on access to app (when call from destructor when close TianBoard app)
    // so i just add try/catch section here
    try {
        Application::app()->restoreOverrideCursor();
    } catch (...) {}
}

void GraphicsScene::moveTo(const QPointF &pPoint) {
    mPreviousPoint = pPoint;
    mPreviousWidth = -1.0;
    mPreviousPolygonItems.clear();
    mArcPolygonItem = 0;
    mDrawWithCompass = false;
}
void GraphicsScene::drawLineTo(const QPointF &pEndPoint, const qreal &pWidth, bool bLineStyle) {
    drawLineTo(pEndPoint, pWidth, pWidth, bLineStyle);
}

void GraphicsScene::drawLineTo(const QPointF &pEndPoint, const qreal &startWidth, const qreal &endWidth,
                               bool bLineStyle) {
    if (mPreviousWidth == -1.0)
        mPreviousWidth = startWidth;

    qreal initialWidth = startWidth;
    if (initialWidth == endWidth)
        initialWidth = mPreviousWidth;

    if (bLineStyle) {
        QSetIterator<QGraphicsItem *> itItems(mAddedItems);

        while (itItems.hasNext()) {
            QGraphicsItem *item = itItems.next();
            removeItem(item);
        }
        mAddedItems.clear();
    }

    GraphicsPolygonItem *polygonItem =
        lineToPolygonItem(QLineF(mPreviousPoint, pEndPoint), initialWidth, endWidth);
    addPolygonItemToCurrentStroke(polygonItem);

    if (!bLineStyle) {
        mPreviousPoint = pEndPoint;
        mPreviousWidth = endWidth;
    }
}

void GraphicsScene::drawCurve(const QList<QPair<QPointF, qreal>> &points) {
    GraphicsPolygonItem *polygonItem = curveToPolygonItem(points);
    addPolygonItemToCurrentStroke(polygonItem);

    mPreviousPoint = points.last().first;
    mPreviousWidth = points.last().second;
}

void GraphicsScene::drawCurve(const QList<QPointF> &points, qreal startWidth, qreal endWidth) {
    GraphicsPolygonItem *polygonItem = curveToPolygonItem(points, startWidth, endWidth);
    addPolygonItemToCurrentStroke(polygonItem);

    mPreviousWidth = endWidth;
    mPreviousPoint = points.last();
}

void GraphicsScene::addPolygonItemToCurrentStroke(GraphicsPolygonItem *polygonItem) {
    if (!polygonItem->brush().isOpaque()) {
        // -------------------------------------------------------------------------------------
        // Here we substract the polygons that are overlapping in order to keep the transparency
        // -------------------------------------------------------------------------------------
        for (int i = 0; i < mPreviousPolygonItems.size(); i++) {
            GraphicsPolygonItem *previous = mPreviousPolygonItems.value(i);
            polygonItem->subtract(previous);
        }
    }

    mpLastPolygon = polygonItem;
    mAddedItems.insert(polygonItem);

    // Here we add the item to the scene
    addItem(polygonItem);
    if (!mCurrentStroke)
        mCurrentStroke = new GraphicsStroke(this);

    polygonItem->setStroke(mCurrentStroke);

    mPreviousPolygonItems.append(polygonItem);
}

void GraphicsScene::eraseLineTo(const QPointF &pEndPoint, const qreal &pWidth) {
    const QLineF line(mPreviousPoint, pEndPoint);
    mPreviousPoint = pEndPoint;

    const QPolygonF eraserPolygon = GeometryUtils::lineToPolygon(line, pWidth);
    const QRectF eraserBoundingRect = eraserPolygon.boundingRect();

    QPainterPath eraserPath;
    eraserPath.addPolygon(eraserPolygon);

    // Get all the items that are intersecting with the eraser path
    QList<QGraphicsItem *> collidItems = items(eraserBoundingRect, Qt::IntersectsItemBoundingRect);

    QList<GraphicsPolygonItem *> intersectedItems;

    typedef QList<QPolygonF> POLYGONSLIST;
    QList<POLYGONSLIST> intersectedPolygons;

#pragma omp parallel for
    for (int i = 0; i < collidItems.size(); i++) {
        GraphicsPolygonItem *pi = qgraphicsitem_cast<GraphicsPolygonItem *>(collidItems[i]);
        if (pi == NULL)
            continue;

        QPainterPath itemPainterPath;
        itemPainterPath.addPolygon(pi->sceneTransform().map(pi->polygon()));

        if (eraserPath.contains(itemPainterPath)) {
#pragma omp critical
            {
                // Compete remove item
                intersectedItems << pi;
                intersectedPolygons << QList<QPolygonF>();
            }
        } else if (eraserPath.intersects(itemPainterPath)) {
            itemPainterPath.setFillRule(Qt::WindingFill);
            QPainterPath newPath = itemPainterPath.subtracted(eraserPath);
#pragma omp critical
            {
                intersectedItems << pi;
                intersectedPolygons << newPath.simplified().toFillPolygons(pi->sceneTransform().inverted());
            }
        }
    }

    for (int i = 0; i < intersectedItems.size(); i++) {
        // item who intersects with eraser
        GraphicsPolygonItem *intersectedPolygonItem = intersectedItems[i];

        if (!intersectedPolygons[i].empty()) {
            // intersected polygons generated as QList<QPolygon> QPainterPath::toFillPolygons(),
            // so each intersectedPolygonItem has one or couple of QPolygons who should be removed from it.
            for (int j = 0; j < intersectedPolygons[i].size(); j++) {
                // create small polygon from couple of polygons to replace particular erased polygon
                GraphicsPolygonItem *polygonItem =
                    new GraphicsPolygonItem(intersectedPolygons[i][j], intersectedPolygonItem->parentItem());

                intersectedPolygonItem->copyItemParameters(polygonItem);
                polygonItem->setNominalLine(false);
                polygonItem->setStroke(intersectedPolygonItem->stroke());
                polygonItem->setStrokesGroup(intersectedPolygonItem->strokesGroup());
                intersectedPolygonItem->strokesGroup()->addToGroup(polygonItem);
                mAddedItems << polygonItem;
            }
        }

        //remove full polygon item for replace it by couple of polygons which creates the same stroke without a part intersects with eraser
        mRemovedItems << intersectedPolygonItem;

        QTransform t;
        bool bApplyTransform = false;
        if (intersectedPolygonItem->strokesGroup()) {
            if (intersectedPolygonItem->strokesGroup()->parentItem()) {
                bApplyTransform = true;
                t = intersectedPolygonItem->sceneTransform();
            }
            intersectedPolygonItem->strokesGroup()->removeFromGroup(intersectedPolygonItem);
        }
        removeItem(intersectedPolygonItem);
        if (bApplyTransform)
            intersectedPolygonItem->setTransform(t);
    }

    if (!intersectedItems.empty())
        setModified(true);
}

void GraphicsScene::drawArcTo(const QPointF &pCenterPoint, qreal pSpanAngle) {
    mDrawWithCompass = true;
    if (mArcPolygonItem) {
        mAddedItems.remove(mArcPolygonItem);
        removeItem(mArcPolygonItem);
        mArcPolygonItem = 0;
    }
    qreal penWidth = Settings::settings()->currentPenWidth();
    penWidth /= Application::boardController->systemScaleFactor();
    penWidth /= Application::boardController->currentZoom();

    mArcPolygonItem = arcToPolygonItem(QLineF(pCenterPoint, mPreviousPoint), pSpanAngle, penWidth);
    mArcPolygonItem->setFillRule(Qt::WindingFill);
    mArcPolygonItem->setStroke(mCurrentStroke);
    mAddedItems.insert(mArcPolygonItem);
    addItem(mArcPolygonItem);

    setDocumentUpdated();
}

void GraphicsScene::setBackground(bool pIsDark, PageBackground pBackground) {
    bool needRepaint = false;

    if (mDarkBackground != pIsDark) {
        mDarkBackground = pIsDark;

        updateEraserColor();
        updateMarkerCircleColor();
        updatePenCircleColor();
        recolorAllItems();

        needRepaint = true;
        setModified(true);
    }

    if (mPageBackground != pBackground) {
        mPageBackground = pBackground;
        needRepaint = true;
        setModified(true);
    }

    if (needRepaint) {
        foreach (QGraphicsView *view, views()) {
            view->resetCachedContent();
        }
    }
}

void GraphicsScene::setBackgroundZoomFactor(qreal zoom) {
    mZoomFactor = zoom;
}


void GraphicsScene::setBackgroundGridSize(int pSize) {
    if (pSize > 0) {
        mBackgroundGridSize = pSize;
        setModified(true);

        foreach (QGraphicsView *view, views())
            view->resetCachedContent();
    }
}

void GraphicsScene::setDrawingMode(bool bModeDesktop) {
    mIsDesktopMode = bModeDesktop;
}

void GraphicsScene::recolorAllItems() {
    QMap<QGraphicsView *, QGraphicsView::ViewportUpdateMode> previousUpdateModes;
    foreach (QGraphicsView *view, views()) {
        previousUpdateModes.insert(view, view->viewportUpdateMode());
        view->setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    }

    bool currentIslight = isLightBackground();
    foreach (QGraphicsItem *item, items()) {
        if (item->type() == GraphicsStrokesGroup::Type) {
            GraphicsStrokesGroup *curGroup = static_cast<GraphicsStrokesGroup *>(item);
            QColor compareColor =
                curGroup->color(currentIslight ? GraphicsStrokesGroup::colorOnDarkBackground
                                               : GraphicsStrokesGroup::colorOnLightBackground);

            if (curGroup->color() == compareColor) {
                QColor newColor =
                    curGroup->color(!currentIslight ? GraphicsStrokesGroup::colorOnDarkBackground
                                                    : GraphicsStrokesGroup::colorOnLightBackground);
                curGroup->setColor(newColor);
            }
        }

        if (item->type() == GraphicsTextItem::Type) {
            GraphicsTextItem *textItem = static_cast<GraphicsTextItem *>(item);
            textItem->recolor();
        }
    }

    foreach (QGraphicsView *view, views()) {
        view->setViewportUpdateMode(previousUpdateModes.value(view));
    }
}

GraphicsPolygonItem *GraphicsScene::lineToPolygonItem(const QLineF &pLine, const qreal &pWidth) {
    GraphicsPolygonItem *polygonItem = new GraphicsPolygonItem(pLine, pWidth);

    initPolygonItem(polygonItem);

    return polygonItem;
}


GraphicsPolygonItem *GraphicsScene::lineToPolygonItem(const QLineF &pLine, const qreal &pStartWidth,
                                                      const qreal &pEndWidth) {
    GraphicsPolygonItem *polygonItem = new GraphicsPolygonItem(pLine, pStartWidth, pEndWidth);

    initPolygonItem(polygonItem);

    return polygonItem;
}

void GraphicsScene::initPolygonItem(GraphicsPolygonItem *polygonItem) {
    QColor colorOnDarkBG;
    QColor colorOnLightBG;

    if (DrawingController::drawingController()->stylusTool() == StylusTool::Marker) {
        colorOnDarkBG = Application::boardController->markerColorOnDarkBackground();
        colorOnLightBG = Application::boardController->markerColorOnLightBackground();
    } else // settings->stylusTool() == StylusTool::Pen + failsafe
    {
        colorOnDarkBG = Application::boardController->penColorOnDarkBackground();
        colorOnLightBG = Application::boardController->penColorOnLightBackground();
    }

    if (mDarkBackground) {
        polygonItem->setColor(colorOnDarkBG);
    } else {
        polygonItem->setColor(colorOnLightBG);
    }

    //polygonItem->setColor(QColor(rand()%256, rand()%256, rand()%256, polygonItem->brush().color().alpha()));

    polygonItem->setColorOnDarkBackground(colorOnDarkBG);
    polygonItem->setColorOnLightBackground(colorOnLightBG);

    polygonItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Graphic));
}

GraphicsPolygonItem *GraphicsScene::arcToPolygonItem(const QLineF &pStartRadius, qreal pSpanAngle,
                                                     qreal pWidth) {
    QPolygonF polygon = GeometryUtils::arcToPolygon(pStartRadius, pSpanAngle, pWidth);

    return polygonToPolygonItem(polygon);
}

GraphicsPolygonItem *GraphicsScene::curveToPolygonItem(const QList<QPair<QPointF, qreal>> &points) {
    QPolygonF polygon = GeometryUtils::curveToPolygon(points, false, true);

    return polygonToPolygonItem(polygon);
}

GraphicsPolygonItem *GraphicsScene::curveToPolygonItem(const QList<QPointF> &points, qreal startWidth,
                                                       qreal endWidth) {
    QPolygonF polygon = GeometryUtils::curveToPolygon(points, startWidth, endWidth);

    return polygonToPolygonItem(polygon);
}

void GraphicsScene::clearSelectionFrame() {
    if (mSelectionFrame) {
        mSelectionFrame->setEnclosedItems(QList<QGraphicsItem *>());
    }
}

BoardView *GraphicsScene::controlView() {
    BoardView *result = 0;
    foreach (QGraphicsView *view, views()) {
        if (view->objectName() == CONTROLVIEW_OBJ_NAME) {
            result = static_cast<BoardView *>(view);
        }
    }

    return result;
}

void GraphicsScene::notifyZChanged(QGraphicsItem *item, qreal zValue) {
    mZLayerController->shiftStoredZValue(item, zValue);
}

void GraphicsScene::updateSelectionFrame() {
    if (!mSelectionFrame) {
        mSelectionFrame = new SelectionFrame();
        addItem(mSelectionFrame);
    }

    QList<QGraphicsItem *> selItems = selectedItems();
    switch (selItems.count()) {
        case 0: {
            mSelectionFrame->setVisible(false);
            mSelectionFrame->setEnclosedItems(selItems);
        } break;
        case 1: {
            mSelectionFrame->setVisible(false);
            mSelectionFrame->setEnclosedItems(QList<QGraphicsItem *>());

            GraphicsItemDelegate *itemDelegate = GraphicsItem::Delegate(selItems.first());
            itemDelegate->createControls();
            selItems.first()->setVisible(true);
            itemDelegate->showControls();

        } break;
        default: {
            mSelectionFrame->setVisible(true);
            mSelectionFrame->setEnclosedItems(selItems);
        } break;
    }
}

void GraphicsScene::updateSelectionFrameWrapper(int) {
    updateSelectionFrame();
}

GraphicsPolygonItem *GraphicsScene::polygonToPolygonItem(const QPolygonF pPolygon) {
    GraphicsPolygonItem *polygonItem = new GraphicsPolygonItem(pPolygon);

    initPolygonItem(polygonItem);

    return polygonItem;
}

void GraphicsScene::hideTool() {
    hideEraser();
    hideMarkerCircle();
    hidePenCircle();
}

void GraphicsScene::leaveEvent(QEvent *event) {
    Q_UNUSED(event);
    hideTool();
}

GraphicsScene *GraphicsScene::sceneDeepCopy() const {
    GraphicsScene *copy = new GraphicsScene(this->document(), this->mUndoRedoStackEnabled);

    copy->setBackground(this->isDarkBackground(), mPageBackground);
    copy->setBackgroundGridSize(mBackgroundGridSize);
    copy->setSceneRect(this->sceneRect());

    if (this->mNominalSize.isValid())
        copy->setNominalSize(this->mNominalSize);

    QListIterator<QGraphicsItem *> itItems(this->mFastAccessItems);
    QMap<GraphicsStroke *, GraphicsStroke *> groupClone;

    while (itItems.hasNext()) {
        QGraphicsItem *item = itItems.next();
        QGraphicsItem *cloneItem = 0;
        Item *boardItem = dynamic_cast<Item *>(item);
        GraphicsStroke *stroke = dynamic_cast<GraphicsStroke *>(item);
        GraphicsGroupContainerItem *group = dynamic_cast<GraphicsGroupContainerItem *>(item);

        if (group) {
            GraphicsGroupContainerItem *groupCloned = group->deepCopyNoChildDuplication();
            groupCloned->resetTransform();
            groupCloned->resetTransform();
            groupCloned->setPos(0, 0);
            bool locked = groupCloned->Delegate()->isLocked();

            foreach (QGraphicsItem *eachItem, group->childItems()) {
                QGraphicsItem *copiedChild =
                    dynamic_cast<QGraphicsItem *>(dynamic_cast<Item *>(eachItem)->deepCopy());
                copy->addItem(copiedChild);
                groupCloned->addToGroup(copiedChild);
            }

            if (locked)
                groupCloned->setData(GraphicsItemData::ItemLocked, QVariant(true));

            copy->addItem(groupCloned);
            groupCloned->setTransform(group->transform());
            groupCloned->setTransform(QTransform::fromTranslate(group->pos().x(), group->pos().y()));
            groupCloned->setTransform(group->transform(), true);
        }

        if (boardItem && !stroke && !group && item->isVisible())
            cloneItem = dynamic_cast<QGraphicsItem *>(boardItem->deepCopy());

        if (cloneItem) {
            copy->addItem(cloneItem);

            if (isBackgroundObject(item))
                copy->setAsBackgroundObject(cloneItem);

            if (this->mTools.contains(item))
                copy->mTools << cloneItem;

            GraphicsPolygonItem *polygon = dynamic_cast<GraphicsPolygonItem *>(item);

            if (polygon) {
                GraphicsStroke *stroke = dynamic_cast<GraphicsStroke *>(item->parentItem());

                if (stroke) {
                    GraphicsStroke *cloneStroke = groupClone.value(stroke);

                    if (!cloneStroke) {
                        cloneStroke = stroke->deepCopy();
                        groupClone.insert(stroke, cloneStroke);
                    }

                    polygon->setStroke(cloneStroke);
                }
            }
        }
    }

    return copy;
}

Item *GraphicsScene::deepCopy() const {
    return sceneDeepCopy();
}

void GraphicsScene::clearContent(clearCase pCase) {
    QSet<QGraphicsItem *> removedItems;
    GraphicsItemUndoCommand::GroupDataTable groupsMap;

    switch (pCase) {
        case clearBackground:
            if (mBackgroundObject) {
                removeItem(mBackgroundObject);
                removedItems << mBackgroundObject;
            }
            break;

        case clearItemsAndAnnotations:
        case clearItems:
        case clearAnnotations:
            foreach (QGraphicsItem *item, items()) {
                GraphicsGroupContainerItem *itemGroup =
                    item->parentItem() ? qgraphicsitem_cast<GraphicsGroupContainerItem *>(item->parentItem())
                                       : 0;
                GraphicsItemDelegate *curDelegate = GraphicsItem::Delegate(item);
                if (!curDelegate) {
                    continue;
                }

                bool isGroup = item->type() == GraphicsGroupContainerItem::Type;
                bool isStrokesGroup = item->type() == GraphicsStrokesGroup::Type;

                bool shouldDelete = false;
                switch (static_cast<int>(pCase)) {
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

                if (shouldDelete) {
                    if (itemGroup) {
                        itemGroup->removeFromGroup(item);

                        groupsMap.insert(itemGroup, GraphicsItem::getOwnUuid(item));
                        if (itemGroup->childItems().count() == 1) {
                            groupsMap.insert(itemGroup,
                                             GraphicsItem::getOwnUuid(itemGroup->childItems().first()));
                            QGraphicsItem *lastItem = itemGroup->childItems().first();
                            bool isSelected = itemGroup->isSelected();
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

    // force refresh, QT is a bit lazy and take a lot of time (nb item ^2 ?) to trigger repaint
    update(sceneRect());

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented

        GraphicsItemUndoCommand *uc =
            new GraphicsItemUndoCommand(this, removedItems, QSet<QGraphicsItem *>(), groupsMap);
        Application::undoStack->push(uc);
    }

    if (pCase == clearBackground) {
        mBackgroundObject = 0;
    }

    setDocumentUpdated();
}

GraphicsPixmapItem *GraphicsScene::addPixmap(const QPixmap &pPixmap, QGraphicsItem *replaceFor,
                                             const QPointF &pPos, qreal pScaleFactor, bool pUseAnimation,
                                             bool useProxyForDocumentPath) {
    GraphicsPixmapItem *pixmapItem = new GraphicsPixmapItem();

    pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    pixmapItem->setPixmap(pPixmap);

    QPointF half(pPixmap.width() * pScaleFactor / 2, pPixmap.height() * pScaleFactor / 2);
    pixmapItem->setPos(pPos - half);

    addItem(pixmapItem);

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, replaceFor, pixmapItem);
        Application::undoStack->push(uc);
    }

    pixmapItem->setTransform(QTransform::fromScale(pScaleFactor, pScaleFactor), true);

    if (pUseAnimation) {
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
        documentPath = this->document()->persistencePath();
    else
        documentPath = Application::boardController->selectedDocument()->persistencePath();

    QString fileName = PersistenceManager::imageDirectory + "/" + pixmapItem->uuid().toString() + ".png";

    QString path = documentPath + "/" + fileName;

    if (!QFile::exists(path)) {
        QDir dir;
        dir.mkdir(documentPath + "/" + PersistenceManager::imageDirectory);

        pixmapItem->pixmap().toImage().save(path, "PNG");
    }

    return pixmapItem;
}

void GraphicsScene::textUndoCommandAdded(GraphicsTextItem *textItem) {
    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        GraphicsTextItemUndoCommand *uc = new GraphicsTextItemUndoCommand(textItem);
        Application::undoStack->push(uc);
    }
}
GraphicsMediaItem *GraphicsScene::addMedia(const QUrl &pMediaFileUrl, bool shouldPlayAsap,
                                           const QPointF &pPos) {
    qDebug() << pMediaFileUrl.toLocalFile();

    // 本地路径与 URL 两种写法都试过再放弃（网络地址只后一种有意义）
    if (!QFile::exists(pMediaFileUrl.toLocalFile()) && !QFile::exists(pMediaFileUrl.toString()))
        return NULL;

    GraphicsMediaItem *mediaItem = GraphicsMediaItem::createMediaItem(pMediaFileUrl);

    // createMediaItem 对不支持的媒体类型返回空；原来只在 connect 前判空，
    // 紧接着就无条件 setPos，等于拖一个不支持的文件进来直接崩。
    if (!mediaItem)
        return NULL;

    connect(Application::boardController, &BoardController::activeSceneChanged, mediaItem,
            &GraphicsMediaItem::activeSceneChanged);

    mediaItem->setPos(pPos);

    mediaItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    mediaItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    addItem(mediaItem);

    mediaItem->show();

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, mediaItem);
        Application::undoStack->push(uc);
    }

    if (shouldPlayAsap)
        mediaItem->play();

    setDocumentUpdated();

    return mediaItem;
}

GraphicsMediaItem *GraphicsScene::addVideo(const QUrl &pVideoFileUrl, bool shouldPlayAsap,
                                           const QPointF &pPos) {
    return addMedia(pVideoFileUrl, shouldPlayAsap, pPos);
}

GraphicsMediaItem *GraphicsScene::addAudio(const QUrl &pAudioFileUrl, bool shouldPlayAsap,
                                           const QPointF &pPos) {
    return addMedia(pAudioFileUrl, shouldPlayAsap, pPos);
}

GraphicsWidgetItem *GraphicsScene::addWidget(const QUrl &pWidgetUrl, const QPointF &pPos) {
    int widgetType = GraphicsWidgetItem::widgetType(pWidgetUrl);

    if (widgetType == WidgetType::Apple) {
        return addAppleWidget(pWidgetUrl, pPos);
    } else if (widgetType == WidgetType::Web) {
        return addWebWidget(pWidgetUrl, pPos);
    } else {
        qDebug() << "GraphicsScene::addWidget: Unknown widget Type";
        return 0;
    }
}

GraphicsAppleWidgetItem *GraphicsScene::addAppleWidget(const QUrl &pWidgetUrl, const QPointF &pPos) {
    GraphicsAppleWidgetItem *appleWidget = new GraphicsAppleWidgetItem(pWidgetUrl);

    addGraphicsWidget(appleWidget, pPos);

    return appleWidget;
}

GraphicsWebWidgetItem *GraphicsScene::addWebWidget(const QUrl &pWidgetUrl, const QPointF &pPos) {
    GraphicsWebWidgetItem *webWidget = new GraphicsWebWidgetItem(pWidgetUrl, 0);

    addGraphicsWidget(webWidget, pPos);

    return webWidget;
}

void GraphicsScene::addGraphicsWidget(GraphicsWidgetItem *graphicsWidget, const QPointF &pPos) {
    graphicsWidget->setFlag(QGraphicsItem::ItemIsSelectable, true);

    addItem(graphicsWidget);

    qreal ssf = 1 / Application::boardController->systemScaleFactor();

    graphicsWidget->setTransform(QTransform::fromScale(ssf, ssf), true);

    graphicsWidget->setPos(QPointF(pPos.x() - graphicsWidget->boundingRect().width() / 2,
                                   pPos.y() - graphicsWidget->boundingRect().height() / 2));

    if (graphicsWidget->canBeContent()) {
        graphicsWidget->loadMainHtml();

        graphicsWidget->setSelected(true);
        if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
            GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, graphicsWidget);
            Application::undoStack->push(uc);
        }

        setDocumentUpdated();
    } else {
        Application::boardController->moveGraphicsWidgetToControlView(graphicsWidget);
    }

    Application::boardController->controlView()->setFocus();
}

GraphicsGroupContainerItem *GraphicsScene::createGroup(QList<QGraphicsItem *> items) {
    GraphicsGroupContainerItem *groupItem = new GraphicsGroupContainerItem();

    addItem(groupItem);
    foreach (QGraphicsItem *item, items) {
        if (item->type() == GraphicsGroupContainerItem::Type) {
            QList<QGraphicsItem *> childItems = item->childItems();
            GraphicsGroupContainerItem *currentGroup = dynamic_cast<GraphicsGroupContainerItem *>(item);
            if (currentGroup) {
                currentGroup->destroy();
            }
            foreach (QGraphicsItem *chItem, childItems) {
                groupItem->addToGroup(chItem);
                mFastAccessItems.removeAll(chItem);
            }
        } else {
            groupItem->addToGroup(item);
            mFastAccessItems.removeAll(item);
        }
    }

    groupItem->setVisible(true);
    groupItem->setFocus();

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        GraphicsItemGroupUndoCommand *uc = new GraphicsItemGroupUndoCommand(this, groupItem);
        Application::undoStack->push(uc);
    }

    setDocumentUpdated();

    return groupItem;
}

void GraphicsScene::addGroup(GraphicsGroupContainerItem *groupItem) {
    addItem(groupItem);
    for (int i = 0; i < groupItem->childItems().count(); i++) {
        QGraphicsItem *it = qgraphicsitem_cast<QGraphicsItem *>(groupItem->childItems().at(i));
        if (it) {
            mFastAccessItems.removeAll(it);
        }
    }

    groupItem->setVisible(true);
    groupItem->setFocus();

    if (groupItem->uuid().isNull()) {
        groupItem->setUuid(QUuid::createUuid());
    }

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, groupItem);
        Application::undoStack->push(uc);
    }

    setDocumentUpdated();
}

GraphicsSvgItem *GraphicsScene::addSvg(const QUrl &pSvgFileUrl, const QPointF &pPos, const QByteArray pData) {
    QString path = pSvgFileUrl.toLocalFile();

    GraphicsSvgItem *svgItem;
    if (pData.isNull())
        svgItem = new GraphicsSvgItem(path);
    else
        svgItem = new GraphicsSvgItem(pData);

    svgItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    svgItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

    qreal sscale = 1 / Application::boardController->systemScaleFactor();
    svgItem->setTransform(QTransform::fromScale(sscale, sscale), true);

    QPointF half(svgItem->boundingRect().width() / 2, svgItem->boundingRect().height() / 2);
    svgItem->setPos(pPos - half);
    svgItem->show();

    addItem(svgItem);

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, svgItem);
        Application::undoStack->push(uc);
    }

    setDocumentUpdated();

    QString documentPath = Application::boardController->selectedDocument()->persistencePath();

    QString fileName = PersistenceManager::imageDirectory + "/" + svgItem->uuid().toString() + ".svg";

    QString completePath = documentPath + "/" + fileName;

    if (!QFile::exists(completePath)) {
        QDir dir;
        dir.mkdir(documentPath + "/" + PersistenceManager::imageDirectory);

        QFile file(completePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "cannot open file for writing embeded svg content " << completePath;
            return NULL;
        }

        file.write(svgItem->fileData());
        file.close();
    }

    return svgItem;
}

GraphicsTextItem *GraphicsScene::addText(const QString &pString, const QPointF &pTopLeft) {
    return addTextWithFont(pString, pTopLeft, Settings::settings()->fontPixelSize(),
                           Settings::settings()->fontFamily(), Settings::settings()->isBoldFont(),
                           Settings::settings()->isItalicFont());
}

GraphicsTextItem *GraphicsScene::addTextWithFont(const QString &pString, const QPointF &pTopLeft,
                                                 int pointSize, const QString &fontFamily, bool bold,
                                                 bool italic) {
    GraphicsTextItem *textItem = new GraphicsTextItem();
    textItem->setPlainText(pString);

    QFont font = textItem->font();

    if (fontFamily == "") {
        font = QFont(Settings::settings()->fontFamily());
    } else {
        font = QFont(fontFamily);
    }

    if (pointSize < 1) {
        font.setPixelSize(Settings::settings()->fontPixelSize());
    } else {
        font.setPointSize(pointSize);
    }

    font.setBold(bold);
    font.setItalic(italic);
    textItem->setFont(font);

    QFontMetrics fi(font);
    QRect br = fi.boundingRect(pString);

    textItem->setTextWidth(qMax((qreal)br.width() + 50, (qreal)200));
    textItem->setTextHeight(br.height());

    addItem(textItem);

    textItem->setPos(pTopLeft);

    textItem->show();

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, textItem);
        Application::undoStack->push(uc);
    }

    connect(textItem, &GraphicsTextItem::textUndoCommandAdded, this, &GraphicsScene::textUndoCommandAdded);

    textItem->setSelected(true);
    textItem->setFocus();

    setDocumentUpdated();

    return textItem;
}

GraphicsTextItem *GraphicsScene::addTextHtml(const QString &pString, const QPointF &pTopLeft) {
    GraphicsTextItem *textItem = new GraphicsTextItem();
    textItem->setPlainText("");
    textItem->setHtml(TextTools::cleanHtml(pString));

    addItem(textItem);
    textItem->show();

    if (mUndoRedoStackEnabled) { //should be deleted after scene own undo stack implemented
        GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(this, 0, textItem);
        Application::undoStack->push(uc);
    }

    connect(textItem, &GraphicsTextItem::textUndoCommandAdded, this, &GraphicsScene::textUndoCommandAdded);

    textItem->setFocus();

    setDocumentUpdated();
    textItem->setPos(pTopLeft);

    return textItem;
}

void GraphicsScene::addItem(QGraphicsItem *item) {
    CoreGraphicsScene::addItem(item);

    // the default z value is already set. This is the case when a svg file is read
    if (item->zValue() == DEFAULT_Z_VALUE || item->zValue() == ZLayerController::errorNum() ||
        !mZLayerController->zLevelAvailable(item->zValue())) {
        qreal zvalue = mZLayerController->generateZLevel(item);
        GraphicsItem::assignZValue(item, zvalue);
    }

    else
        notifyZChanged(item, item->zValue());

    if (!mTools.contains(item))
        ++mItemCount;

    mFastAccessItems << item;
}

void GraphicsScene::addItems(const QSet<QGraphicsItem *> &items) {
    foreach (QGraphicsItem *item, items) {
        CoreGraphicsScene::addItem(item);
        GraphicsItem::assignZValue(item, mZLayerController->generateZLevel(item));
    }

    mItemCount += items.size();

    mFastAccessItems += items.values();
}

void GraphicsScene::removeItem(QGraphicsItem *item) {
    item->setSelected(false);
    CoreGraphicsScene::removeItem(item);
    Application::boardController->freezeWebWidget(item, true);

    if (!mTools.contains(item))
        --mItemCount;

    mFastAccessItems.removeAll(item);
    /* delete the item if it is cache to allow its reinstanciation, because Cache implements design pattern Singleton. */
    if (dynamic_cast<GraphicsCache *>(item))
        CoreGraphicsScene::deleteItem(item);
}

void GraphicsScene::removeItems(const QSet<QGraphicsItem *> &items) {
    foreach (QGraphicsItem *item, items)
        CoreGraphicsScene::removeItem(item);

    mItemCount -= items.size();

    foreach (QGraphicsItem *item, items)
        mFastAccessItems.removeAll(item);
}

void GraphicsScene::deselectAllItems() {
    foreach (QGraphicsItem *gi, selectedItems()) {
        gi->clearFocus();
        gi->setSelected(false);
        // Hide selection frame
        if (mSelectionFrame) {
            mSelectionFrame->setEnclosedItems(QList<QGraphicsItem *>());
        }
        GraphicsTextItem *textItem = dynamic_cast<GraphicsTextItem *>(gi);
        if (textItem)
            textItem->activateTextEditor(false);
    }
}

void GraphicsScene::deselectAllItemsExcept(QGraphicsItem *item) {
    foreach (QGraphicsItem *eachItem, selectedItems()) {
        if (eachItem != item) {
            eachItem->setSelected(false);

            GraphicsTextItem *textItem = dynamic_cast<GraphicsTextItem *>(eachItem);
            if (textItem)
                textItem->activateTextEditor(false);
        }
    }
}

/**
 * Return the bounding rectangle of all items on the page except for tools (ruler, compass,...)
 */
QRectF GraphicsScene::annotationsBoundingRect() const {
    QRectF boundingRect;

    foreach (QGraphicsItem *item, items()) {
        if (!mTools.contains(rootItem(item)))
            boundingRect |= item->sceneBoundingRect();
    }

    return boundingRect;
}

bool GraphicsScene::isEmpty() const {
    return mItemCount == 0;
}

QGraphicsItem *GraphicsScene::setAsBackgroundObject(QGraphicsItem *item, bool pAdaptTransformation,
                                                    bool pExpand) {
    if (mBackgroundObject) {
        removeItem(mBackgroundObject);
        mBackgroundObject = 0;
    }

    if (item) {
        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        item->setFlag(QGraphicsItem::ItemIsMovable, false);
        item->setAcceptedMouseButtons(Qt::NoButton);
        item->setData(GraphicsItemData::ItemLayerType, ItemLayerType::FixedBackground);

        if (pAdaptTransformation) {
            item = scaleToFitDocumentSize(item, true, 0, pExpand);
        }

        if (item->scene() != this)
            addItem(item);

        mZLayerController->setLayerType(item, itemLayerType::BackgroundItem);
        GraphicsItem::assignZValue(item, mZLayerController->generateZLevel(item));

        mBackgroundObject = item;
    }

    return item;
}

void GraphicsScene::unsetBackgroundObject() {
    if (!mBackgroundObject)
        return;

    mBackgroundObject->setFlag(QGraphicsItem::ItemIsSelectable, true);
    mBackgroundObject->setFlag(QGraphicsItem::ItemIsMovable, true);
    mBackgroundObject->setAcceptedMouseButtons(Qt::LeftButton);

    // Item zLayer and Layer Type should be set by the caller of this function, as
    // it may depend on the object type, where it was before, etc.

    mBackgroundObject = 0;
}

QRectF GraphicsScene::normalizedSceneRect(qreal ratio) {
    QRectF normalizedRect(nominalSize().width() / -2, nominalSize().height() / -2, nominalSize().width(),
                          nominalSize().height());

    foreach (QGraphicsItem *gi, mFastAccessItems) {
        if (gi && gi->isVisible() && !mTools.contains(gi)) {
            normalizedRect = normalizedRect.united(gi->sceneBoundingRect());
        }
    }

    if (ratio > 0.0) {
        qreal normalizedRectRatio = normalizedRect.width() / normalizedRect.height();

        if (normalizedRectRatio > ratio) {
            //the normalized rect is too wide, we increase height
            qreal newHeight = normalizedRect.width() / ratio;
            qreal offset = (newHeight - normalizedRect.height()) / 2;
            normalizedRect.setY(normalizedRect.y() - offset);
            normalizedRect.setHeight(newHeight);
        } else if (normalizedRectRatio < ratio) {
            //the normalized rect is too high, we increase the width
            qreal newWidth = normalizedRect.height() * ratio;
            qreal offset = (newWidth - normalizedRect.width()) / 2;
            normalizedRect.setX(normalizedRect.x() - offset);
            normalizedRect.setWidth(newWidth);
        }
    }

    return normalizedRect;
}

QGraphicsItem *GraphicsScene::itemForUuid(QUuid uuid) {
    QGraphicsItem *result = 0;
    QString ui = uuid.toString();

    //simple search before implementing container for fast access
    foreach (QGraphicsItem *item, items()) {
        if (GraphicsScene::getPersonalUuid(item) == uuid && !uuid.isNull()) {
            result = item;
        }
    }

    return result;
}

void GraphicsScene::setDocument(DocumentProxy *pDocument) {
    if (pDocument != mDocument) {
        if (mDocument) {
            setModified(true);
        }

        mDocument = pDocument;
        setParent(pDocument);
    }
}

QGraphicsItem *GraphicsScene::scaleToFitDocumentSize(QGraphicsItem *item, bool center, int margin,
                                                     bool expand) {
    int maxWidth = mNominalSize.width() - (margin * 2);
    int maxHeight = mNominalSize.height() - (margin * 2);

    QRectF size = item->sceneBoundingRect();

    if (expand || size.width() > maxWidth || size.height() > maxHeight) {
        qreal ratio = qMin(maxWidth / size.width(), maxHeight / size.height());

        item->setTransform(QTransform::fromScale(ratio, ratio), true);

        if (center) {
            item->setPos(item->sceneBoundingRect().width() / -2.0, item->sceneBoundingRect().height() / -2.0);
        }
    }

    return item;
}

void GraphicsScene::addRuler(QPointF center) {
    GraphicsRuler *ruler = new GraphicsRuler(); // mem : owned and destroyed by the scene
    mTools << ruler;
    QRectF rect = ruler->rect();
    ruler->setRect(center.x() - rect.width() / 2, center.y() - rect.height() / 2, rect.width(),
                   rect.height());

    ruler->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    addItem(ruler);

    ruler->setVisible(true);
}

void GraphicsScene::addProtractor(QPointF center) {
    // Protractor

    GraphicsProtractor *protractor = new GraphicsProtractor(); // mem : owned and destroyed by the scene
    mTools << protractor;

    protractor->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    addItem(protractor);

    QPointF itemSceneCenter = protractor->sceneBoundingRect().center();
    protractor->moveBy(center.x() - itemSceneCenter.x(), center.y() - itemSceneCenter.y());

    protractor->setVisible(true);
}

void GraphicsScene::addTriangle(QPointF center) {
    // Triangle

    GraphicsTriangle *triangle = new GraphicsTriangle(); // mem : owned and destroyed by the scene
    mTools << triangle;

    triangle->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    addItem(triangle);

    QPointF itemSceneCenter = triangle->sceneBoundingRect().center();
    triangle->moveBy(center.x() - itemSceneCenter.x(), center.y() - itemSceneCenter.y());

    triangle->setVisible(true);
}

void GraphicsScene::addMagnifier(MagnifierParams params) {
    // can have only one magnifier at one time
    if (magniferControlViewWidget)
        return;

    QWidget *cContainer = (QWidget *)(Application::boardController->controlContainer());
    QGraphicsView *cView = (QGraphicsView *)Application::boardController->controlView();
    QGraphicsView *dView = (QGraphicsView *)Application::boardController->displayView();

    QPoint dvZeroPoint = dView->mapToGlobal(QPoint(0, 0));

    int cvW = cView->width();
    int dvW = dView->width();
    qreal wCoeff = (qreal)dvW / (qreal)cvW;

    int cvH = cView->height();
    int dvH = dView->height();
    qreal hCoeff = (qreal)dvH / (qreal)cvH;

    QPoint ccPoint(params.x, params.y);
    QPoint globalPoint = cContainer->mapToGlobal(ccPoint);
    QPoint cvPoint = cView->mapFromGlobal(globalPoint);
    QPoint dvPoint(cvPoint.x() * wCoeff + dvZeroPoint.x(), cvPoint.y() * hCoeff + dvZeroPoint.y());

    magniferControlViewWidget =
        new Magnifier((QWidget *)(Application::boardController->controlContainer()), true);
    magniferControlViewWidget->setGrabView((QGraphicsView *)Application::boardController->controlView());
    magniferControlViewWidget->setMoveView((QWidget *)(Application::boardController->controlContainer()));
    magniferControlViewWidget->setSize(params.sizePercentFromScene);
    magniferControlViewWidget->setZoom(params.zoom);

    magniferDisplayViewWidget =
        new Magnifier((QWidget *)(Application::boardController->displayView()), false);
    magniferDisplayViewWidget->setGrabView((QGraphicsView *)Application::boardController->controlView());
    magniferDisplayViewWidget->setMoveView((QGraphicsView *)Application::boardController->displayView());
    magniferDisplayViewWidget->setSize(params.sizePercentFromScene);
    magniferDisplayViewWidget->setZoom(params.zoom);

    magniferControlViewWidget->grabNMove(globalPoint, globalPoint, true);
    magniferDisplayViewWidget->grabNMove(globalPoint, dvPoint, true);
    magniferControlViewWidget->show();
    magniferDisplayViewWidget->show();

    connect(magniferControlViewWidget, &Magnifier::magnifierMoved_Signal, this,
            [this](QPoint newPos) { moveMagnifier(newPos); });
    connect(magniferControlViewWidget, &Magnifier::magnifierClose_Signal, this,
            &GraphicsScene::closeMagnifier);
    connect(magniferControlViewWidget, &Magnifier::magnifierZoomIn_Signal, this,
            &GraphicsScene::zoomInMagnifier);
    connect(magniferControlViewWidget, &Magnifier::magnifierZoomOut_Signal, this,
            &GraphicsScene::zoomOutMagnifier);
    connect(magniferControlViewWidget, &Magnifier::magnifierDrawingModeChange_Signal, this,
            &GraphicsScene::changeMagnifierMode);
    connect(magniferControlViewWidget, &Magnifier::magnifierResized_Signal, this,
            &GraphicsScene::resizedMagnifier);

    setModified(true);
}

void GraphicsScene::moveMagnifier() {
    if (magniferControlViewWidget) {
        QPoint magnifierPos =
            QPoint(magniferControlViewWidget->pos().x() + magniferControlViewWidget->size().width() / 2,
                   magniferControlViewWidget->pos().y() + magniferControlViewWidget->size().height() / 2);
        moveMagnifier(magnifierPos, true);
        setModified(true);
    }
}

void GraphicsScene::moveMagnifier(QPoint newPos, bool forceGrab) {
    QWidget *cContainer = (QWidget *)(Application::boardController->controlContainer());
    QGraphicsView *cView = (QGraphicsView *)Application::boardController->controlView();
    QGraphicsView *dView = (QGraphicsView *)Application::boardController->displayView();

    QPoint dvZeroPoint = dView->mapToGlobal(QPoint(0, 0));

    int cvW = cView->width();
    int dvW = dView->width();
    qreal wCoeff = (qreal)dvW / (qreal)cvW;

    int cvH = cView->height();
    int dvH = dView->height();
    qreal hCoeff = (qreal)dvH / (qreal)cvH;

    QPoint globalPoint = cContainer->mapToGlobal(newPos);
    QPoint cvPoint = cView->mapFromGlobal(globalPoint);
    QPoint dvPoint(cvPoint.x() * wCoeff + dvZeroPoint.x(), cvPoint.y() * hCoeff + dvZeroPoint.y());

    magniferControlViewWidget->grabNMove(globalPoint, globalPoint, forceGrab, false);
    magniferDisplayViewWidget->grabNMove(globalPoint, dvPoint, forceGrab, true);

    setModified(true);
}

void GraphicsScene::closeMagnifier() {
    DisposeMagnifierQWidgets();
    setModified(true);
}

void GraphicsScene::zoomInMagnifier() {
    if (magniferControlViewWidget->zoom() < 8) {
        magniferControlViewWidget->setZoom(magniferControlViewWidget->zoom() + 0.5);
        magniferDisplayViewWidget->setZoom(magniferDisplayViewWidget->zoom() + 0.5);
    }
}

void GraphicsScene::zoomOutMagnifier() {
    if (magniferControlViewWidget->zoom() > 1) {
        magniferControlViewWidget->setZoom(magniferControlViewWidget->zoom() - 0.5);
        magniferDisplayViewWidget->setZoom(magniferDisplayViewWidget->zoom() - 0.5);
        setModified(true);
    }
}

void GraphicsScene::changeMagnifierMode(int mode) {
    if (magniferControlViewWidget)
        magniferControlViewWidget->setDrawingMode(mode);
}

void GraphicsScene::resizedMagnifier(qreal newPercent) {
    if (newPercent > 18 && newPercent < 50) {
        magniferControlViewWidget->setSize(newPercent);
        magniferControlViewWidget->grabPoint();
        magniferDisplayViewWidget->setSize(newPercent);
        magniferDisplayViewWidget->grabPoint();
        setModified(true);
    }
}

void GraphicsScene::addCompass(QPointF center) {
    GraphicsCompass *compass = new GraphicsCompass(); // mem : owned and destroyed by the scene
    mTools << compass;
    addItem(compass);

    QRectF rect = compass->rect();
    compass->setRect(center.x() - rect.width() / 2, center.y() - rect.height() / 2, rect.width(),
                     rect.height());

    compass->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    compass->setVisible(true);
}

void GraphicsScene::addCache() {
    GraphicsCache *cache = GraphicsCache::instance(this);
    if (!items().contains(cache)) {
        addItem(cache);

        cache->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

        cache->setVisible(true);
        cache->setSelected(true);
        Application::boardController->notifyCache(true);
        Application::boardController->notifyPageChanged();
    }
}

void GraphicsScene::addMask(const QPointF &center) {
    GraphicsCurtainItem *curtain = new GraphicsCurtainItem(); // mem : owned and destroyed by the scene
    mTools << curtain;

    addItem(curtain);

    QRectF rect = Application::boardController->activeScene()->normalizedSceneRect();
    rect.setRect(center.x() - rect.width() / 4, center.y() - rect.height() / 4, rect.width() / 2,
                 rect.height() / 2);
    curtain->setRect(rect);
    curtain->setVisible(true);
    curtain->setSelected(true);
}

void GraphicsScene::setRenderingQuality(Item::RenderingQuality pRenderingQuality) {
    QListIterator<QGraphicsItem *> itItems(mFastAccessItems);

    while (itItems.hasNext()) {
        QGraphicsItem *gItem = itItems.next();

        Item *boardItem = dynamic_cast<Item *>(gItem);

        if (boardItem) {
            boardItem->setRenderingQuality(pRenderingQuality);
        }
    }
}

QList<QUrl> GraphicsScene::relativeDependencies() const {
    QList<QUrl> relativePathes;

    QListIterator<QGraphicsItem *> itItems(mFastAccessItems);

    while (itItems.hasNext()) {
        QGraphicsItem *item = itItems.next();

        GraphicsVideoItem *videoItem = qgraphicsitem_cast<GraphicsVideoItem *>(item);
        if (videoItem) {
            QString completeFileName = QFileInfo(videoItem->mediaFileUrl().toLocalFile()).fileName();
            QString path = PersistenceManager::videoDirectory + "/";
            relativePathes << QUrl(path + completeFileName);
            continue;
        }

        GraphicsAudioItem *audioItem = qgraphicsitem_cast<GraphicsAudioItem *>(item);
        if (audioItem) {
            QString completeFileName = QFileInfo(audioItem->mediaFileUrl().toLocalFile()).fileName();
            QString path = PersistenceManager::audioDirectory + "/";
            relativePathes << QUrl(path + completeFileName);
            continue;
        }

        GraphicsWidgetItem *widget = qgraphicsitem_cast<GraphicsWidgetItem *>(item);
        if (widget) {
            QString widgetPath =
                PersistenceManager::widgetDirectory + "/" + widget->uuid().toString() + ".wgt";
            QString screenshotPath = PersistenceManager::widgetDirectory + "/" +
                                     widget->uuid().toString().remove("{").remove("}") + ".png";
            relativePathes << QUrl(widgetPath);
            relativePathes << QUrl(screenshotPath);
            continue;
        }

        GraphicsPixmapItem *pixmapItem = qgraphicsitem_cast<GraphicsPixmapItem *>(item);
        if (pixmapItem) {
            relativePathes << QUrl(PersistenceManager::imageDirectory + "/" + pixmapItem->uuid().toString() +
                                   ".png");
            continue;
        }

        GraphicsSvgItem *svgItem = qgraphicsitem_cast<GraphicsSvgItem *>(item);
        if (svgItem) {
            relativePathes << QUrl(PersistenceManager::imageDirectory + "/" + svgItem->uuid().toString() +
                                   ".svg");
            continue;
        }
    }

    return relativePathes;
}

QSize GraphicsScene::nominalSize() {
    if (mDocument && !mNominalSize.isValid()) {
        mNominalSize = mDocument->defaultDocumentSize();
    }

    return mNominalSize;
}

/**
 * @brief Return the scene's boundary size, including any background item
 *
 * If no background item is present, this returns nominalSize()
 */
QSize GraphicsScene::sceneSize() {
    GraphicsPDFItem *pdfItem = qgraphicsitem_cast<GraphicsPDFItem *>(backgroundObject());

    if (pdfItem) {
        QRectF targetRect = pdfItem->sceneBoundingRect();
        return targetRect.size().toSize();
    }

    else
        return nominalSize();
}

void GraphicsScene::setNominalSize(const QSize &pSize) {
    if (nominalSize() != pSize) {
        mNominalSize = pSize;

        if (mDocument)
            mDocument->setDefaultDocumentSize(pSize);
    }
}

void GraphicsScene::setNominalSize(int pWidth, int pHeight) {
    setNominalSize(QSize(pWidth, pHeight));
}

void GraphicsScene::setSelectedZLevel(QGraphicsItem *item) {
    item->setZValue(mZLayerController->generateZLevel(itemLayerType::SelectedItem));
}

void GraphicsScene::setOwnZlevel(QGraphicsItem *item) {
    item->setZValue(item->data(GraphicsItemData::ItemOwnZValue).toReal());
}

QUuid GraphicsScene::getPersonalUuid(QGraphicsItem *item) {
    QString idCandidate = item->data(GraphicsItemData::ItemUuid).toString();
    return idCandidate == QUuid().toString() ? QUuid() : QUuid(idCandidate);
}

qreal GraphicsScene::changeZLevelTo(QGraphicsItem *item, ZLayerController::moveDestination dest,
                                    bool addUndo) {
    qreal previousZVal = item->data(GraphicsItemData::ItemOwnZValue).toReal();
    qreal res = mZLayerController->changeZLevelTo(item, dest);

    if (addUndo) {
        GraphicsItemZLevelUndoCommand *uc = new GraphicsItemZLevelUndoCommand(this, item, previousZVal, dest);
        Application::undoStack->push(uc);
    }

    return res;
}

QGraphicsItem *GraphicsScene::rootItem(QGraphicsItem *item) const {
    QGraphicsItem *root = item;

    while (root->parentItem()) {
        root = root->parentItem();
    }

    return root;
}

void GraphicsScene::drawItems(QPainter *painter, int numItems, QGraphicsItem *items[],
                              const QStyleOptionGraphicsItem options[], QWidget *widget) {
    if (mRenderingContext == NonScreen || mRenderingContext == PdfExport) {
        int count = 0;

        QGraphicsItem **itemsFiltered = new QGraphicsItem *[numItems];
        QStyleOptionGraphicsItem *optionsFiltered = new QStyleOptionGraphicsItem[numItems];

        for (int i = 0; i < numItems; i++) {
            if (!mTools.contains(rootItem(items[i]))) {
                bool isPdfItem = qgraphicsitem_cast<GraphicsPDFItem *>(items[i]) != NULL;
                if (!isPdfItem || mRenderingContext == NonScreen) {
                    itemsFiltered[count] = items[i];
                    optionsFiltered[count] = options[i];
                    count++;
                }
            }
        }

        QGraphicsScene::drawItems(painter, count, itemsFiltered, optionsFiltered, widget);

        delete[] optionsFiltered;
        delete[] itemsFiltered;

    } else if (mRenderingContext == Recording) {
        int count = 0;

        QGraphicsItem **itemsFiltered = new QGraphicsItem *[numItems];
        QStyleOptionGraphicsItem *optionsFiltered = new QStyleOptionGraphicsItem[numItems];

        for (int i = 0; i < numItems; i++) {
            bool ok;
            int itemLayerType = items[i]->data(GraphicsItemData::ItemLayerType).toInt(&ok);
            if (ok &&
                (itemLayerType >= ItemLayerType::FixedBackground && itemLayerType <= ItemLayerType::Tool)) {
                itemsFiltered[count] = items[i];
                optionsFiltered[count] = options[i];
                count++;
            }
        }

        QGraphicsScene::drawItems(painter, count, itemsFiltered, optionsFiltered, widget);

        delete[] optionsFiltered;
        delete[] itemsFiltered;

    } else {
        QGraphicsScene::drawItems(painter, numItems, items, options, widget);
    }
}

void GraphicsScene::drawBackground(QPainter *painter, const QRectF &rect) {
    if (mIsDesktopMode) {
        QGraphicsScene::drawBackground(painter, rect);
        return;
    }
    bool darkBackground = isDarkBackground();

    if (darkBackground) {
        painter->fillRect(rect, QBrush(QColor(Qt::black)));
    } else {
        painter->fillRect(rect, QBrush(QColor(Qt::white)));
    }

    if (mZoomFactor > 0.5) {
        QColor bgCrossColor;

        if (darkBackground)
            bgCrossColor = QColor(Settings::settings()->boardCrossColorDarkBackground->get().toString());
        else
            bgCrossColor = QColor(Settings::settings()->boardCrossColorLightBackground->get().toString());
        if (mZoomFactor < 0.7) {
            int alpha = 255 * mZoomFactor / 2;
            bgCrossColor.setAlpha(alpha); // fade the crossing on small zooms
        }

        painter->setPen(bgCrossColor);

        if (mPageBackground == PageBackground::crossed) {
            qreal firstY = ((int)(rect.y() / backgroundGridSize())) * backgroundGridSize();

            for (qreal yPos = firstY; yPos < rect.y() + rect.height(); yPos += backgroundGridSize()) {
                painter->drawLine(rect.x(), yPos, rect.x() + rect.width(), yPos);
            }

            qreal firstX = ((int)(rect.x() / backgroundGridSize())) * backgroundGridSize();

            for (qreal xPos = firstX; xPos < rect.x() + rect.width(); xPos += backgroundGridSize()) {
                painter->drawLine(xPos, rect.y(), xPos, rect.y() + rect.height());
            }
        }

        else if (mPageBackground == PageBackground::ruled) {
            qreal firstY = ((int)(rect.y() / backgroundGridSize())) * backgroundGridSize();

            for (qreal yPos = firstY; yPos < rect.y() + rect.height(); yPos += backgroundGridSize()) {
                painter->drawLine(rect.x(), yPos, rect.x() + rect.width(), yPos);
            }
        }
    }
}

void GraphicsScene::keyReleaseEvent(QKeyEvent *keyEvent) {
    QList<QGraphicsItem *> si = selectedItems();

    if (keyEvent->matches(QKeySequence::SelectAll)) {
        QListIterator<QGraphicsItem *> itItems(this->mFastAccessItems);

        while (itItems.hasNext())
            itItems.next()->setSelected(true);

        keyEvent->accept();
        return;
    }

    if ((si.size() > 0) && (keyEvent->isAccepted())) {
#ifdef Q_OS_MAC
        if (keyEvent->key() == Qt::Key_Backspace)
#else
        if (keyEvent->matches(QKeySequence::Delete))
#endif
        {
            QVector<GraphicsItem *> itemsToRemove;
            QVector<QGraphicsItem *> itemToRemove;

            bool bRemoveOk = true;

            foreach (QGraphicsItem *item, si) {
                switch (item->type()) {
                    case GraphicsWidgetItem::Type: {
                        GraphicsWebWidgetItem *wc3_widget = dynamic_cast<GraphicsWebWidgetItem *>(item);
                        if (0 != wc3_widget)
                            if (!wc3_widget->hasFocus())
                                itemsToRemove << wc3_widget;
                        break;
                    }
                    case GraphicsTextItem::Type: {
                        GraphicsTextItem *text_item = dynamic_cast<GraphicsTextItem *>(item);
                        if (0 != text_item) {
                            if (!text_item->hasFocus())
                                itemsToRemove << text_item;
                            else
                                bRemoveOk = false;
                        }
                        break;
                    }

                    case GraphicsGroupContainerItem::Type: {
                        GraphicsGroupContainerItem *group_item =
                            dynamic_cast<GraphicsGroupContainerItem *>(item);
                        if (NULL != group_item) {
                            if (!hasTextItemWithFocus(group_item))
                                itemsToRemove << group_item;
                            else
                                bRemoveOk = false;
                        }
                        break;
                    }

                    default: {
                        GraphicsItem *ubgi = dynamic_cast<GraphicsItem *>(item);
                        if (0 != ubgi)
                            itemsToRemove << ubgi;
                        else
                            itemToRemove << item;
                    }
                }
            }

            if (bRemoveOk) {
                foreach (GraphicsItem *item, itemsToRemove) {
                    item->remove();
                }
                foreach (QGraphicsItem *pItem, itemToRemove) {
                    CoreGraphicsScene::removeItem(pItem);
                }
            }
        }

        keyEvent->accept();
    }

    QGraphicsScene::keyReleaseEvent(keyEvent);
}

bool GraphicsScene::hasTextItemWithFocus(GraphicsGroupContainerItem *item) {
    bool bHasFocus = false;

    foreach (QGraphicsItem *pItem, item->childItems()) {
        GraphicsTextItem *text_item = dynamic_cast<GraphicsTextItem *>(pItem);
        if (NULL != text_item) {
            if (text_item->hasFocus()) {
                bHasFocus = true;
                break;
            }
        }
    }

    return bHasFocus;
}


void GraphicsScene::simplifyCurrentStroke() {
    if (!mCurrentStroke)
        return;

    GraphicsStroke *simplerStroke = mCurrentStroke->simplify();
    if (!simplerStroke)
        return;

    foreach (GraphicsPolygonItem *poly, mCurrentStroke->polygons()) {
        mPreviousPolygonItems.removeAll(poly);
        removeItem(poly);
    }

    mCurrentStroke = simplerStroke;

    foreach (GraphicsPolygonItem *poly, mCurrentStroke->polygons()) {
        addItem(poly);
        mPreviousPolygonItems.append(poly);
    }
}

void GraphicsScene::setDocumentUpdated() {
    if (document())
        document()->setMetaData(Settings::documentUpdatedAt,
                                StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
}

void GraphicsScene::createEraiser() {
    if (Settings::settings()->showEraserPreviewCircle->get().toBool()) {
        mEraser = new QGraphicsEllipseItem(); // mem : owned and destroyed by the scene
        mEraser->setRect(QRect(0, 0, 0, 0));
        mEraser->setVisible(false);

        updateEraserColor();

        mEraser->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));
        mEraser->setData(
            GraphicsItemData::itemLayerType,
            QVariant(itemLayerType::Eraiser)); //Necessary to set if we want z value to be assigned correctly

        mTools << mEraser;
        addItem(mEraser);
    }
}

void GraphicsScene::createPointer() {
    mPointer = new QGraphicsEllipseItem(); // mem : owned and destroyed by the scene
    mPointer->setRect(QRect(0, 0, 20, 20));
    mPointer->setVisible(false);

    mPointer->setPen(Qt::NoPen);
    mPointer->setBrush(QBrush(QColor(255, 0, 0, 186)));

    mPointer->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));
    mPointer->setData(
        GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::Pointer)); //Necessary to set if we want z value to be assigned correctly

    mTools << mPointer;
    addItem(mPointer);
}

void GraphicsScene::createMarkerCircle() {
    if (Settings::settings()->showMarkerPreviewCircle->get().toBool()) {
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

void GraphicsScene::createPenCircle() {
    if (Settings::settings()->showPenPreviewCircle->get().toBool()) {
        mPenCircle = new QGraphicsEllipseItem();

        mPenCircle->setRect(QRect(0, 0, 0, 0));
        mPenCircle->setVisible(false);

        mPenCircle->setPen(Qt::DotLine);
        updatePenCircleColor();

        mPenCircle->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));
        mPenCircle->setData(GraphicsItemData::itemLayerType, QVariant(itemLayerType::Eraiser));

        mTools << mPenCircle;
        addItem(mPenCircle);
    }
}

void GraphicsScene::updateEraserColor() {
    if (!mEraser)
        return;

    if (mDarkBackground) {
        mEraser->setBrush(Settings::eraserBrushDarkBackground);
        mEraser->setPen(Settings::eraserPenDarkBackground);
    }

    else {
        mEraser->setBrush(Settings::eraserBrushLightBackground);
        mEraser->setPen(Settings::eraserPenLightBackground);
    }
}

void GraphicsScene::updateMarkerCircleColor() {
    if (!mMarkerCircle)
        return;

    QPen mcPen = mMarkerCircle->pen();

    if (mDarkBackground) {
        mcPen.setColor(Settings::markerCirclePenColorDarkBackground);
        mMarkerCircle->setBrush(Settings::markerCircleBrushColorDarkBackground);
    }

    else {
        mcPen.setColor(Settings::markerCirclePenColorLightBackground);
        mMarkerCircle->setBrush(Settings::markerCircleBrushColorLightBackground);
    }

    mcPen.setStyle(Qt::DotLine);
    mMarkerCircle->setPen(mcPen);
}

void GraphicsScene::updatePenCircleColor() {
    if (!mPenCircle)
        return;

    QPen mcPen = mPenCircle->pen();

    if (mDarkBackground) {
        mcPen.setColor(Settings::penCirclePenColorDarkBackground);
        mPenCircle->setBrush(Settings::penCircleBrushColorDarkBackground);
    }

    else {
        mcPen.setColor(Settings::penCirclePenColorLightBackground);
        mPenCircle->setBrush(Settings::penCircleBrushColorLightBackground);
    }

    mcPen.setStyle(Qt::DotLine);
    mPenCircle->setPen(mcPen);
}

void GraphicsScene::setToolCursor(int tool) {
    if (tool == (int)StylusTool::Selector || tool == (int)StylusTool::Text || tool == (int)StylusTool::Play) {
        deselectAllItems();
        hideMarkerCircle();
        hidePenCircle();
    }

    if (mCurrentStroke && mCurrentStroke->polygons().empty()) {
        delete mCurrentStroke;
        mCurrentStroke = NULL;
    }
}

void GraphicsScene::initStroke() {
    mCurrentStroke = new GraphicsStroke(this);
}
