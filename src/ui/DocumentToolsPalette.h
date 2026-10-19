#pragma once

#include <QButtonGroup>
#include <QUrl>
#include <QMap>

#include <memory>

#include "ui/ActionPalette.h"

class DocumentToolsPalette : public ActionPalette
{
    Q_OBJECT

    public:
        DocumentToolsPalette(QWidget *parent = 0);
        virtual ~DocumentToolsPalette();
        bool isEmpty() { return actions().count() == 0; }

    private:
        struct Private;
        std::unique_ptr<Private> d;

};

