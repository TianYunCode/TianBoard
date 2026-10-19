#include <QtWidgets>
#include <QPainterPath>

#include "ui/FloatingPalette.h"

#include "platform/PlatformUtils.h"

#include "core/Settings.h"


struct FloatingPalette::Private
{
    QBrush mBackgroundBrush;
    bool mbGrip{false};
    bool mCustomPosition{false};
    bool mIsMoving{false};
    QList<FloatingPalette*> mAssociatedPalette;
    QPoint mDragPosition;
    bool mCanBeMinimized{false};
    eMinimizedLocation mMinimizedLocation{eMinimizedLocation_None};
    Qt::Corner mDefaultPosition{Qt::TopLeftCorner};
};

FloatingPalette::FloatingPalette(Qt::Corner position, QWidget *parent)
    : QWidget(parent, parent ? Qt::Widget : Qt::Tool | (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint))
    , d(new Private)
{
    d->mDefaultPosition = position;

    setCursor(Qt::ArrowCursor);

    if (parent)
    {
        setAttribute(Qt::WA_NoMousePropagation);
    }
    else
    {
        // standalone window
        // !!!! Should be included into Windows after QT recompilation
#ifndef Q_OS_WIN
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_MacAlwaysShowToolWindow);
#endif
#ifdef Q_OS_OSX
        setAttribute(Qt::WA_MacAlwaysShowToolWindow);
        setAttribute(Qt::WA_MacNoShadow);
#endif
    }

    d->mBackgroundBrush = QBrush(Settings::paletteColor);
    d->mbGrip = true;
}

FloatingPalette::~FloatingPalette() = default;

void FloatingPalette::setGrip(bool newGrip)
{
    d->mbGrip = newGrip;
    update();
}


void FloatingPalette::setGripVisible(bool visible)
{
    d->mbGrip = visible;
}


void FloatingPalette::setBackgroundBrush(const QBrush& brush)
{
    if (d->mBackgroundBrush != brush)
    {
        d->mBackgroundBrush = brush;
        update();
    }
}


void FloatingPalette::setBackgroundAlpha(int alpha)
{
    QColor fadedColor = d->mBackgroundBrush.color();
    fadedColor.setAlpha(alpha);
    d->mBackgroundBrush = QBrush(fadedColor);
}


void FloatingPalette::setCustomPosition(bool pFlag)
{
    d->mCustomPosition = pFlag;

    if (pFlag)
        removeAllAssociatedPalette();

}


void FloatingPalette::setCustomPositionFlag(bool pFlag)
{
    d->mCustomPosition = pFlag;
}


bool FloatingPalette::isMoving() const
{
    return d->mIsMoving;
}


eMinimizedLocation FloatingPalette::minimizedLocation()
{
    return d->mMinimizedLocation;
}

int FloatingPalette::radius()
{
    return 10;
}


int FloatingPalette::border()
{
    return 0;
}


void FloatingPalette::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    emit mouseEntered();
}

void FloatingPalette::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        d->mIsMoving = true;
        d->mDragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
    else
    {
        QWidget::mousePressEvent(event);
    }
}

void FloatingPalette::mouseMoveEvent(QMouseEvent *event)
{
    if (d->mIsMoving)
    {
        moveInsideParent(event->globalPos() - d->mDragPosition);
        event->accept();
        emit moving();
    }
    else
    {
        QWidget::mouseMoveEvent(event);
    }
}

void FloatingPalette::mouseReleaseEvent(QMouseEvent *event)
{
    if (d->mIsMoving)
    {
        d->mIsMoving = false;
        setCustomPosition(true);
        event->accept();
    }
    else
    {
        QWidget::mouseReleaseEvent(event);
    }
}

int FloatingPalette::getParentRightOffset()
{
    return 0;
}

void FloatingPalette::moveInsideParent(const QPoint &position)
{
    QWidget *parent = parentWidget();

    if (parent)
    {
        int margin = Settings::boardMargin - border();
        qreal newX = qMax(margin, qMin(parent->width() - getParentRightOffset() - width() - margin, position.x()));
        qreal newY = qMax(margin, qMin(parent->height() - height() - margin, position.y()));

        if (!d->mCustomPosition && !d->mIsMoving)
        {
            if (d->mDefaultPosition == Qt::TopLeftCorner || d->mDefaultPosition == Qt::BottomLeftCorner)
            {
                newX = margin;
            }
            else
            {
                newX = qMax(margin, parent->width() - getParentRightOffset() - width() - margin);
            }
        }
        move(newX, newY);
        minimizePalette(QPoint(newX, newY));
    }
    else
    {
        move(position);
    }
}

