#pragma once

#include <QWidget>
#include <QPixmap>
#include <QString>

#include <memory>

typedef enum
{
    eWBDockPaletteWidget_BOARD,
    eWBDockPaletteWidget_WEB,
    eWBDockPaletteWidget_DOCUMENT,
    eWBDockPaletteWidget_DESKTOP,
} eWBDockPaletteWidgetMode;

class DockPaletteWidget : public QWidget
{
    Q_OBJECT
public:
    DockPaletteWidget(QWidget* parent=0, const char* name="DockPaletteWidget");
    ~DockPaletteWidget();

    QPixmap iconToRight();
    QPixmap iconToLeft();
    QString name();

    virtual bool visibleInMode(eWBDockPaletteWidgetMode mode) = 0;

    void registerMode(eWBDockPaletteWidgetMode mode);

    bool visibleState();
    void setVisibleState(bool state);

signals:
    void hideTab(DockPaletteWidget* widget);
    void showTab(DockPaletteWidget* widget);

public slots:
    void slot_changeMode(eWBDockPaletteWidgetMode newMode);

protected:
    void setName(const QString& name);
    void setIconToRight(const QPixmap& icon);
    void setIconToLeft(const QPixmap& icon);

    struct Private;
    std::unique_ptr<Private> d;
};

