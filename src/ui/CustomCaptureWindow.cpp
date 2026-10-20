#include <QApplication>
#include "core/VirtualDesktop.h"
#include <QStyle>

#include "ui/CustomCaptureWindow.h"

#include "platform/PlatformUtils.h"
#include "ui/RubberBand.h"

struct CustomCaptureWindow::Private
{
    QPixmap      mWholeScreenPixmap;
    QRubberBand *mSelectionBand{nullptr};
    QStyle      *mRubberBandStyle{nullptr};
    QPoint       mOrigin{0, 0};
    bool         mIsSelecting{false};
};

CustomCaptureWindow::CustomCaptureWindow(QWidget *parent) : QDialog(parent, Qt::FramelessWindowHint | Qt::Window), d(new Private)
{
    setCursor(Qt::CrossCursor);
    // 先保持透明，待 execute() 定位并 show() 之后再恢复不透明，避免窗口闪现。
    setWindowOpacity(0.0);
}

CustomCaptureWindow::~CustomCaptureWindow()
{
    delete d->mSelectionBand;
    delete d->mRubberBandStyle;
}

QPixmap CustomCaptureWindow::getSelectedPixmap()
{
    if (d->mSelectionBand)
    {
        return d->mWholeScreenPixmap.copy(d->mSelectionBand->geometry());
    }
    else
    {
        return QPixmap();
    }
}

int CustomCaptureWindow::execute(const QPixmap &pScreenPixmap)
{
    d->mWholeScreenPixmap         = pScreenPixmap;

    // 覆盖鼠标所在的那块屏幕，保证多屏环境下截图与坐标一致。
    VirtualDesktop *desktop       = VirtualDesktop::instance();
    int             currentScreen = desktop->screenNumber(QCursor::pos());
    setGeometry(desktop->screenGeometry(currentScreen));
    this->show();
    setWindowOpacity(1.0);

    return exec();
}

void CustomCaptureWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (d->mIsSelecting)
    {
        d->mSelectionBand->setGeometry(QRect(d->mOrigin, event->pos()).normalized());
    }

    event->accept();
}

void CustomCaptureWindow::mousePressEvent(QMouseEvent *event)
{
    if (!d->mIsSelecting)
    {
        d->mIsSelecting = true;
        d->mOrigin      = event->pos();

        if (!d->mSelectionBand)
        {
            d->mSelectionBand = new RubberBand(QRubberBand::Rectangle, this);
        }

        d->mSelectionBand->setGeometry(QRect(d->mOrigin, QSize()));
        d->mSelectionBand->show();
        event->accept();
    }
}

void CustomCaptureWindow::mouseReleaseEvent(QMouseEvent *event)
{
    d->mIsSelecting = false;

    if (d->mSelectionBand)
    {
        d->mSelectionBand->hide();

        event->accept();

        // 太小的选择多半是误触，不当作有效截图。
        if (!(d->mSelectionBand->geometry().width() < 6 && d->mSelectionBand->geometry().height() < 6))
        {
            accept();
        }
    }
}

void CustomCaptureWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        d->mIsSelecting = false;

        if (d->mSelectionBand)
        {
            d->mSelectionBand->hide();
        }

        event->accept();
        reject();
    }
}

void CustomCaptureWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
}

void CustomCaptureWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.drawPixmap(0, 0, d->mWholeScreenPixmap);
}
