#pragma once

class DocumentProxy;

#include <QElapsedTimer>
#include <QWidget>
#include <QMouseEvent>
#include <QBrush>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QTime>
#include <QPoint>
#include <QPixmap>
#include <QMap>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVector>

#include <memory>

#include "ui/DockPaletteWidget.h"

#define TABSIZE 50    
#define CLICKTIME 1000000

/**
 * \brief The dock positions
 */
typedef enum
{
    eWBDockOrientation_Left, 
    eWBDockOrientation_Right,
    eWBDockOrientation_Top,
    eWBDockOrientation_Bottom
}eWBDockOrientation;

typedef enum
{
    eWBDockTabOrientation_Up,
    eWBDockTabOrientation_Down
}eWBDockTabOrientation;

class DockPalette;

class TabDockPalette : public QWidget
{
    Q_OBJECT
    friend class DockPalette;

public:
    TabDockPalette(DockPalette *dockPalette, QWidget *parent = 0);
    ~TabDockPalette();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void paintEvent(QPaintEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

typedef enum
{
    eWBDockPaletteType_LEFT,
    eWBDockPaletteType_RIGHT
} eWBDockPaletteType;


class DockPalette : public QWidget
{
    Q_OBJECT
    friend class TabDockPalette;

public:
    DockPalette(eWBDockPaletteType paletteType, QWidget* parent=0, const char* name="DockPalette");
    ~DockPalette();

    eWBDockOrientation orientation();
    void setOrientation(eWBDockOrientation orientation);
    void setTabsOrientation(eWBDockTabOrientation orientation);
    void showTabWidget(int tabIndex);
    QRect getTabPaletteRect();

    virtual void assignParent(QWidget *widget);
    virtual void setVisible(bool visible);

    virtual void paintEvent(QPaintEvent *event);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);

    void setBackgroundBrush(const QBrush& brush);
    void registerWidget(DockPaletteWidget* widget);

    void addTab(DockPaletteWidget* widget);
    void removeTab(DockPaletteWidget* widget);

    void connectSignals();

    bool switchMode(eWBDockPaletteWidgetMode mode);

    QVector<DockPaletteWidget*> GetWidgetsList();

public:
    bool isTabFlotable();
    void setTabFlotable(bool newFlotable);
    int getAdditionalVOffset() const;
    void setAdditionalVOffset(int newOffset);

    eWBDockPaletteType paletteType();

public slots:
    void onShowTabWidget(DockPaletteWidget* widget);
    void onHideTabWidget(DockPaletteWidget* widget);
    void onAllDownloadsFinished();
    virtual void onDocumentSet(DocumentProxy* documentProxy);

signals:
    void mouseEntered();
    void pageSelectionChangedRequired();

protected:
    virtual int border();
    virtual int radius();
    virtual int customMargin();
    virtual void updateMaxWidth();
    virtual void resizeEvent(QResizeEvent *event);
    virtual int collapseWidth();

    /** The current mode of the palette */
    eWBDockPaletteWidgetMode currentMode();
    /** The last width of the palette */
    int lastWidth();
    void setLastWidth(int width);
    void setCollapseWidth(int width);
    /** A flag used to allow the resize */
    bool canResize();
    /** Last opened tab index for the given mode */
    void setLastOpenedTabForMode(eWBDockPaletteWidgetMode mode, int tabIndex);

private slots:
    void onToolbarPosUpdated();
    void onResizeRequest(QResizeEvent* event);

private:
    void tabClicked(int tabIndex);
    int tabSpacing();
    void toggleCollapseExpand();
    void moveTabs();
    void resizeTabs();

    struct Private;
    std::unique_ptr<Private> d;
};

