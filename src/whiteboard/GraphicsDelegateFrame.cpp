#include "whiteboard/GraphicsDelegateFrame.h"

#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "core/Application.h"
#include "settings/Settings.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"

#include "whiteboard/GraphicsItemDelegate.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsProxyWidget.h"

#include "model/Resources.h"

GraphicsDelegateFrame::GraphicsDelegateFrame(GraphicsItemDelegate *pDelegate, QRectF pRect, qreal pFrameWidth, bool respectRatio, bool hasTitleBar)
    : QGraphicsRectItem(), QObject(pDelegate), mCurrentTool(None), mDelegate(pDelegate), mVisible(true), mFrameWidth(pFrameWidth),
      mNominalFrameWidth(pFrameWidth), mRespectRatio(respectRatio), mAngle(0), mAngleOffset(0), mTotalScaleX(-1), mTotalScaleY(-1), mTranslateX(0),
      mTranslateY(0), mTotalTranslateX(0), mTotalTranslateY(0), mOperationMode(Scaling), mFlippedX(false), mFlippedY(false), mMirrorX(false),
      mMirrorY(false), mTitleBarHeight(hasTitleBar ? 20 : 0), mNominalTitleBarHeight(hasTitleBar ? 20 : 0)
{
    mAngleTolerance = Settings::settings()->angleTolerance->get().toReal();

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setAcceptedMouseButtons(Qt::LeftButton);
    // 框架矩形在内容区域外扩一个边框宽度，有标题栏时顶部再多留出标题高度
    setRect(pRect.adjusted(mFrameWidth, mFrameWidth + mTitleBarHeight, mFrameWidth * -1, mFrameWidth * -1));

    setBrush(QBrush(Settings::paletteColor));
    setPen(Qt::NoPen);
    setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    // SVG 把手只负责显示，真正的命中区域由下面的透明矩形承担
    mBottomRightResizeGripSvgItem = new QGraphicsSvgItem(":/images/resize.svg", this);
    mBottomResizeGripSvgItem      = new QGraphicsSvgItem(":/images/resizeBottom.svg", this);
    mLeftResizeGripSvgItem        = new QGraphicsSvgItem(":/images/resizeLeft.svg", this);
    mRightResizeGripSvgItem       = new QGraphicsSvgItem(":/images/resizeRight.svg", this);
    mTopResizeGripSvgItem         = new QGraphicsSvgItem(":/images/resizeTop.svg", this);

    // 透明矩形让把手的可点击范围略大于图标本身，便于细边框上操作
    mBottomRightResizeGrip        = new QGraphicsRectItem(this);
    mBottomRightResizeGrip->setPen(Qt::NoPen);
    mBottomResizeGrip = new QGraphicsRectItem(this);
    mBottomResizeGrip->setPen(Qt::NoPen);
    mLeftResizeGrip = new QGraphicsRectItem(this);
    mLeftResizeGrip->setPen(Qt::NoPen);
    mRightResizeGrip = new QGraphicsRectItem(this);
    mRightResizeGrip->setPen(Qt::NoPen);
    mTopResizeGrip = new QGraphicsRectItem(this);
    mTopResizeGrip->setPen(Qt::NoPen);

    mRotateButton = new QGraphicsSvgItem(":/images/rotate.svg", this);
    mRotateButton->setCursor(Resources::resources()->rotateCursor);
    // 不可旋转的图形项不显示旋转按钮
    mRotateButton->setVisible(mDelegate->testFrameFlags(GF_REVOLVABLE));

    updateResizeCursors();

    setAntiScale(1.0);

    positionHandles();

    this->setAcceptHoverEvents(true);
}

GraphicsDelegateFrame::~GraphicsDelegateFrame()
{
}

void GraphicsDelegateFrame::setAntiScale(qreal pAntiScale)
{
    // 视图缩放时反向缩放边框宽度和把手图标，使其在屏幕上保持固定大小
    mFrameWidth     = mNominalFrameWidth * pAntiScale;
    mTitleBarHeight = mNominalTitleBarHeight * pAntiScale;

    QTransform tr;
    tr.scale(pAntiScale, pAntiScale);

    mBottomRightResizeGripSvgItem->setTransform(tr);
    mBottomResizeGripSvgItem->setTransform(tr);
    mLeftResizeGripSvgItem->setTransform(tr);
    mRightResizeGripSvgItem->setTransform(tr);
    mTopResizeGripSvgItem->setTransform(tr);
    mRotateButton->setTransform(tr);
}

void GraphicsDelegateFrame::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPainterPath path;
    path.addRoundedRect(rect(), mFrameWidth / 2, mFrameWidth / 2);

    if (rect().width() > 1 && rect().height() > 1)
    {
        // 从圆角外框中挖掉内部矩形，只保留一圈边框；有标题栏时内部区域顶部下移
        QPainterPath extruded;
        extruded.addRect(rect().adjusted(mFrameWidth, mFrameWidth + mTitleBarHeight, (mFrameWidth * -1), (mFrameWidth * -1)));
        path = path.subtracted(extruded);
    }

    painter->fillPath(path, brush());
}

