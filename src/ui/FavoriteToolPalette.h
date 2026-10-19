#pragma once

#include <QtWidgets>

#include <memory>

#include "ui/ActionPalette.h"

class FavoriteToolPalette : public ActionPalette
{
    Q_OBJECT

    public:
        FavoriteToolPalette(QWidget* parent = 0);
        virtual ~FavoriteToolPalette();

    private:
        struct Private;
        std::unique_ptr<Private> d;

    private slots:
        void addFavorite();
};

