#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QCheckBox>
#include <QtWebChannel/QWebChannel>
#include "ui/StartupHintsPalette.h"

#include "utils/Globals.h"
#include "core/Settings.h"

struct StartupHintsPalette::Private
{
    QCheckBox* mShowNextTime{nullptr};
    QVBoxLayout* mLayout{nullptr};
    QHBoxLayout* mButtonLayout{nullptr};
    WebWidgetBoardAPI *boardApi{nullptr};
	QWebEngineView* mpWebView{nullptr};
};


StartupHintsPalette::StartupHintsPalette(QWidget *parent) :
    FloatingPalette(Qt::TopRightCorner,parent)
    , d(new Private)
{
    setObjectName("StartupHintsPalette");
    if(Settings::settings()->appStartupHintsEnabled->get().toBool()){
        setFixedSize(700,450);
        d->mLayout = new QVBoxLayout();
        d->mLayout->setContentsMargins(10,28,10,10);
        setLayout(d->mLayout);
        QString url = Settings::settings()->applicationStartupHintsDirectory() + "/index.html";
        d->mpWebView = new QWebEngineView(this);
        d->boardApi = new WebWidgetBoardAPI(0);
        d->mpWebView->setUrl(QUrl::fromLocalFile(url));
        d->mpWebView->setAcceptDrops(false);
        d->mLayout->addWidget(d->mpWebView);
        d->mButtonLayout = new QHBoxLayout();
        d->mLayout->addLayout(d->mButtonLayout);
        d->mShowNextTime = new QCheckBox(tr("Visible next time"),this);
        d->mShowNextTime->setCheckState(Qt::Checked);
        // stateChanged(int) 在 Qt6 已弃用，改用 toggled 并直接写设置
        connect(d->mShowNextTime, &QCheckBox::toggled, this, [this](bool checked) {
            Settings::settings()->appStartupHintsEnabled->setBool(checked);
        });
        d->mButtonLayout->addStretch();
        d->mButtonLayout->addWidget(d->mShowNextTime);
    }
    else
        hide();
}

StartupHintsPalette::~StartupHintsPalette()
{
//    DELETEPTR(d->mButtonLayout);
//    DELETEPTR(d->mLayout);
}

void StartupHintsPalette::paintEvent(QPaintEvent *event)
{
    FloatingPalette::paintEvent(event);

    QPainter painter(this);
    painter.drawPixmap(0, 0, QPixmap(":/images/close.svg"));
}


void StartupHintsPalette::close()
{
    hide();
}


void StartupHintsPalette::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->pos().x() >= 0 && event->pos().x() < QPixmap(":/images/close.svg").width()
        && event->pos().y() >= 0 && event->pos().y() < QPixmap(":/images/close.svg").height())
    {
        event->accept();
        close();
    }

    FloatingPalette::mouseReleaseEvent(event);
}

void StartupHintsPalette::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    adjustSizeAndPosition();
    move((parentWidget()->width() - width()) / 2, (parentWidget()->height() - height()) / 5);
}


int StartupHintsPalette::border()
{
    return 40;
}

void StartupHintsPalette::javaScriptWindowObjectCleared()
{
}