QPainterPath GraphicsDelegateFrame::shape() const
{
    QPainterPath path;

    // 命中区域用直角矩形而非圆角，避免右下角圆弧导致缩放手把无法被点中
    path.addRect(rect());

    if (rect().width() > 0 && rect().height() > 0)
    {
        QPainterPath extruded;
        extruded.addRect(rect().adjusted(mFrameWidth, mFrameWidth + mTitleBarHeight, mFrameWidth * -1, mFrameWidth * -1));
        path = path.subtracted(extruded);
    }

    return path;
}

void GraphicsDelegateFrame::initializeTransform()
{
    // 用四个角在场景中的映射反推当前的旋转角、总缩放和翻转状态，
    // 之后所有拖动都基于这份“起始状态”重新计算，避免逐帧累积误差
    QTransform itemTransform = delegated()->sceneTransform();
    QRectF     itemRect      = delegated()->boundingRect();
    QPointF    topLeft       = itemTransform.map(itemRect.topLeft());
    QPointF    topRight      = itemTransform.map(itemRect.topRight());
    QPointF    bottomLeft    = itemTransform.map(itemRect.bottomLeft());

    qreal horizontalFlip     = (topLeft.x() > topRight.x()) ? -1 : 1;
    mMirrorX                 = horizontalFlip < 0;
    if (horizontalFlip < 0)
    {
        // 平移计算以“距离最近的边”为基准，水平翻转时必须先交换左右角，
        // 否则会选错锚点。
        QPointF tmp = topLeft;
        topLeft     = topRight;
        topRight    = tmp;

        // 高度按边长计算而非 y 差值，因此翻转后左下角要取原始右下角
        bottomLeft  = itemTransform.map(itemRect.bottomRight());
    }

    qreal verticalFlip = (bottomLeft.y() < topLeft.y()) ? -1 : 1;
    mMirrorY           = verticalFlip < 0;
    if (verticalFlip < 0 && !mMirrorX)
    {
        topLeft    = itemTransform.map(itemRect.bottomLeft());
        topRight   = itemTransform.map(itemRect.bottomRight());
        bottomLeft = itemTransform.map(itemRect.topLeft());
    }

    QLineF topLine(topLeft, topRight);
    QLineF leftLine(topLeft, bottomLeft);
    qreal  width  = topLine.length();
    qreal  height = leftLine.length();

    mAngle        = topLine.angle();

    // 边长丢失了方向信息，需乘回翻转符号才能还原带正负的缩放
    mTotalScaleX  = (width / itemRect.width()) * horizontalFlip;
    mTotalScaleY  = height / itemRect.height() * verticalFlip;

    // 由缩放和旋转组成的局部变换，其平移分量即是需要从整体变换中剥离的部分
    QTransform tr;
    QPointF    center = delegated()->boundingRect().center();
    tr.translate(center.x() * mTotalScaleX, center.y() * mTotalScaleY);
    tr.rotate(-mAngle);
    tr.translate(-center.x() * mTotalScaleX, -center.y() * mTotalScaleY);
    tr.scale(mTotalScaleX, mTotalScaleY);

    mTotalTranslateX = delegated()->transform().dx() - tr.dx();
    mTotalTranslateY = delegated()->transform().dy() - tr.dy();
}

void GraphicsDelegateFrame::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // 整个拖动过程合并为一次撤销步骤
    mDelegate->startUndoStep();

    mStartingPoint = event->scenePos();

    initializeTransform();

    // 本次拖动的增量从零开始
    mScaleX           = 1;
    mScaleY           = 1;
    mTranslateX       = 0;
    mTranslateY       = 0;
    mAngleOffset      = 0;

    mInitialTransform = buildTransform();
    mOriginalSize     = delegated()->boundingRect().size();

    mCurrentTool      = toolFromPos(event->pos());
    setCursorFromAngle(QString::number((int)mAngle % 360));
    event->accept();
}

void GraphicsDelegateFrame::setCursorFromAngle(QString angle)
{
    if (mCurrentTool == Rotate)
    {
        QWidget *controlViewport = Application::boardController->controlView()->viewport();

        QSize cursorSize(45, 30);

        // 旋转光标在运行时绘制：白底黑框的圆角矩形中显示当前角度
        QImage mask_img(cursorSize, QImage::Format_Mono);
        mask_img.fill(0xff);
        QPainter mask_ptr(&mask_img);
        mask_ptr.setBrush(QBrush(QColor(0, 0, 0)));
        mask_ptr.drawRoundedRect(0, 0, cursorSize.width() - 1, cursorSize.height() - 1, 6, 6);
        QBitmap bmpMask = QBitmap::fromImage(mask_img);

        QPixmap pixCursor(cursorSize);
        pixCursor.fill(QColor(Qt::white));

        QPainter painter(&pixCursor);

        painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        painter.setBrush(QBrush(Qt::white));
        painter.setPen(QPen(QColor(Qt::black)));
        painter.drawRoundedRect(1, 1, cursorSize.width() - 2, cursorSize.height() - 2, 6, 6);
        painter.setFont(QFont("Arial", 10));
        painter.drawText(1, 1, cursorSize.width(), cursorSize.height(), Qt::AlignCenter, angle.append(QChar(176)));
        painter.end();

        // 用位图做遮罩裁出圆角外形，再把光标应用到控制视图
        pixCursor.setMask(bmpMask);
        controlViewport->setCursor(QCursor(pixCursor));
    }
}

