#include <QPoint>
#include <QPointF>
#include <QPainterPath>

#include "ui/DockPalette.h"

#include "platform/PlatformUtils.h"

#include "core/Settings.h"
#include "core/Application.h"
#include "core/PreferencesController.h"
#include "core/DownloadManager.h"

#include "whiteboard/BoardController.h"


struct TabDockPalette::Private
{
    DockPalette *dock{nullptr};
    int mVerticalOffset{0};
    bool mFlotable{false};
};

struct DockPalette::Private
{
    /** The current mode */
    eWBDockPaletteWidgetMode mCurrentMode{eWBDockPaletteWidget_BOARD};
    /** The current dock orientation */
    eWBDockOrientation mOrientation{eWBDockOrientation_Left};
    /** The current background brush */
    QBrush mBackgroundBrush;
    /** The preferred width */
    int mPreferredWidth{1000};
    /** The preferred height */
    int mPreferredHeight{1000};
    /** A flag used to allow the resize */
    bool mCanResize{false};
    /** A flag indicating if the palette has been resized between a click and a release */
    bool mResized{false};
    /** The width that trig the collapse */
    int mCollapseWidth{1500};
    /** The last width of the palette */
    int mLastWidth{-1};
    /** The click time*/
    QElapsedTimer mClickTime;
    /** The mouse pressed position */
    QPoint mMousePressPos;
    /** The tab orientation */
    eWBDockTabOrientation mTabsOrientation{eWBDockTabOrientation_Up};
    /** The h position of the tab */
    int mHTab{0};
    /** The stacked widget */
    QStackedWidget* mpStackWidget{nullptr};
    /** The layout */
    QVBoxLayout* mpLayout{nullptr};
    /** The current tab index */
    int mCurrentTab{0};
    /** The visible tab widgets */
    QVector<DockPaletteWidget*> mTabWidgets;
    /** The current widget */
    QVector<DockPaletteWidget*> mRegisteredWidgets;
    /** The current tab widget */
    QString mCrntTabWidget;
    /** Last opened tab index depending on mode */
    QMap<eWBDockPaletteWidgetMode,int> mLastOpenedTabForMode;

    eWBDockPaletteType mPaletteType{eWBDockPaletteType_LEFT};
    TabDockPalette *mTabPalette{nullptr};
};

/**
 * \brief The constructor
 */
DockPalette::DockPalette(eWBDockPaletteType paletteType, QWidget *parent, const char *name)
:QWidget(parent, Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint)
, d(new Private)
{
    d->mPaletteType = paletteType;
    d->mTabPalette = new TabDockPalette(this, parent);

    setObjectName(name);

    d->mpLayout = new QVBoxLayout();
    setLayout(d->mpLayout);

    d->mpStackWidget = new QStackedWidget(this);
    d->mpLayout->addWidget(d->mpStackWidget);

    // clear the tab widgets
    d->mTabWidgets.clear();

    // We let 2 pixels in order to keep a small border for the resizing
    setMinimumWidth(0);

    if (parent)
    {
        setAttribute(Qt::WA_NoMousePropagation);
        setAttribute(Qt::WA_TranslucentBackground);
    }
    else
    {
        // standalone window
        setAttribute(Qt::WA_TranslucentBackground);
    }

    d->mBackgroundBrush = QBrush(Settings::paletteColor);

    // This is the only way to set the background as transparent!
    setStyleSheet("QWidget {background-color: transparent}");

    // Set the position of the tab
    onToolbarPosUpdated();
    connect(Settings::settings()->appToolBarPositionedAtTop, SIGNAL(changed(QVariant)), this, SLOT(onToolbarPosUpdated()));
    connect(DownloadManager::downloadManager(), SIGNAL(allDownloadsFinished()), this, SLOT(onAllDownloadsFinished()));

    connect(Application::boardController,SIGNAL(documentSet(DocumentProxy*)),this,SLOT(onDocumentSet(DocumentProxy*)));
    connect(this,SIGNAL(pageSelectionChangedRequired()),Application::boardController,SLOT(selectionChanged()));
}

