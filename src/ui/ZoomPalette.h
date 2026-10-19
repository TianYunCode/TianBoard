#pragma once

#include "ui/FloatingPalette.h"

#include <memory>

class QPushButton;
class BoardController;

class ZoomPalette : public FloatingPalette
{
    Q_OBJECT

public:
    ZoomPalette(QWidget *parent);
    virtual ~ZoomPalette();

public slots:
    void hide();
    void refreshPalette();

private slots:
    void showHideExtraButton();
    void goHundred();

private:
    struct Private;
    std::unique_ptr<Private> d;

};

