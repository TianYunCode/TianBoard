#pragma once

#include <QtWidgets>
#include <QWidget>
#include <QLineEdit>
#include <QStyleOption>

#include "web/WebView.h"

#include <memory>

class ClearButton;

class ExLineEdit : public QWidget
{
    Q_OBJECT;

public:
    ExLineEdit(QWidget *parent = 0);
    ~ExLineEdit() override;

    QLineEdit *lineEdit() const;

    void setLeftWidget(QWidget *widget);
    QWidget *leftWidget() const;

    QSize sizeHint() const;

    QVariant inputMethodQuery(Qt::InputMethodQuery property) const;

    void setVisible(bool pVisible);

protected:
    void focusInEvent(QFocusEvent *event);
    void focusOutEvent(QFocusEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void inputMethodEvent(QInputMethodEvent *e);
    bool event(QEvent *event);

protected:
    void updateGeometries();
    void initStyleOption(QStyleOptionFrame *option) const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};


class WebView;

class UrlLineEdit : public ExLineEdit
{
    Q_OBJECT;

public:
    UrlLineEdit(QWidget *parent = 0);
    ~UrlLineEdit() override;
    void setWebView(WebView *webView);

protected:

    void focusOutEvent(QFocusEvent *event);

private slots:
    void webViewUrlChanged(const QUrl &url);

private:
    struct Private;
    std::unique_ptr<Private> d;
};