bool GraphicsDelegateFrame::canResizeBottomRight(qreal width, qreal height, qreal scaleFactor)
{
    bool res = false;

    // 任何一次翻转都会让宽或高变为负值，因此按四个象限分别判定缩放后是否仍大于边框本身
    if (!mMirrorX && !mMirrorX && ((width * scaleFactor) > 2 * mFrameWidth && (height * scaleFactor) > 2 * mFrameWidth))
    {
        res = true;
    }
    else if (mMirrorX && !mMirrorY && (-width * scaleFactor) > 2 * mFrameWidth && (height * scaleFactor) > 2 * mFrameWidth)
    {
        res = true;
    }
    else if (!mMirrorX && mMirrorY && (width * scaleFactor) > 2 * mFrameWidth && (-height * scaleFactor) > 2 * mFrameWidth)
    {
        res = true;
    }
    else if (mMirrorX && mMirrorY && (-width * scaleFactor) > 2 * mFrameWidth && (-height * scaleFactor) > 2 * mFrameWidth)
    {
        res = true;
    }

    return res;
}

QPointF GraphicsDelegateFrame::getFixedPointFromPos()
{
    // 缩放时保持不动的锚点：拖上边固定下边、拖左边固定右边，翻转与旋转会改变具体取哪个角
    QPointF fixedPoint;
    if (!moving() && !rotating())
    {
        if (resizingTop())
        {
            if (mMirrorX && mMirrorY)
            {
                if ((0 < mAngle) && (mAngle < 90))
                {
                    fixedPoint = delegated()->sceneBoundingRect().topLeft();
                }
                else
                {
                    fixedPoint = delegated()->sceneBoundingRect().topRight();
                }
            }
            else
            {
                if ((0 < mAngle) && (mAngle <= 90))
                {
                    fixedPoint = delegated()->sceneBoundingRect().bottomRight();
                }
                else
                {
                    fixedPoint = delegated()->sceneBoundingRect().bottomLeft();
                }
            }
        }
        else if (resizingLeft())
        {
            if (mMirrorX && mMirrorY)
            {
                if ((0 < mAngle) && (mAngle < 90))
                {
                    fixedPoint = delegated()->sceneBoundingRect().bottomLeft();
                }
                else
                {
                    fixedPoint = delegated()->sceneBoundingRect().topLeft();
                }
            }
            else
            {
                if ((0 < mAngle) && (mAngle <= 90))
                {
                    fixedPoint = delegated()->sceneBoundingRect().topRight();
                }
                else
                {
                    fixedPoint = delegated()->sceneBoundingRect().bottomRight();
                }
            }
        }
    }
    return fixedPoint;
}

QSizeF GraphicsDelegateFrame::getResizeVector(qreal moveX, qreal moveY)
{
    qreal dPosX = 0;
    qreal dPosY = 0;

    // 翻转后鼠标移动方向与尺寸变化方向相反，需要取反
    if (resizingTop())
    {
        if (mMirrorX && mMirrorY)
        {
            dPosY = moveY;
        }
        else
        {
            dPosY = -moveY;
        }
    }
    else if (resizingLeft())
    {
        if (mMirrorX && mMirrorY)
        {
            dPosX = moveX;
        }
        else
        {
            dPosX = -moveX;
        }
    }

    else if (resizingRight())
    {
        dPosX = (mMirrorX) ? -moveX : moveX;
    }
    else if (resizingBottom())
    {
        dPosY = mMirrorY ? -moveY : moveY;
    }

    return QSizeF(dPosX, dPosY);
}

QSizeF GraphicsDelegateFrame::resizeDelegate(qreal moveX, qreal moveY)
{
    QSizeF incVector;
    mFixedPoint                          = getFixedPointFromPos();

    ResizableGraphicsItem *resizableItem = dynamic_cast<ResizableGraphicsItem *>(delegated());
    if (resizableItem)
    {
        incVector = getResizeVector(moveX, moveY);
        resizableItem->resize(mOriginalSize + incVector);

        // 拖上边/左边时尺寸变化以左上角为原点，需反向平移保持锚点不动
        if (resizingTop() || resizingLeft() || ((mMirrorX || mMirrorY) && resizingBottomRight()))
        {
            QPointF pos1 = getFixedPointFromPos();
            delegated()->setPos(delegated()->pos() - pos1 + mFixedPoint);
        }
    }

    return incVector;
}

