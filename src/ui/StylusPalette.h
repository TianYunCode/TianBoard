#pragma once

#include <QButtonGroup>

#include <memory>

#include "ui/ActionPalette.h"

class StylusPalette : public ActionPalette
{
    Q_OBJECT

public:
    StylusPalette(QWidget *parent = 0, Qt::Orientation orient = Qt::Vertical);
    virtual ~StylusPalette();

    void initPosition();
		
private slots:
    void stylusToolDoubleClicked();

private:
    struct Private;
    std::unique_ptr<Private> d;

signals:
    void stylusToolDoubleClicked(int tool);
};