void FloatingPalette::addAssociatedPalette(FloatingPalette* pOtherPalette)
{
    d->mAssociatedPalette.append(pOtherPalette);
}

void FloatingPalette::removeAssociatedPalette(FloatingPalette* pOtherPalette)
{
    d->mAssociatedPalette.removeOne(pOtherPalette);
}

QSize FloatingPalette::preferredSize()
{
    QSize palettePreferredSize = sizeHint();

    if (palettePreferredSize.width() == 0)
    {
        palettePreferredSize = childrenRect().size();
    }

    return palettePreferredSize;
}

void FloatingPalette::adjustSizeAndPosition(bool pUp)
{
    QSize newPreferredSize = preferredSize();

    foreach (FloatingPalette* palette, d->mAssociatedPalette)
    {
        QSize palettePreferredSize = palette->preferredSize();
        newPreferredSize.setWidth(newPreferredSize.expandedTo(palettePreferredSize).width());
    }
    QSize previousSize = size();
    int biggerHeight = preferredSize().height() - previousSize.height();
    if ((pUp && (biggerHeight > 0))
        || (!pUp && (biggerHeight < 0)))
    {
        move(pos().x(), pos().y() - biggerHeight);
    }

    if (newPreferredSize != size())
    {
        resize(newPreferredSize);
        moveInsideParent(pos());
        foreach(FloatingPalette* palette, d->mAssociatedPalette)
        {
            palette->move(pos().x(), palette->pos().y());
            palette->resize(newPreferredSize.width(), palette->size().height());
        }
    }
}

void FloatingPalette::removeAllAssociatedPalette()
{
    foreach (FloatingPalette* palette, d->mAssociatedPalette)
    {
        palette->removeAssociatedPalette(this);
        removeAssociatedPalette(palette);
    }
}

void FloatingPalette::showEvent(QShowEvent *)
{
    moveInsideParent(pos());
}

void FloatingPalette::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(d->mBackgroundBrush);

    if(d->mbGrip)
    {
        painter.setBrush(QBrush(QColor(170, 170 ,170)));
        QPainterPath borderPath;
        borderPath.addRoundedRect(0, 0, width(), height(), radius(), radius());
        borderPath.addRoundedRect(border(), border(), width() - 2 * border(), height() - 2 * border(), radius(), radius());
        painter.drawPath(borderPath);
        painter.setBrush(d->mBackgroundBrush);
        painter.drawRoundedRect(border(), border(), width() - 2 * border(), height() - 2 * border(), radius(), radius());
    }
    else
    {
        painter.drawRoundedRect(border(), border(), width() - 2 * border(), height() - 2 * border(), radius(), radius());
    }
}

int FloatingPalette::gripSize()
{
    return 5;
}

void FloatingPalette::minimizePalette(const QPoint& pos)
{
    if(!d->mCanBeMinimized)
    {
        //  If this floating palette cannot be minimized, we exit this method.
    return;
    }

    if(d->mMinimizedLocation == eMinimizedLocation_None)
    {
    //  Verify if we have to minimize this palette
    if(pos.x() == 5)
    {
        d->mMinimizedLocation = eMinimizedLocation_Left;
    }
//    else if(pos.y() == 5)
//    {
//        mMinimizedLocation = eMinimizedLocation_Top;
//    }
    else if(pos.x() == parentWidget()->width() - getParentRightOffset() - width() - 5)
    {
        d->mMinimizedLocation = eMinimizedLocation_Right;
    }
//    else if(pos.y() == parentSize.height() - height() - 5)
//    {
//        mMinimizedLocation = eMinimizedLocation_Bottom;
//    }

    //  Minimize the Palette
    if(d->mMinimizedLocation != eMinimizedLocation_None)
    {
        emit minimizeStart(d->mMinimizedLocation);
    }
    }
    else
    {
    //  Restore the palette
    if(pos.x() > 5 &&
       pos.y() > 5 &&
       pos.x() < parentWidget()->width() - getParentRightOffset()  - width() - 5 &&
       pos.y() < parentWidget()->size().height() - height() - 5)
    {
        d->mMinimizedLocation = eMinimizedLocation_None;
        emit maximizeStart();
    }
    }
}

void FloatingPalette::setMinimizePermission(bool permission)
{
    d->mCanBeMinimized = permission;
}
