#pragma once

#include <QtWidgets>
#include <QTableView>

class EditTableView : public QTableView
{
    Q_OBJECT;

public:
    EditTableView(QWidget *parent = 0);
    void keyPressEvent(QKeyEvent *event);

public slots:
    void removeOne();
    void removeAll();
};


