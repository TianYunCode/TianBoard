#pragma once

#include <QtWidgets>
#include <QResizeEvent>

#include "ui/PropertyPalette.h"

class RightPalette;

class DesktopPropertyPalette : public PropertyPalette
{
    Q_OBJECT

    public:
        DesktopPropertyPalette(QWidget *parent, RightPalette* _rightPalette);
    private:
        RightPalette* rightPalette;
    protected:
        virtual int getParentRightOffset();
};

class DesktopPenPalette : public DesktopPropertyPalette
{
    Q_OBJECT
    public:
        DesktopPenPalette(QWidget *parent, RightPalette* rightPalette);
        virtual ~DesktopPenPalette(){}
    public slots:
        void onParentMinimized();
        void onParentMaximized();

    private slots:
        void onButtonReleased();

};

class DesktopEraserPalette : public DesktopPropertyPalette
{
    public:
        DesktopEraserPalette(QWidget *parent, RightPalette* rightPalette);
        virtual ~DesktopEraserPalette(){}
};

class DesktopMarkerPalette : public DesktopPropertyPalette
{
    public:
        DesktopMarkerPalette(QWidget *parent, RightPalette* rightPalette);
        virtual ~DesktopMarkerPalette(){}
};