/**
 * \brief The destructor
 */
DockPalette::~DockPalette()
{
    if(NULL != d->mpStackWidget)
    {
        delete d->mpStackWidget;
        d->mpStackWidget = NULL;
    }
    if(NULL != d->mpLayout)
    {
        delete d->mpLayout;
        d->mpLayout = NULL;
    }
}

eWBDockPaletteWidgetMode DockPalette::currentMode()
{
    return d->mCurrentMode;
}

int DockPalette::lastWidth()
{
    return d->mLastWidth;
}

void DockPalette::setLastWidth(int width)
{
    d->mLastWidth = width;
}

void DockPalette::setCollapseWidth(int width)
{
    d->mCollapseWidth = width;
}

bool DockPalette::canResize()
{
    return d->mCanResize;
}

void DockPalette::setLastOpenedTabForMode(eWBDockPaletteWidgetMode mode, int tabIndex)
{
    d->mLastOpenedTabForMode.insert(mode, tabIndex);
}

QVector<DockPaletteWidget*> DockPalette::GetWidgetsList()
{
    return d->mRegisteredWidgets;
}

bool DockPalette::isTabFlotable()
{
    return d->mTabPalette->d->mFlotable;
}

void DockPalette::setTabFlotable(bool newFlotable)
{
    d->mTabPalette->d->mFlotable = newFlotable;
}

int DockPalette::getAdditionalVOffset() const
{
    return d->mTabPalette->d->mVerticalOffset;
}

void DockPalette::setAdditionalVOffset(int newOffset)
{
    d->mTabPalette->d->mVerticalOffset = newOffset;
}

eWBDockPaletteType DockPalette::paletteType()
{
    return d->mPaletteType;
}

void DockPalette::onDocumentSet(DocumentProxy* documentProxy)
{
    Q_UNUSED(documentProxy);
}

/**
 * \brief Get the current orientation
 * @return the current orientation
 */
eWBDockOrientation DockPalette::orientation()
{
    return d->mOrientation;
}

/**
 * \brief Set the current orientation
 * @param orientation as the given orientation
 */
void DockPalette::setOrientation(eWBDockOrientation orientation)
{
    // Set the size
    d->mOrientation = orientation;

    if(orientation == eWBDockOrientation_Left || orientation == eWBDockOrientation_Right)
    {
        setMaximumHeight(parentWidget()->height());
        setMinimumHeight(maximumHeight());
    }
    else if(orientation == eWBDockOrientation_Top || orientation == eWBDockOrientation_Bottom)
    {
        setMaximumWidth(parentWidget()->width());
        setMinimumWidth(maximumWidth());
    }
}

/**
 * \brief Handle the resize event
 * @param event as the resize event
 */
void DockPalette::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updateMaxWidth();
    if(parentWidget())
    {
        setMinimumHeight(parentWidget()->height());
    }
    // Set the position
    QPoint origin;
    switch(d->mOrientation)
    {
    case eWBDockOrientation_Right:
        origin.setX(parentWidget()->width() - this->width());
        origin.setY(0);
        break;
    case eWBDockOrientation_Bottom:
        // Not supported yet
    case eWBDockOrientation_Top:
        // Not supported yet
    case eWBDockOrientation_Left:
    default:
        origin.setX(0);
        origin.setY(0);
        break;
    }
    move(origin.x(), origin.y());
    moveTabs();
}

/**
 * \brief Handle the mouse enter event
 * @param event as the mouse event
 */
void DockPalette::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    // We want to set the cursor as an arrow everytime it enters the palette
    setCursor(Qt::ArrowCursor);
    emit mouseEntered();
}

/**
 * \brief Handle the mouse leave event
 * @param event as the mouse event
 */
void DockPalette::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    // Restore the cursor to its previous shape
    unsetCursor();
}

/**
 * \brief Draw the palette
 * @param event as the paint event
 */
