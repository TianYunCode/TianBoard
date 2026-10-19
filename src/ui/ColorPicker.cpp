#include "ui/ColorPicker.h"

#include <QtWidgets>


struct ColorPicker::Private
{
    QList<QColor> mColors;
    int mSelectedColorIndex{0};
};

ColorPicker::ColorPicker(QWidget* parent)
    : QFrame(parent)
    , d(new Private)
{
    // NOOP
}

ColorPicker::ColorPicker(QWidget* parent, const QList<QColor>& colors, int pSelectedColorIndex)
    : QFrame(parent)
    , d(new Private)
{
    d->mColors = colors;
    d->mSelectedColorIndex = pSelectedColorIndex;
}


ColorPicker::~ColorPicker()
{
    // NOOP
}

QList<QColor> ColorPicker::getColors() const
{
    return d->mColors;
}

void ColorPicker::setColors(const QList<QColor>& pColors)
{
    d->mColors = pColors;
    repaint();
}

int ColorPicker::selectedColorIndex() const
{
    return d->mSelectedColorIndex;
}

void ColorPicker::setSelectedColorIndex(int pSelectedColorIndex)
{
    d->mSelectedColorIndex = pSelectedColorIndex;
    repaint();
}


void ColorPicker::paintEvent ( QPaintEvent * event )
{
    Q_UNUSED(event);

    QPainter painter(this);

    if (d->mSelectedColorIndex >= 0 && d->mSelectedColorIndex < d->mColors.size())
    {
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setBrush(d->mColors.at(d->mSelectedColorIndex));

        painter.drawRect(0, 0, width(), height());
    }

}

void ColorPicker::mousePressEvent ( QMouseEvent * event )
{
    if (event->buttons() & Qt::LeftButton)
    {
        if (d->mColors.isEmpty())
            return;

        d->mSelectedColorIndex++;

        if (d->mSelectedColorIndex >= d->mColors.size())
            d->mSelectedColorIndex = 0;

        repaint();

        emit colorSelected(d->mColors.at(d->mSelectedColorIndex));

    }
}


