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

/** 停靠面板所在的一侧 */
enum class DockOrientation { Left, Right, Top, Bottom };

/** 标签朝向 */
enum class DockTabOrientation { Up, Down };

class DockPalette;

class TabDockPalette : public QWidget {
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

/** 左/右停靠面板 */
enum class DockPaletteType { Left, Right };


class DockPalette : public QWidget {
    Q_OBJECT
    friend class TabDockPalette;

public:
    DockPalette(DockPaletteType paletteType, QWidget *parent = 0, const char *name = "DockPalette");
    ~DockPalette();

    DockOrientation orientation();
    void setOrientation(DockOrientation orientation);
    void setTabsOrientation(DockTabOrientation orientation);
    void showTabWidget(int tabIndex);
    QRect getTabPaletteRect();

    virtual void assignParent(QWidget *widget);
    virtual void setVisible(bool visible);

    virtual void paintEvent(QPaintEvent *event);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);

    void setBackgroundBrush(const QBrush &brush);
    void registerWidget(DockPaletteWidget *widget);

    void addTab(DockPaletteWidget *widget);
    void removeTab(DockPaletteWidget *widget);

    void connectSignals();

    bool switchMode(DockWidgetMode mode);

    QVector<DockPaletteWidget *> GetWidgetsList();

public:
    bool isTabFlotable();
    void setTabFlotable(bool newFlotable);
    int getAdditionalVOffset() const;
    void setAdditionalVOffset(int newOffset);

    DockPaletteType paletteType();

public slots:
    void onShowTabWidget(DockPaletteWidget *widget);
    void onHideTabWidget(DockPaletteWidget *widget);
    void onAllDownloadsFinished();
    virtual void onDocumentSet(DocumentProxy *documentProxy);

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
    DockWidgetMode currentMode();
    /** The last width of the palette */
    int lastWidth();
    void setLastWidth(int width);
    void setCollapseWidth(int width);
    /** A flag used to allow the resize */
    bool canResize();
    /** Last opened tab index for the given mode */
    void setLastOpenedTabForMode(DockWidgetMode mode, int tabIndex);

private slots:
    void onToolbarPosUpdated();
    void onResizeRequest(QResizeEvent *event);

private:
    void tabClicked(int tabIndex);
    int tabSpacing();
    void toggleCollapseExpand();
    void moveTabs();
    void resizeTabs();

    struct Private;
    std::unique_ptr<Private> d;
};