void GraphicsDelegateFrame::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (None == mCurrentTool)
    {
        return;
    }

    QLineF move   = QLineF(mStartingPoint, event->scenePos());
    qreal  moveX  = (event->pos() - mStartingPoint).x();
    qreal  moveY  = (event->pos() - mStartingPoint).y();
    qreal  width  = delegated()->boundingRect().width() * mTotalScaleX;
    qreal  height = delegated()->boundingRect().height() * mTotalScaleY;

    if (mOperationMode == Scaling)
    {
        if (!rotating())
        {
            mTranslateX = moveX;
            // 执行缩放
            if (resizingBottomRight())
            {
                // 右下角拖动必须保持宽高比：用两个方向的平均缩放系数
                qreal scaleX;
                qreal scaleY;

                if (!mMirrorX)
                {
                    scaleX = (width + moveX) / width;
                }
                else
                {
                    scaleX = (width - moveX) / width;
                }

                if (!mMirrorY)
                {
                    scaleY = (height + moveY) / height;
                }
                else
                {
                    scaleY = (height - moveY) / height;
                }

                qreal scaleFactor = (scaleX + scaleY) / 2;

                // 不允许把内容缩到比边框还小，否则手把会互相重叠
                if (canResizeBottomRight(width, height, scaleFactor))
                {
                    if (mRespectRatio)
                    {
                        mScaleX = scaleFactor;
                        mScaleY = scaleFactor;
                    }
                    else
                    {
                        mScaleX = scaleX;
                        mScaleY = scaleY;
                    }
                }
            }
            else if (resizingLeft() || resizingRight())
            {
                if (width != 0)
                {
                    qreal scaleX = 0.0;
                    if (resizingLeft())
                    {
                        scaleX = (width - moveX) / width;
                    }
                    else if (resizingRight())
                    {
                        scaleX = (width + moveX) / width;
                    }
                    // 允许翻转的项在拖过对边时进入水平镜像，否则限制最小尺寸
                    if (mDelegate->testFrameFlags(GF_FLIPPABLE_ALL_AXIS) && qAbs(scaleX) != 0)
                    {
                        if ((qAbs(width * scaleX)) < 2 * mFrameWidth)
                        {
                            bool negative = (scaleX < 0) ? true : false;
                            if (negative)
                            {
                                if (mMirrorX)
                                {
                                    scaleX = 2 * mFrameWidth / width;
                                }
                                else
                                {
                                    scaleX = -2 * mFrameWidth / width;
                                }
                            }
                            else
                            {
                                scaleX    = -1;
                                mFlippedX = !mFlippedX;
                            }
                        }
                        mScaleX = scaleX;
                    }
                    else if (scaleX > 1 || (width * scaleX) > 2 * mFrameWidth)
                    {
                        mScaleX = scaleX;
                        if (resizingLeft())
                        {
                            mTranslateX = moveX;
                        }
                    }
                }
            }
            else if (resizingTop() || resizingBottom())
            {
                if (height != 0)
                {
                    qreal scaleY = 0.0;
                    if (resizingTop())
                    {
                        scaleY = (height - moveY) / height;
                    }
                    else if (resizingBottom())
                    {
                        scaleY = (height + moveY) / height;
                    }

                    if (mDelegate->testFrameFlags(GF_FLIPPABLE_ALL_AXIS) && qAbs(scaleY) != 0)
                    {
                        if ((qAbs(height * scaleY)) < 2 * mFrameWidth)
                        {
                            bool negative = (scaleY < 0) ? true : false;
                            if (negative)
                            {
                                if (mMirrorY)
                                {
                                    scaleY = 2 * mFrameWidth / width;
                                }
                                else
                                {
                                    scaleY = -2 * mFrameWidth / width;
                                }
                            }
                            else
                            {
                                scaleY    = -1;
                                mFlippedY = !mFlippedY;
                            }
                        }
                        mScaleY = scaleY;
                    }
                    else if (scaleY > 1 || (height * scaleY) > 2 * mFrameWidth)
                    {
                        mScaleY = scaleY;
                        if (resizingTop())
                        {
                            mTranslateY = moveY;
                        }
                    }
                }
            }
        }
    }

    if (rotating())
    {
        mTranslateX = 0;
        mTranslateY = 0;

        // 以框架中心为轴，把上一帧到当前帧的夹角累加到总角度
        QLineF startLine(sceneBoundingRect().center(), event->lastScenePos());
        QLineF currentLine(sceneBoundingRect().center(), event->scenePos());
        mAngle += startLine.angleTo(currentLine);

        // 角度接近 45° 或 30° 的整数倍时吸附，并把超出部分留到下一次拖动继续累加
        if ((int)mAngle % 45 >= 45 - mAngleTolerance || (int)mAngle % 45 <= mAngleTolerance)
        {
            mAngle        = qRound(mAngle / 45) * 45;
            mAngleOffset += startLine.angleTo(currentLine);
            if ((int)mAngleOffset % 360 > mAngleTolerance && (int)mAngleOffset % 360 < 360 - mAngleTolerance)
            {
                mAngle       += mAngleOffset;
                mAngleOffset  = 0;
            }
        }
        else if ((int)mAngle % 30 >= 30 - mAngleTolerance || (int)mAngle % 30 <= mAngleTolerance)
        {
            mAngle        = qRound(mAngle / 30) * 30;
            mAngleOffset += startLine.angleTo(currentLine);
            if ((int)mAngleOffset % 360 > mAngleTolerance && (int)mAngleOffset % 360 < 360 - mAngleTolerance)
            {
                mAngle       += mAngleOffset;
                mAngleOffset  = 0;
            }
        }

        setCursorFromAngle(QString::number((int)mAngle % 360));
    }
    else if (moving())
    {
        mTranslateX = move.dx();
        mTranslateY = move.dy();
        moveLinkedItems(move);
    }

    if (mOperationMode == Scaling || moving() || rotating())
    {
        QTransform tr = buildTransform();

        if (resizingRight() || resizingBottom() || resizingBottomRight())
        {
            // 缩放会以中心为基准改变包围盒，这里比较缩放前后角点位置，
            // 把差值补进平移量，使拖动的那条边/角保持贴合鼠标
            if (resizingBottomRight() && (mMirrorX || mMirrorY))
            {
                if (mFlippedX && !mMirrorX && mFlippedY)
                {
                    mTranslateX +=
                        mInitialTransform.map(delegated()->boundingRect().bottomLeft()).x() - tr.map(delegated()->boundingRect().bottomLeft()).x();
                    mTranslateY +=
                        mInitialTransform.map(delegated()->boundingRect().bottomLeft()).y() - tr.map(delegated()->boundingRect().bottomLeft()).y();
                }
                else if ((mFlippedX || mMirrorX) && (mFlippedY || mMirrorY))
                {
                    mTranslateX +=
                        mInitialTransform.map(delegated()->boundingRect().bottomRight()).x() - tr.map(delegated()->boundingRect().bottomRight()).x();
                    mTranslateY +=
                        mInitialTransform.map(delegated()->boundingRect().bottomRight()).y() - tr.map(delegated()->boundingRect().bottomRight()).y();
                }
                else if (mFlippedX || mMirrorX)
                {
                    mTranslateX +=
                        mInitialTransform.map(delegated()->boundingRect().topRight()).x() - tr.map(delegated()->boundingRect().topRight()).x();
                    mTranslateY +=
                        mInitialTransform.map(delegated()->boundingRect().topRight()).y() - tr.map(delegated()->boundingRect().topRight()).y();
                }
                else if (mFlippedY || mMirrorY)
                {
                    mTranslateX +=
                        mInitialTransform.map(delegated()->boundingRect().bottomLeft()).x() - tr.map(delegated()->boundingRect().bottomLeft()).x();
                    mTranslateY +=
                        mInitialTransform.map(delegated()->boundingRect().bottomLeft()).y() - tr.map(delegated()->boundingRect().bottomLeft()).y();
                }
                else
                {
                    mTranslateX +=
                        mInitialTransform.map(delegated()->boundingRect().bottomRight()).x() - tr.map(delegated()->boundingRect().bottomRight()).x();
                    mTranslateY +=
                        mInitialTransform.map(delegated()->boundingRect().bottomRight()).y() - tr.map(delegated()->boundingRect().bottomRight()).y();
                }
            }
            else
            {
                mTranslateX += mInitialTransform.map(delegated()->boundingRect().topLeft()).x() - tr.map(delegated()->boundingRect().topLeft()).x();
                mTranslateY += mInitialTransform.map(delegated()->boundingRect().topLeft()).y() - tr.map(delegated()->boundingRect().topLeft()).y();
            }
        }
        else if (resizingTop() || resizingLeft())
        {
            // 拖上边/左边时锚点在右下角，用右下角的位置偏差补偿平移
            QPointF bottomRight  = tr.map(delegated()->boundingRect().bottomRight());
            QPointF fixedPoint   = mInitialTransform.map(delegated()->boundingRect().bottomRight());
            mTranslateX         += fixedPoint.x() - bottomRight.x();
            mTranslateY         += fixedPoint.y() - bottomRight.y();
        }
        delegated()->setTransform(buildTransform());
    }
    else // 调整尺寸/水平调整尺寸模式
    {
        if (resizingBottomRight())
        {
            static QSizeF incV = QSizeF();
            static QSizeF incH = QSizeF();

            // 角点缩放拆成两次单向缩放：先竖直方向，再水平方向
            if (mMirrorX && mMirrorY)
            {
                mCurrentTool = ResizeTop;
            }
            else
            {
                mCurrentTool = ResizeBottom;
            }

            incV           = resizeDelegate(moveX, moveY);
            mOriginalSize += incV;

            if (mMirrorX && mMirrorY)
            {
                mCurrentTool = ResizeLeft;
            }
            else
            {
                mCurrentTool = ResizeRight;
            }

            // 把场景坐标下的位移旋转回内容自身的坐标系，才能正确算出水平/竖直分量
            move           = QLineF(event->lastScenePos(), event->scenePos());
            moveX          = move.length() * cos((move.angle() - mAngle) * PI / 180);
            moveY          = -move.length() * sin((move.angle() - mAngle) * PI / 180);

            mFixedPoint    = getFixedPointFromPos();

            incH           = resizeDelegate(moveX, moveY);

            mOriginalSize -= incV;
            mOriginalSize += incH;

            mCurrentTool   = ResizeBottomRight;
        }
        else
        {
            resizeDelegate(moveX, moveY);
        }
    }
    event->accept();
}