void DockPalette::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(d->mBackgroundBrush);
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);

    int nbTabs = d->mTabWidgets.size();
    if(0 < nbTabs)
    {
        // First draw the BIG RECTANGLE (I write it big because the rectangle is big...)
        if(d->mOrientation == eWBDockOrientation_Left)
        {
            path.addRect(0.0, 0.0, width(), height());
        }
        else if(d->mOrientation == eWBDockOrientation_Right)
        {
            path.addRect(0.0, 0.0, width(), height());
        }

        painter.drawPath(path);
    }
}

/**
 * \brief Set the background brush
 * @param brush as the given brush
 */
void DockPalette::setBackgroundBrush(const QBrush &brush)
{
    if (d->mBackgroundBrush != brush)
    {
        d->mBackgroundBrush = brush;
        update();
    }
}

/**
 * Get the border size
 * @return the border size
 */
int DockPalette::border()
{
    return 10;
}

/**
 * \brief Get the radius size
 * @return the radius size
 */
int DockPalette::radius()
{
    return 5;
}

/**
 * \brief Set the maximum width
 */
void DockPalette::updateMaxWidth()
{
    // Only the inherited class will overload this method
}

/**
 * \brief Get the collapse width value
 * @return the collapse widht value
 */
int DockPalette::collapseWidth()
{
    return d->mCollapseWidth;
}

/**
 * \brief collapse/expand automatically the palette after a click on its tab
 */
void DockPalette::tabClicked(int tabIndex)
{
    // If the current tab is not the clicked one, show its content
    if(d->mCurrentTab != tabIndex)
    {
        showTabWidget(tabIndex);
    }
    // else collapse the palette
    else
    {
        toggleCollapseExpand();
    }
    d->mTabPalette->update();
    emit pageSelectionChangedRequired();
}

/**
 * \brief Show the tab widget related to the given index
 * @param tabIndex as the given index
 */
void DockPalette::showTabWidget(int tabIndex)
{
    d->mpStackWidget->setCurrentIndex(tabIndex);
    d->mCurrentTab = tabIndex;

    // Update the current tab index
    if(NULL != (dynamic_cast<DockPaletteWidget*>(d->mpStackWidget->widget(tabIndex)))){
        d->mCrntTabWidget = dynamic_cast<DockPaletteWidget*>(d->mpStackWidget->widget(tabIndex))->name();
    }

}

/**
 * \brief Toggle the collapse / expand state
 */
void DockPalette::toggleCollapseExpand()
{
    if(width() < d->mCollapseWidth)
        resize(d->mLastWidth,height());
    else{
        d->mLastWidth = width();
        update();
        resize(0,height());
    }
}

/**
 * \brief Set the tabs orientation
 * @param orientation as the given tabs orientation
 */
void DockPalette::setTabsOrientation(eWBDockTabOrientation orientation)
{
    d->mTabsOrientation = orientation;
}

/**
 * \brief Update the tab position regarding the toolbar position (up or down)
 */
void DockPalette::onToolbarPosUpdated()
{
    // Get the position of the tab
    if(Settings::settings()->appToolBarPositionedAtTop->get().toBool())
    {
        setTabsOrientation(eWBDockTabOrientation_Up);
    }
    else
    {
        setTabsOrientation(eWBDockTabOrientation_Down);
    }
    moveTabs();
    update();
}

/**
 * \brief Get the custom margin
 * @return the custom margin value
 */
int DockPalette::customMargin()
{
    return 5;
}

/**
 * \brief Add the given tab widget
 * @param widget as the given widget
 */
void DockPalette::addTab(DockPaletteWidget *widget)
{
    if(!d->mTabWidgets.contains(widget) && widget->visibleState())
    {
        widget->setVisible(true);
        d->mTabWidgets.append(widget);
        d->mpStackWidget->addWidget(widget);
        d->mpStackWidget->setCurrentWidget(widget);
        resizeTabs();
        update();
    }
}
/**
 * \brief Remove the given tab
 * @param widgetName as the tab widget name
 */
