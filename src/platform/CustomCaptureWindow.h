#pragma once

#include <QtWidgets>
#include <QDialog>
#include <QRubberBand>

#include <memory>

class CustomCaptureWindow : public QDialog
{
    Q_OBJECT

    public:
        CustomCaptureWindow(QWidget *parent = 0);
        virtual ~CustomCaptureWindow();

        int execute(const QPixmap &pScreenPixmap);

        QPixmap getSelectedPixmap();

    protected:
        virtual void showEvent ( QShowEvent * event );
        virtual void mouseMoveEvent ( QMouseEvent * event );
        virtual void mousePressEvent ( QMouseEvent * event );
        virtual void mouseReleaseEvent ( QMouseEvent * event );
        virtual void keyPressEvent ( QKeyEvent * event );
        virtual void paintEvent(QPaintEvent *event);

    private:
        struct Private;
        std::unique_ptr<Private> d;
};

