#pragma once

#include "ui/DockPalette.h"

#include <memory>

class RightPalette : public DockPalette
{
    Q_OBJECT
public:
    RightPalette(QWidget* parent=0, const char* name="RightPalette");
    ~RightPalette();
    bool switchMode(eWBDockPaletteWidgetMode mode);

signals:
    void resized();

protected:
    void updateMaxWidth();
    void mouseMoveEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;

};