void DockPalette::removeTab(DockPaletteWidget* widget)
{
    int nWidget = d->mTabWidgets.indexOf(widget);
    if( nWidget >= 0 )
    {
        d->mpStackWidget->removeWidget(widget);
        d->mTabWidgets.remove(nWidget);
        widget->hide();
        update();
    }
    resizeTabs();
    d->mCurrentTab = qMax(d->mCurrentTab - 1, 0);
}

/**
 * \brief Handle the resize request
 * @param event as the given resize request
 */
void DockPalette::onResizeRequest(QResizeEvent *event)
{
    resizeEvent(event);
}

/**
 * \brief Get the tab spacing
 * @return the tab spacing
 */
int DockPalette::tabSpacing()
{
    return 2;
}

/**
 * \brief Show the given widget
 * @param widgetName as the given widget name
 */
void DockPalette::onShowTabWidget(DockPaletteWidget* widget)
{
    if (d->mRegisteredWidgets.contains(widget))
    {
        widget->setVisibleState(true);
        addTab(widget);
        moveTabs();
    }
}

/**
 * \brief Hide the given widget
 * @param widgetName as the given widget name
 */
void DockPalette::onHideTabWidget(DockPaletteWidget* widget)
{
    if (d->mRegisteredWidgets.contains(widget))
    {
        widget->setVisibleState(false);
        removeTab(widget);
        moveTabs();
    }
}

/**
 * \brief Connect the show / hide signals of the widget related to this dock palette
 */
void DockPalette::connectSignals()
{
    for(int i=0; i < d->mRegisteredWidgets.size(); i++)
    {
        connect(d->mRegisteredWidgets.at(i), SIGNAL(showTab(DockPaletteWidget*)), this, SLOT(onShowTabWidget(DockPaletteWidget*)));
        connect(d->mRegisteredWidgets.at(i), SIGNAL(hideTab(DockPaletteWidget*)), this, SLOT(onHideTabWidget(DockPaletteWidget*)));
    }
}

/**
 * \brief Register the given widget
 * @param widget as the given widget
 */
void DockPalette::registerWidget(DockPaletteWidget *widget)
{
    if(!d->mRegisteredWidgets.contains(widget))
    {
        // Update the parent of this widget
        widget->setParent(this);
        d->mRegisteredWidgets.append(widget);

        // By default, the widget is hidden
        widget->hide();
    }
}

/**
 * \brief Handles the 'all download finished' notification
 */
void DockPalette::onAllDownloadsFinished()
{
    for(int i=0; i<d->mTabWidgets.size(); i++){
        DockPaletteWidget* pW = d->mTabWidgets.at(i);
        if(NULL != pW && d->mCrntTabWidget == pW->name()){
            d->mpStackWidget->setCurrentWidget(pW);
        }
    }
}

void DockPalette::moveTabs()
{
    int x = width();
    if (d->mOrientation == eWBDockOrientation_Right)
    {
        if (parentWidget())
            x = parentWidget()->width() - width() - border() * 2;
    }

    int y = border();
    if(eWBDockTabOrientation_Down == d->mTabsOrientation)
    {
        if (d->mOrientation == eWBDockOrientation_Right)
        {
            if (parentWidget())
                y = parentWidget()->height() - border()- d->mTabWidgets.size() * TABSIZE;
        }
        else
            y = height() - border()- d->mTabWidgets.size() * TABSIZE;
    }

    d->mHTab = y;

    d->mTabPalette->move(x,y + d->mTabPalette->d->mVerticalOffset);
}

void DockPalette::resizeTabs()
{
    int numTabs = d->mTabWidgets.size();
    d->mTabPalette->setFixedSize(2 * border(), (numTabs * TABSIZE) + qMax(numTabs - 1, 0) * tabSpacing());
}

QRect DockPalette::getTabPaletteRect()
{
    QRect tabsRect = d->mTabPalette->rect();
    QPoint topLeft = d->mTabPalette->mapToParent(tabsRect.topLeft());
    QPoint bottomRight = d->mTabPalette->mapToParent(tabsRect.bottomRight());

    return QRect(topLeft, bottomRight);
}

void DockPalette::assignParent(QWidget *widget)
{
    setParent(widget);
    d->mTabPalette->setParent(widget);
}

