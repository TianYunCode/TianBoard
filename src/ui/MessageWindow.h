#pragma once

#include <QtWidgets>
#include <QHBoxLayout>
#include <QLabel>

#include <memory>

#include "ui/FloatingPalette.h"

class SpinningWheel;

class MessageWindow  : public FloatingPalette
{
    Q_OBJECT

    public:
        MessageWindow(QWidget *parent = 0);
        virtual ~MessageWindow();

        void showMessage(const QString& message, bool showSpinningWheel = false);
        void hideMessage();

    protected:
        void timerEvent(QTimerEvent *event);

    private:
        struct Private;
        std::unique_ptr<Private> d;
};

