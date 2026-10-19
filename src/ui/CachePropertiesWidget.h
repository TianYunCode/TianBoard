#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QVector>

#include "ui/DockPaletteWidget.h"
#include "whiteboard/GraphicsCache.h"

#define MAX_SHAPE_WIDTH 200

class CachePropertiesWidget : public DockPaletteWidget
{
    Q_OBJECT
public:
    CachePropertiesWidget(QWidget* parent=0, const char* name="CachePropertiesWidget");
    ~CachePropertiesWidget();

    bool visibleInMode(eWBDockPaletteWidgetMode mode)
    {
        return mode == eWBDockPaletteWidget_BOARD;
    }

public slots:
    void updateCurrentCache();

private slots:
    void onCloseClicked();
    void updateCacheColor(QColor color);
    void onColorClicked();
    void updateShapeButtons();
    void onSizeChanged(int newSize);
    void onCacheEnabled();

private:
    QVBoxLayout* mpLayout;
    QLabel* mpCachePropertiesLabel;
    QLabel* mpColorLabel;
    QLabel* mpShapeLabel;
    QLabel* mpSizeLabel;
    QPushButton* mpColor;
    QPushButton* mpSquareButton;
    QPushButton* mpCircleButton;
    QPushButton* mpCloseButton;
    QSlider* mpSizeSlider;
    QHBoxLayout* mpColorLayout;
    QHBoxLayout* mpShapeLayout;
    QHBoxLayout* mpSizeLayout;
    QHBoxLayout* mpCloseLayout;
    QWidget* mpProperties;
    QVBoxLayout* mpPropertiesLayout;
    QColor mActualColor;
    eMaskShape mActualShape;
    GraphicsCache* mpCurrentCache;

};