void DockPalette::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    d->mTabPalette->setVisible(visible);
}

bool DockPalette::switchMode(eWBDockPaletteWidgetMode mode)
{
    d->mLastOpenedTabForMode.insert(d->mCurrentMode, d->mpStackWidget->currentIndex());
    d->mCurrentMode = mode;
    bool hasVisibleElements = false;
    //-------------------------------//
    // get full palette widgets list, parse it, show all widgets for BOARD mode, and hide all other
    for(int i = 0; i < d->mRegisteredWidgets.size(); i++)
    {
        DockPaletteWidget* pNextWidget = d->mRegisteredWidgets.at(i);
        if( pNextWidget != NULL )
        {
            if( pNextWidget->visibleInMode(mode) )
            {
                addTab(pNextWidget);
                hasVisibleElements = true;
            }
            else
            {
                removeTab(pNextWidget);
            }
        }
    }
    //-------------------------------//

    if(d->mRegisteredWidgets.size() > 0)
        showTabWidget(d->mLastOpenedTabForMode.value(d->mCurrentMode));

    update();

    return hasVisibleElements;
}


TabDockPalette::TabDockPalette(DockPalette *dockPalette, QWidget *parent) :
            QWidget(parent)
, d(new Private)
{
    d->dock = dockPalette;

    int numTabs = dockPalette->d->mTabWidgets.size();
    resize(2 * dockPalette->border(), (numTabs * TABSIZE) + qMax(numTabs - 1, 0) * dockPalette->tabSpacing());

    setAttribute(Qt::WA_TranslucentBackground);
}

void TabDockPalette::paintEvent(QPaintEvent *)
{
    int nTabs = d->dock->d->mTabWidgets.size();
    if (nTabs <= 0) {
        qDebug() << "not enough tabs";
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(d->dock->d->mBackgroundBrush);

    int yFrom = 0;
    for (int i = 0; i < nTabs; i++) {
        DockPaletteWidget* pCrntWidget = d->dock->d->mTabWidgets.at(i);
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);
        QPixmap iconPixmap;

        switch (d->dock->d->mOrientation) {
        case eWBDockOrientation_Left://zhusizhi
            path.addRect(0, yFrom, width() / 2, TABSIZE);
            path.addRoundedRect(0, yFrom, width(), TABSIZE, d->dock->radius(), d->dock->radius());
            if (pCrntWidget) {
                if(d->dock->d->mCollapseWidth >= d->dock->width()) {
                    // Get the collapsed icon
                    iconPixmap = pCrntWidget->iconToRight();
                } else {
                    // Get the expanded icon
                    iconPixmap = pCrntWidget->iconToLeft();
                }

            }
            break;

        case eWBDockOrientation_Right:
            path.addRect(width() /2, yFrom, width() / 2, TABSIZE);
            path.addRoundedRect(0, yFrom, width(), TABSIZE, d->dock->radius(), d->dock->radius());
            if (pCrntWidget) {
                if(d->dock->d->mCollapseWidth >= d->dock->width()) {
                    // Get the collapsed icon
                    iconPixmap = pCrntWidget->iconToLeft();
                } else {
                    // Get the expanded icon
                    iconPixmap = pCrntWidget->iconToRight();
                }
            }
            break;

        case eWBDockOrientation_Top: ;
        case eWBDockOrientation_Bottom: ;
        default:
            break;
        }

        painter.save();
        //QPixmap transparencyPix(":/images/tab_mask.png");
        if (d->dock->d->mCurrentTab != i) {
            //iconPixmap.setAlphaChannel(transparencyPix); // deprecated; removed for now, to be re-implemented
            QColor color(0x7F, 0x7F, 0x7F, 0x3F);
            painter.setBrush(QBrush(color));
        }

        painter.drawPath(path);
        painter.drawPixmap(2, yFrom + 2, width() - 4, TABSIZE - 4, iconPixmap);
        yFrom += (TABSIZE + d->dock->tabSpacing());
        painter.restore();
    }
}
TabDockPalette::~TabDockPalette()
{
}

