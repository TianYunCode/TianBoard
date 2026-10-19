#pragma once

#include <memory>

#include "ui/ActionPalette.h"

class WebToolsPalette : public ActionPalette
{
    Q_OBJECT

public:
    WebToolsPalette(QWidget *parent);
    virtual ~WebToolsPalette();

private:
    struct Private;
    std::unique_ptr<Private> d;

};

