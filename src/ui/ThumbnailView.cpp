#include "ui/ThumbnailView.h"
#include "whiteboard/GraphicsScene.h"

#include "core/MimeData.h"


struct ThumbnailView::Private
{
    QHBoxLayout* mHBoxLayout{nullptr};
};

ThumbnailView::ThumbnailView(GraphicsScene *scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , d(new Private)
{
    d->mHBoxLayout = new QHBoxLayout(this);

    setAcceptDrops(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

    int nominalWidth = scene->nominalSize().width();
    int nominalHeight = scene->nominalSize().height();
    QRectF nominalSceneRect(-nominalWidth/2, -nominalHeight/2, nominalWidth, nominalHeight);
    fitInView(nominalSceneRect, Qt::KeepAspectRatio);
    setSceneRect(nominalSceneRect);

    setStyleSheet( "QGraphicsView { border-style: none; }" );

    setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);

    //set stylesheet
    setObjectName("DockPaletteWidgetBox");
    setStyleSheet("background:white");

    d->mHBoxLayout->setAlignment(Qt::AlignHCenter);
}

ThumbnailView::~ThumbnailView()
{

}