void TabDockPalette::mousePressEvent(QMouseEvent *event)
{
    d->dock->d->mClickTime.start();
    // The goal here is to verify if the user can resize the widget.
    // It is only possible to resize it if the border is selected
    QPoint p = event->pos();
    d->dock->d->mMousePressPos = p;
    d->dock->d->mResized = false;

    switch(d->dock->d->mOrientation) {
    case eWBDockOrientation_Left:
        d->dock->d->mCanResize = true;
        break;
    case eWBDockOrientation_Right:
        d->dock->d->mCanResize = true;
        break;
    case eWBDockOrientation_Top:
        // Not supported yet
        break;
    case eWBDockOrientation_Bottom:
        // Not supported yet
        break;
    default:
        break;
    }
}
void TabDockPalette::mouseMoveEvent(QMouseEvent *event)
{
    QPoint p = event->pos();

    if(d->dock->d->mCanResize && ((d->dock->d->mMousePressPos - p).manhattanLength() > QApplication::startDragDistance()))
    {
        if (qAbs(d->dock->d->mMousePressPos.y() - p.y()) > 10 && d->mFlotable) {
            qDebug() << "y differences" << d->dock->d->mMousePressPos.y() << p.y();
            d->mVerticalOffset += p.y() - d->dock->d->mMousePressPos.y();
            move(this->pos().x(),  p.y());
        }

        switch(d->dock->d->mOrientation) {

        case eWBDockOrientation_Left:
            p.setX(p.x() + d->dock->width());
            if(p.x() < d->dock->collapseWidth() && p.x() >= d->dock->minimumWidth()) {
                d->dock->update();
                d->dock->resize(0, d->dock->height());
                //dock->mLastWidth = dock->collapseWidth() + 1;
                d->dock->d->mResized = true;
            } else if (p.x() <= d->dock->maximumWidth() && p.x() >= d->dock->minimumWidth()) {
                d->dock->resize(p.x(), d->dock->height());
                d->dock->d->mResized = true;
            }
            break;

        case eWBDockOrientation_Right:
            p.setX(p.x() - 2 * d->dock->border());
            if((d->dock->x() + p.x() > d->dock->parentWidget()->width() - d->dock->collapseWidth()) && (d->dock->x() + p.x() < d->dock->parentWidget()->width())) {
                d->dock->update();
                d->dock->resize(0, d->dock->height());
                //dock->mLastWidth = dock->collapseWidth() + 1;
                d->dock->d->mResized = true;
            } else if((d->dock->x() + p.x() >= d->dock->parentWidget()->width() - d->dock->maximumWidth()) && (d->dock->x() + p.x() <= d->dock->parentWidget()->width() - d->dock->minimumWidth())) {
                d->dock->resize(d->dock->parentWidget()->width() - (d->dock->x() + p.x()), d->dock->height());
                d->dock->d->mResized = true;
            }
            break;

        case eWBDockOrientation_Top:
        case eWBDockOrientation_Bottom:
            if(p.y() <= d->dock->maximumHeight()) {
                d->dock->resize(width(), p.y());
                d->dock->d->mResized = true;
            }
            break;

        default:
            break;
        }
    }
}

void TabDockPalette::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if(!d->dock->d->mResized && d->dock->d->mClickTime.elapsed() < CLICKTIME) {
        int nbTabs = d->dock->d->mTabWidgets.size();
        int clickedTab = 0;
        // If the clicked position is in the tab, perform the related action

        if(d->dock->d->mMousePressPos.x() >= 0 &&
                d->dock->d->mMousePressPos.x() <= width() &&
                d->dock->d->mMousePressPos.y() >= 0 &&
                d->dock->d->mMousePressPos.y() <= nbTabs * TABSIZE + (nbTabs -1)*d->dock->tabSpacing()) {

            clickedTab = (d->dock->d->mMousePressPos.y()) / (TABSIZE + d->dock->tabSpacing());
            d->dock->tabClicked(clickedTab);
        }
    }
    d->dock->d->mCanResize = false;
}