QList<GraphicsDelegateFrame *> GraphicsDelegateFrame::getLinkedFrames()
{
    // 与当前项一起被选中的其他项视为联动对象，拖动时保持相对位置
    QList<GraphicsDelegateFrame *> linkedFrames;
    QList<QGraphicsItem *>         sItems = mDelegate->delegated()->scene()->selectedItems();
    if (sItems.count())
    {
        sItems.removeAll(delegated());

        foreach (QGraphicsItem *item, sItems)
        {
            GraphicsItem *gitem = dynamic_cast<GraphicsItem *>(item);
            if (gitem)
            {
                linkedFrames << gitem->Delegate()->frame();
            }
        }
    }
    return linkedFrames;
}

void GraphicsDelegateFrame::prepareFramesToMove(QList<GraphicsDelegateFrame *> framesToMove)
{
    mLinkedFrames = framesToMove;
    // 每个联动框架各自记录起始变换，之后用相同的位移分别重建
    foreach (GraphicsDelegateFrame *frame, mLinkedFrames)
    {
        frame->prepareLinkedFrameToMove();
    }
}

void GraphicsDelegateFrame::prepareLinkedFrameToMove()
{
    mDelegate->startUndoStep();

    mStartingPoint = QPointF(0, 0);

    initializeTransform();

    // 联动移动只需要平移，缩放与旋转增量归零
    mScaleX           = 1;
    mScaleY           = 1;
    mTranslateX       = 0;
    mTranslateY       = 0;
    mAngleOffset      = 0;

    mInitialTransform = buildTransform();

    mCurrentTool      = Move;
}

