#pragma once

#include <QtWidgets>
#include <QFrame>

#include <memory>

class ColorPicker : public QFrame
{
    Q_OBJECT

    public:
        ColorPicker(QWidget* parent);
        ColorPicker(QWidget* parent, const QList<QColor>& colors, int pSelectedColorIndex = 0);
        virtual ~ColorPicker();
        QList<QColor> getColors() const;

        void setColors(const QList<QColor>& pColors);

        int selectedColorIndex() const;

        void setSelectedColorIndex(int pSelectedColorIndex);

    signals:
        void colorSelected(const QColor& color);

    protected:
        virtual void paintEvent ( QPaintEvent * event );
        virtual void mousePressEvent ( QMouseEvent * event );

    private:
        struct Private;
        std::unique_ptr<Private> d;
};

