#pragma once

#include <QWidget>
#include <QPixmap>
#include <QString>

#include <memory>

/** 白板当前处于哪个界面，决定哪些停靠面板可见 */
enum class DockWidgetMode { Board, Web, Document, Desktop };

class DockPaletteWidget : public QWidget {
    Q_OBJECT
public:
    DockPaletteWidget(QWidget *parent = 0, const char *name = "DockPaletteWidget");
    ~DockPaletteWidget();

    QPixmap iconToRight();
    QPixmap iconToLeft();
    QString name();

    virtual bool visibleInMode(DockWidgetMode mode) = 0;

    void registerMode(DockWidgetMode mode);

    bool visibleState();
    void setVisibleState(bool state);

signals:
    void hideTab(DockPaletteWidget *widget);
    void showTab(DockPaletteWidget *widget);

public slots:
    void slot_changeMode(DockWidgetMode newMode);

protected:
    void setName(const QString &name);
    void setIconToRight(const QPixmap &icon);
    void setIconToLeft(const QPixmap &icon);

    struct Private;
    std::unique_ptr<Private> d;
};
