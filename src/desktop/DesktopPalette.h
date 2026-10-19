#pragma once
#include <QtWidgets>
#include <QShowEvent>
#include <QHideEvent>

#include <memory>

#include "ui/ActionPalette.h"
#include "ui/RightPalette.h"

class DesktopPalette : public ActionPalette {
    Q_OBJECT

public:
    DesktopPalette(QWidget *parent, RightPalette *rightPalette);
    virtual ~DesktopPalette();

    void disappearForCapture();
    void appear();
    QPoint buttonPos(QAction *action);

signals:
    void boardClick();
    void customClick();
    void windowClick();
    void screenClick();

    //#ifdef Q_OS_LINUX //TODO: check why this produces an error on linux if uncommented
    void refreshMask();
    //#endif

public slots:
    void showHideClick(bool checked);
    void updateShowHideState(bool pShowEnabled);
    void setShowHideButtonVisible(bool visible);
    void setDisplaySelectButtonVisible(bool show);
    void minimizeMe(eMinimizedLocation location);
    void maximizeMe();
    void parentResized();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

    virtual int getParentRightOffset();

private:
    void adjustPosition();

    struct Private;
    std::unique_ptr<Private> d;

signals:
    void stylusToolChanged(int tool);
};