void GraphicsDelegateFrame::moveLinkedItems(QLineF movingVector, bool bLinked)
{
    if (bLinked)
    {
        mCurrentTool = Move;

        mTranslateX  = movingVector.dx();
        mTranslateY  = movingVector.dy();

        // 联动项直接写回变换，不进入自身的撤销栈
        delegated()->setTransform(buildTransform(), false);
    }
    else
    {
        foreach (GraphicsDelegateFrame *frame, mLinkedFrames)
        {
            frame->moveLinkedItems(movingVector, true);
        }
    }
}

QTransform GraphicsDelegateFrame::buildTransform()
{
    // 变换顺序：先平移，再绕中心旋转，最后缩放；绕中心的旋转用前后两次相反平移实现
    QTransform tr;
    QPointF    center = delegated()->boundingRect().center();

    // 平移
    tr.translate(mTotalTranslateX + mTranslateX, mTotalTranslateY + mTranslateY);

    // 设置角度
    tr.translate(center.x() * mTotalScaleX * mScaleX, center.y() * mTotalScaleY * mScaleY);
    tr.rotate(-mAngle);
    tr.translate(-center.x() * mTotalScaleX * mScaleX, -center.y() * mTotalScaleY * mScaleY);

    // 缩放
    tr.scale(mTotalScaleX * mScaleX, mTotalScaleY * mScaleY);
    return tr;
}

void GraphicsDelegateFrame::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    updateResizeCursors();

    mDelegate->commitUndoStep();
    // 把本次增量并入累计值，下一次拖动从新状态继续
    mTotalScaleX     *= mScaleX;
    mTotalScaleY     *= mScaleY;
    mTotalTranslateX += mTranslateX;
    mTotalTranslateY += mTranslateY;
    event->accept();

    mCurrentTool = None;

    QGraphicsRectItem::mouseReleaseEvent(event);

    // 复位缩放状态，让按钮恢复显示
    if (isResizing())
    {
        mResizing = false;
    }
}

void GraphicsDelegateFrame::updateResizeCursors()
{
    // 手把光标随内容角度旋转，让箭头方向与拖动方向一致
    QPixmap    pix(":/images/cursors/resize.png");
    QTransform tr;

    tr.rotate(-mAngle);
    QCursor resizeCursor = QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2, pix.height() / 2);
    mLeftResizeGrip->setCursor(resizeCursor);
    mRightResizeGrip->setCursor(resizeCursor);

    tr.rotate(-90);
    resizeCursor = QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2, pix.height() / 2);
    mBottomResizeGrip->setCursor(resizeCursor);
    mTopResizeGrip->setCursor(resizeCursor);

    tr.rotate(-45);
    resizeCursor = QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2, pix.height() / 2);
    mBottomRightResizeGrip->setCursor(resizeCursor);
}

void GraphicsDelegateFrame::setVisible(bool visible)
{
    mVisible = visible;
    // 不可见时用无画刷代替隐藏，保留框架的几何与命中信息
    if (mVisible)
    {
        setBrush(QBrush(Settings::paletteColor));
    }
    else
    {
        setBrush(Qt::NoBrush);
    }
}

