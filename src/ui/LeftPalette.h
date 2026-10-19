#pragma once

#include "ui/DockPalette.h"

#include <memory>

class LeftPalette : public DockPalette {
public:
    LeftPalette(QWidget *parent = 0, const char *name = "LeftPalette");
    ~LeftPalette();

    bool switchMode(DockWidgetMode mode);

public slots:
    void onDocumentSet(DocumentProxy *documentProxy);

protected:
    void updateMaxWidth();
    void resizeEvent(QResizeEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
