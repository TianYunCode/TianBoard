#pragma once

#include <QtWidgets>

class DesktopAnnotationController;

class WindowCapture : public QObject
{
    Q_OBJECT

    public:
        WindowCapture(DesktopAnnotationController *parent = 0);
        virtual ~WindowCapture();
        int execute();
        const QPixmap getCapturedWindow();

    private:
        QPixmap mWindowPixmap;
        DesktopAnnotationController* mParent;
};
