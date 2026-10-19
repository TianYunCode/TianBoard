#pragma once

#include "web/UrlLineEdit.h"

#include <QtWidgets>
#include <QAbstractButton>

class SearchButton;

class ClearButton : public QAbstractButton
{
    Q_OBJECT;

    public:
        ClearButton(QWidget *parent = 0);
        void paintEvent(QPaintEvent *event);

    public slots:
        void textChanged(const QString &text);
};

class SearchLineEdit : public ExLineEdit
{
    Q_OBJECT;
    Q_PROPERTY(QString inactiveText READ inactiveText WRITE setInactiveText)

public:
    SearchLineEdit(QWidget *parent = 0);

    QString inactiveText() const;
    void setInactiveText(const QString &text);

    QMenu *menu() const;
    void setMenu(QMenu *menu);

    void setVisible(bool pVisible);

protected:
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);
signals:
	void textChanged(const QString &text);

private:
    void updateGeometries();

    SearchButton *mSearchButton;
    QString mInactiveText;
};


