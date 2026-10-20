#include "whiteboard/AbstractDrawRuler.h"
#include <QtSvg>
#include <QtSvgWidgets>
#include "utils/Enums.h"
#include "ui/Resources.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/DrawingController.h"
#include "core/Application.h"
#include "whiteboard/BoardController.h"

const QColor AbstractDrawRuler::sLightBackgroundMiddleFillColor = QColor(0x72, 0x72, 0x72, sFillTransparency);
const QColor AbstractDrawRuler::sLightBackgroundEdgeFillColor   = QColor(0xc3, 0xc3, 0xc3, sFillTransparency);
const QColor AbstractDrawRuler::sLightBackgroundDrawColor       = QColor(0x33, 0x33, 0x33, sDrawTransparency);
const QColor AbstractDrawRuler::sDarkBackgroundMiddleFillColor  = QColor(0xb5, 0xb5, 0xb5, sFillTransparency);
const QColor AbstractDrawRuler::sDarkBackgroundEdgeFillColor    = QColor(0xdd, 0xdd, 0xdd, sFillTransparency);
const QColor AbstractDrawRuler::sDarkBackgroundDrawColor        = QColor(0xff, 0xff, 0xff, sDrawTransparency);

const int AbstractDrawRuler::sLeftEdgeMargin                    = 10;
const int AbstractDrawRuler::sDegreeToQtAngleUnit               = 16;
const int AbstractDrawRuler::sRotationRadius                    = 15;
const int AbstractDrawRuler::sFillTransparency                  = 127;
const int AbstractDrawRuler::sDrawTransparency                  = 192;
const int AbstractDrawRuler::sRoundingRadius                    = sLeftEdgeMargin / 2;

AbstractDrawRuler::AbstractDrawRuler() : mShowButtons(false), mAntiScaleRatio(1.0)
{
    sPixelsPerCentimeter = Application::boardController->activeScene()->backgroundGridSize();
}

void AbstractDrawRuler::create(QGraphicsItem &item)
{
    item.setFlag(QGraphicsItem::ItemIsMovable, true);
    item.setFlag(QGraphicsItem::ItemIsSelectable, true);
    item.setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    item.setAcceptHoverEvents(true);

    mCloseSvgItem = new QGraphicsSvgItem(":/images/closeTool.svg", &item);
    mCloseSvgItem->setVisible(false);
    mCloseSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));
}

AbstractDrawRuler::~AbstractDrawRuler()
{
}

QCursor AbstractDrawRuler::moveCursor() const
{
    return Qt::SizeAllCursor;
}

QCursor AbstractDrawRuler::rotateCursor() const
{
    return Resources::resources()->rotateCursor;
}

QCursor AbstractDrawRuler::closeCursor() const
{
    return Qt::ArrowCursor;
}

QCursor AbstractDrawRuler::drawRulerLineCursor() const
{
    return Resources::resources()->drawLineRulerCursor;
}

QColor AbstractDrawRuler::drawColor() const
{
    return scene()->isDarkBackground() ? sDarkBackgroundDrawColor : sLightBackgroundDrawColor;
}

QColor AbstractDrawRuler::middleFillColor() const
{
    return scene()->isDarkBackground() ? sDarkBackgroundMiddleFillColor : sLightBackgroundMiddleFillColor;
}

QColor AbstractDrawRuler::edgeFillColor() const
{
    return scene()->isDarkBackground() ? sDarkBackgroundEdgeFillColor : sLightBackgroundEdgeFillColor;
}

QFont AbstractDrawRuler::font() const
{
    QFont font("Arial");
    font.setPixelSize(16);
    return font;
}

void AbstractDrawRuler::StartLine(const QPointF &position, qreal width)
{
    Q_UNUSED(position);
    Q_UNUSED(width);
}
void AbstractDrawRuler::DrawLine(const QPointF &position, qreal width)
{
    Q_UNUSED(position);
    Q_UNUSED(width);
}
void AbstractDrawRuler::EndLine()
{
}

void AbstractDrawRuler::paint()
{
    mAntiScaleRatio = 1 / (Application::boardController->systemScaleFactor() * Application::boardController->currentZoom());
    QTransform antiScaleTransform;
    antiScaleTransform.scale(mAntiScaleRatio, mAntiScaleRatio);

    mCloseSvgItem->setTransform(antiScaleTransform);
    mCloseSvgItem->setPos(closeButtonRect().topLeft());
}
