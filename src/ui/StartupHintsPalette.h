#pragma once

class QCheckBox;
class QVBoxLayout;
class QHBoxLayout;

#include <QWebEngineView>

#include <memory>

#include "ui/FloatingPalette.h"
#include "api/WebWidgetBoardAPI.h"

class StartupHintsPalette : public FloatingPalette
{
    Q_OBJECT
public:
    StartupHintsPalette(QWidget *parent = 0);
    ~StartupHintsPalette();

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void showEvent(QShowEvent *event);

private:
    void close();
    int border();

    struct Private;
    std::unique_ptr<Private> d;

private slots:
    void javaScriptWindowObjectCleared();
};

