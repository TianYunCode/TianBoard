#pragma once

#include <QtWidgets>
#include <QPoint>
#include <QButtonGroup>
#include <QToolButton>

#include <memory>

#include "ui/FloatingPalette.h"

class ActionPaletteButton;

class ActionPalette : public FloatingPalette {
    Q_OBJECT

public:
    ActionPalette(QList<QAction *> actions, Qt::Orientation orientation = Qt::Vertical, QWidget *parent = 0);
    ActionPalette(Qt::Orientation orientation, QWidget *parent = 0);
    ActionPalette(Qt::Corner corner, QWidget *parent = 0, Qt::Orientation orient = Qt::Vertical);
    ActionPalette(QWidget *parent = 0);

    virtual ~ActionPalette();

    void setButtonIconSize(const QSize &size);
    void setToolButtonStyle(Qt::ToolButtonStyle);

    QList<QAction *> actions();
    virtual void setActions(QList<QAction *> actions);
    void groupActions();
    virtual void addAction(QAction *action);

    void setClosable(bool closable);
    void setAutoClose(bool autoClose);

    void setCustomCloseProcessing(bool customCloseProcessing);

    virtual int border();
    virtual void clearLayout();
    QSize buttonSize();

    virtual ActionPaletteButton *getButtonFromAction(QAction *action);

public slots:
    void close();

signals:
    void closed();
    void buttonGroupClicked(int id);
    void customMouseReleased();

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void init(Qt::Orientation orientation);

    virtual void updateLayout();

    ActionPaletteButton *createPaletteButton(QAction *action, QWidget *parent);

    const QList<ActionPaletteButton *> &buttons() const;
    void clearButtons();
    QButtonGroup *buttonGroup() const;
    void setButtonGroup(QButtonGroup *group);
    Qt::ToolButtonStyle toolButtonStyle() const;
    void appendAction(QAction *action);
    void clearActions();
    void clearButtonMap();
    void initButtonState();

private:
    struct Private;
    std::unique_ptr<Private> d;

protected slots:
    void buttonClicked();
    void actionChanged();
};

class ActionPaletteButton : public QToolButton {
    Q_OBJECT

public:
    ActionPaletteButton(QAction *action, QWidget *parent = 0);
    virtual ~ActionPaletteButton();

signals:
    void doubleClicked();

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    virtual bool hitButton(const QPoint &pos) const;
};
