#pragma once

#include <memory>

#include "ui/ActionPalette.h"
#include "core/Application.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsScene.h"

class BackgroundPalette : public ActionPalette
{
Q_OBJECT

public:
    BackgroundPalette(QList<QAction*> actions, QWidget* parent = 0);
    BackgroundPalette(QWidget* parent = 0);
    ~BackgroundPalette() override;

    void addAction(QAction *action);
    void setActions(QList<QAction *> actions);
    void clearLayout();

public slots:
    void showEvent(QShowEvent* event);
    void backgroundChanged();
    void refresh();

protected slots:
    void sliderValueChanged(int value);
    void defaultBackgroundGridSize();

protected:
    virtual void updateLayout();
    void init();

    struct Private;
    std::unique_ptr<Private> d;

};