void GraphicsDelegateFrame::positionHandles()
{
    QRectF itemRect = delegated()->boundingRect();

    // 媒体工具栏弹出时，框架顶部要额外让出工具栏高度，避免遮挡
    if (mDelegate->getToolBarItem() && mDelegate->getToolBarItem()->isVisibleOnBoard() && mDelegate->getToolBarItem()->isShifting())
    {
        QPointF graphicsItemPosition = itemRect.topLeft();
        itemRect.setTopLeft(graphicsItemPosition - QPointF(0, mDelegate->getToolBarItem()->boundingRect().height() * mDelegate->antiScaleRatio()));
    }

    QTransform itemTransform = delegated()->sceneTransform();
    QPointF    topLeft       = itemTransform.map(itemRect.topLeft());
    QPointF    topRight      = itemTransform.map(itemRect.topRight());
    QPointF    bottomLeft    = itemTransform.map(itemRect.bottomLeft());
    QPointF    bottomRight   = itemTransform.map(itemRect.bottomRight());
    QPointF    center        = itemTransform.map(itemRect.center());
    int        rotateHeight  = QLineF(topLeft, bottomLeft).length();

    // 翻转后角点的左右/上下顺序会颠倒，先规整成左上、右上、左下、右下
    if (topLeft.x() > topRight.x())
    {
        QPointF topTmp    = topRight;
        QPointF bottomTmp = bottomRight;
        topRight          = topLeft;
        topLeft           = topTmp;
        bottomRight       = bottomLeft;
        bottomLeft        = bottomTmp;
    }

    if (bottomLeft.y() > topLeft.y())
    {
        QPointF leftTmp  = bottomLeft;
        QPointF rightTmp = bottomRight;
        bottomLeft       = topLeft;
        topLeft          = leftTmp;
        bottomRight      = topRight;
        topRight         = rightTmp;
    }

    QLineF topLine(topLeft, topRight);
    qreal  angle = topLine.angle();
    qreal  width = topLine.length();

    QLineF leftLine(topLeft, bottomLeft);
    qreal  height = leftLine.length();

    int h         = rotating() ? rotateHeight : height;

    // 可见框架四周各留出一个边框宽度；不可见时只保留内容尺寸
    if (mVisible)
    {
        setRect(center.x() - mFrameWidth - width / 2, center.y() - mFrameWidth - mTitleBarHeight - h / 2, width + 2 * mFrameWidth,
            h + (2 * mFrameWidth) + mTitleBarHeight);
    }
    else
    {
        setRect(center.x() - width / 2, center.y() - h / 2, width, h);
    }

    // 通过三次变换把框架绕自身中心旋转到与内容一致的角度
    resetTransform();
    setTransform(QTransform::fromTranslate(center.x(), center.y()), true);
    setTransform(QTransform().rotate(-angle), true);
    setTransform(QTransform::fromTranslate(-center.x(), -center.y()), true);
    // TODO: 三个变换可合并为一次，减少重复计算

    // 重新挂载把手，确保它们位于框架的子项列表中且坐标系一致
    mBottomRightResizeGripSvgItem->setParentItem(this);
    mBottomResizeGripSvgItem->setParentItem(this);
    mLeftResizeGripSvgItem->setParentItem(this);
    mRightResizeGripSvgItem->setParentItem(this);
    mTopResizeGripSvgItem->setParentItem(this);
    mRotateButton->setParentItem(this);

    mBottomRightResizeGrip->setParentItem(this);
    mBottomResizeGrip->setParentItem(this);
    mLeftResizeGrip->setParentItem(this);
    mRightResizeGrip->setParentItem(this);
    mTopResizeGrip->setParentItem(this);

    QRectF brRect = mBottomRightResizeGripSvgItem->mapRectToParent(mBottomRightResizeGripSvgItem->boundingRect());
    QRectF bRect  = mBottomResizeGripSvgItem->mapRectToParent(mBottomResizeGripSvgItem->boundingRect());
    QRectF lRect  = mLeftResizeGripSvgItem->mapRectToParent(mLeftResizeGripSvgItem->boundingRect());
    QRectF rRect  = mRightResizeGripSvgItem->mapRectToParent(mRightResizeGripSvgItem->boundingRect());
    QRectF trRect = mTopResizeGripSvgItem->mapRectToParent(mTopResizeGripSvgItem->boundingRect());

    // 图标按自身尺寸贴到对应的边或角上
    mBottomRightResizeGripSvgItem->setPos(rect().right() - brRect.width(), rect().bottom() - brRect.height());
    mBottomResizeGripSvgItem->setPos(rect().center().x() - bRect.width() / 2, rect().bottom() - bRect.height());

    mLeftResizeGripSvgItem->setPos(rect().left(), rect().center().y() - lRect.height() / 2);
    mRightResizeGripSvgItem->setPos(rect().right() - rRect.width(), rect().center().y() - rRect.height() / 2);

    mTopResizeGripSvgItem->setPos(rect().center().x() - trRect.width() / 2, rect().y());
    mRotateButton->setPos(rect().right() - mFrameWidth - 5, rect().top() + 5);

    mBottomRightResizeGrip->setRect(bottomRightResizeGripRect());
    mBottomResizeGrip->setRect(bottomResizeGripRect());
    mLeftResizeGrip->setRect(leftResizeGripRect());
    mRightResizeGrip->setRect(rightResizeGripRect());
    mTopResizeGrip->setRect(topResizeGripRect());

    QVariant vLocked                 = delegated()->data(GraphicsItemData::ItemLocked);
    bool     isLocked                = (vLocked.isValid() && vLocked.toBool());
    bool     bShowHorizontalResizers = ResizingHorizontally == mOperationMode;
    bool     bShowVerticalResizers   = ResizingHorizontally != mOperationMode;
    bool     bShowAllResizers        = Resizing == mOperationMode || Scaling == mOperationMode;

    // 锁定的项不显示任何把手；仅水平调整时只露出左右把手
    mBottomRightResizeGripSvgItem->setVisible(!isLocked && bShowAllResizers);
    mBottomResizeGripSvgItem->setVisible(!isLocked && (bShowVerticalResizers || bShowAllResizers));
    mLeftResizeGripSvgItem->setVisible(!isLocked && (bShowHorizontalResizers || bShowAllResizers));
    mRightResizeGripSvgItem->setVisible(!isLocked && (bShowHorizontalResizers || bShowAllResizers));
    mTopResizeGripSvgItem->setVisible(!isLocked && (bShowVerticalResizers || bShowAllResizers));
    mRotateButton->setVisible(mDelegate->testFrameFlags(GF_REVOLVABLE) && !isLocked);

    mBottomRightResizeGrip->setVisible(!isLocked && bShowAllResizers);
    mBottomResizeGrip->setVisible(!isLocked && (bShowVerticalResizers || bShowAllResizers));
    mLeftResizeGrip->setVisible(!isLocked && (bShowHorizontalResizers || bShowAllResizers));
    mRightResizeGrip->setVisible(!isLocked && (bShowHorizontalResizers || bShowAllResizers));
    mTopResizeGrip->setVisible(!isLocked && (bShowVerticalResizers || bShowAllResizers));

    // 锁定状态用半透明底色提示
    if (isLocked)
    {
        QColor baseColor = Settings::paletteColor;
        baseColor.setAlphaF(baseColor.alphaF() / 3);
        setBrush(QBrush(baseColor));
    }
    else
    {
        setBrush(QBrush(Settings::paletteColor));
    }

    // 与内容同 z 值，使框架在场景中的叠放顺序和内容保持一致
    setZValue(delegated()->zValue());
}

