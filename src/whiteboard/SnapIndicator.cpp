#include "whiteboard/SnapIndicator.h"

#include <QGraphicsView>
#include <QPaintEvent>
#include <QPainter>
#include <QPropertyAnimation>

struct SnapIndicator::Private
{
    qreal               mAngle{0};                ///< 标记的旋转角（ painters 顺时针为正）
    int                 mAlpha{0};                ///< 当前不透明度，由动画驱动
    QColor              mColor{0x62, 0xa7, 0xe0}; ///< 默认浅蓝
    QPropertyAnimation *mAnimation{nullptr};      ///< 淡出动画
};

SnapIndicator::SnapIndicator(QWidget *parent) : QLabel(parent), d(new Private)
{
    setFixedSize(120, 120);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    d->mAnimation = new QPropertyAnimation(this, "alpha", this);
    d->mAnimation->setStartValue(0xff);
    d->mAnimation->setEndValue(0);
    d->mAnimation->setDuration(1000);

    connect(d->mAnimation, &QPropertyAnimation::finished, this, &QWidget::hide);
}

SnapIndicator::~SnapIndicator()
{
    delete d;
}

void SnapIndicator::appear(Qt::Corner corner, const QPointF &snapPoint, qreal angle)
{
    d->mAnimation->stop();

    // QPainter 的旋转是顺时针，而图元边线的角度是逆时针，先取反。
    d->mAngle = -angle;
    switch (corner)
    {
        case Qt::TopLeftCorner:
            d->mAngle += 270;
            break;
        case Qt::BottomLeftCorner:
            d->mAngle += 180;
            break;
        case Qt::BottomRightCorner:
            d->mAngle += 90;
            break;
        default:
            break;
    }

    show();

    // 父控件必须是已经 setScene 的视图，否则场景坐标无处换算。
    if (auto *view = qobject_cast<QGraphicsView *>(parentWidget()); view && view->scene())
    {
        move(view->mapFromScene(snapPoint) - QPoint(width() / 2, height() / 2));
        d->mAnimation->start();
    }
}

int SnapIndicator::alpha() const
{
    return d->mAlpha;
}

void SnapIndicator::setAlpha(int opacity)
{
    d->mAlpha = opacity;
    update();
}

void SnapIndicator::setColor(const QColor &color)
{
    d->mColor = color;
}

void SnapIndicator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.translate(width() / 2, height() / 2);
    painter.rotate(d->mAngle);

    QColor penColor = d->mColor;
    penColor.setAlpha(d->mAlpha);

    QPen pen;
    pen.setColor(penColor);
    painter.setPen(pen);

    // 一个直角从中心指向左边与下边，看起来就像把那个角“卡”在网格交点上。
    QPolygon triangle;
    triangle << QPoint() << QPoint(-width() / 2, 0) << QPoint(0, height() / 2);

    painter.setBrush(penColor);
    painter.drawPolygon(triangle);
}
