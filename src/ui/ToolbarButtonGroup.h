#pragma once

#include <QtWidgets>
#include <QWidget>
#include <QToolBar>
#include <QToolButton>
#include <QActionGroup>

class ToolbarButtonGroup : public QWidget {
    Q_OBJECT

public:
    ToolbarButtonGroup(QToolBar *toolbar, const QList<QAction *> &actions = QList<QAction *>());
    virtual ~ToolbarButtonGroup();

    void setIcon(const QIcon &icon, int index);
    void setColor(const QColor &color, int index);
    int currentIndex() const;

protected:
    void paintEvent(QPaintEvent *);

public slots:
    void setCurrentIndex(int index);
    void colorPaletteChanged();
    void displayText(QVariant display);

private slots:
    void selected(QAction *action);

signals:
    void activated(int index);
    void currentIndexChanged(int index);

private:
    QToolButton *mToolButton;
    QString mLabel;
    QList<QAction *> mActions;
    QList<QToolButton *> mButtons;
    int mCurrentIndex;
    bool mDisplayLabel;
    QActionGroup *mActionGroup;
};