QGraphicsItem *GraphicsDelegateFrame::delegated()
{
    return mDelegate->delegated();
}

GraphicsDelegateFrame::FrameTool GraphicsDelegateFrame::toolFromPos(QPointF pos)
{
    // 命中判定顺序与把手的优先级一致：锁定项不响应，水平调整模式忽略上下边
    if (mDelegate->isLocked())
    {
        return None;
    }
    else if (bottomRightResizeGripRect().contains(pos) && ResizingHorizontally != mOperationMode)
    {
        return ResizeBottomRight;
    }
    else if (bottomResizeGripRect().contains(pos) && ResizingHorizontally != mOperationMode)
    {
        // 竖直翻转后视觉上的“下边”实际是内容的上边
        if (mMirrorY)
        {
            return ResizeTop;
        }
        else
        {
            return ResizeBottom;
        }
    }
    else if (leftResizeGripRect().contains(pos))
    {
        if (mMirrorX)
        {
            return ResizeRight;
        }
        else
        {
            return ResizeLeft;
        }
    }
    else if (rightResizeGripRect().contains(pos))
    {
        if (mMirrorX)
        {
            return ResizeLeft;
        }
        else
        {
            return ResizeRight;
        }
    }
    else if (topResizeGripRect().contains(pos) && ResizingHorizontally != mOperationMode)
    {
        if (mMirrorY)
        {
            return ResizeBottom;
        }
        else
        {
            return ResizeTop;
        }
    }
    else if (rotateButtonBounds().contains(pos) && mDelegate && mDelegate->testFrameFlags(GF_REVOLVABLE))
    {
        return Rotate;
    }
    else
    {
        return Move;
    }
}

QRectF GraphicsDelegateFrame::bottomRightResizeGripRect() const
{
    // 各把手命中区都是边长为 mFrameWidth 的正方形，紧贴矩形外侧
    return QRectF(rect().right() - mFrameWidth, rect().bottom() - mFrameWidth, mFrameWidth, mFrameWidth);
}

QRectF GraphicsDelegateFrame::bottomResizeGripRect() const
{
    return QRectF(rect().center().x() - mFrameWidth / 2, rect().bottom() - mFrameWidth, mFrameWidth, mFrameWidth);
}

QRectF GraphicsDelegateFrame::leftResizeGripRect() const
{
    return QRectF(rect().left(), rect().center().y() - mFrameWidth / 2, mFrameWidth, mFrameWidth);
}

QRectF GraphicsDelegateFrame::rightResizeGripRect() const
{
    return QRectF(rect().right() - mFrameWidth, rect().center().y() - mFrameWidth / 2, mFrameWidth, mFrameWidth);
}

QRectF GraphicsDelegateFrame::topResizeGripRect() const
{
    return QRectF(rect().center().x() - mFrameWidth / 2, rect().top(), mFrameWidth, mFrameWidth);
}

QRectF GraphicsDelegateFrame::rotateButtonBounds() const
{
    return QRectF(rect().right() - mFrameWidth, rect().top(), mFrameWidth, mFrameWidth);
}

void GraphicsDelegateFrame::refreshGeometry()
{
    // 只取宽高不考虑旋转，用于快速刷新框架尺寸
    QRectF     itemRect      = delegated()->boundingRect();
    QTransform itemTransform = delegated()->sceneTransform();
    QPointF    topLeft       = itemTransform.map(itemRect.topLeft());
    QPointF    topRight      = itemTransform.map(itemRect.topRight());
    QPointF    bottomLeft    = itemTransform.map(itemRect.bottomLeft());

    QLineF topLine(topLeft, topRight);
    qreal  width = topLine.length();
    QLineF leftLine(topLeft, bottomLeft);
    qreal  height = leftLine.length();
    setRect(topRight.x() - mFrameWidth, topLeft.y() - mFrameWidth, width + 2 * mFrameWidth, height + 2 * mFrameWidth);
}
