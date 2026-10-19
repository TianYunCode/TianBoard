#pragma once

#include "ui/ActionPalette.h"

#include <QLayout>
#include <QPainter>
#include <QMenu>
#include <QIcon>

#include <memory>

#include "platform/PlatformUtils.h"

class KeyButton;
class KeyboardButton;

class Application;
class MainWindow;

class BTNImages {
public:
    BTNImages(QString _strHeight, int _width, int _height);

    QString m_strHeight;
    int m_width;
    int m_height;

    QImage m_btnLeftPassive;
    QImage m_btnCenterPassive;
    QImage m_btnRightPassive;
    QImage m_btnLeftActive;
    QImage m_btnCenterActive;
    QImage m_btnRightActive;

private:
    QString m_strLeftPassive;
    QString m_strCenterPassive;
    QString m_strRightPassive;
    QString m_strLeftActive;
    QString m_strCenterActive;
    QString m_strRightActive;
};

class ContentImage {
public:
    ContentImage(QString strHeight, int m_height, QString strContentPath);

    QString m_strHeight;
    int m_height;

    QImage m_btnContent;

private:
    QString m_strContent;
};

class KeyboardPalette : public ActionPalette {
    Q_OBJECT

    friend class KeyboardButton;
    friend class CapsLockButton;
    friend class ShiftButton;
    friend class LocaleButton;
    friend class KeyButton;

public:
    KeyboardPalette(QWidget *parent);
    ~KeyboardPalette();

    bool isEnabled();
    virtual QSize sizeHint() const;
    virtual void adjustSizeAndPosition(bool pUp = true);
    QString getKeyButtonSize() const;
    void setKeyButtonSize(const QString &strSize);

    bool keyboardVisible() const;

signals:
    void moved(const QPoint &);
    void localeChanged(int);
    void keyboardActivated(bool);

private slots:
    void syncPosition(const QPoint &pos);
    void syncLocale(int nLocale);
    void keyboardPaletteButtonSizeChanged(QVariant size);
    void onActivated(bool b);

public slots:
    // 由 BoardPaletteManager / WebController 连接
    void onDeactivated();
    void showKeyboard(bool show);
    void hideKeyboard();

protected:
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);
    virtual void paintEvent(QPaintEvent *event);
    virtual void moveEvent(QMoveEvent *event);

    void sendKeyEvent(KEYCODE keyCode);

    void setLocale(int nLocale);

    const QString *getLocaleName();

    void init();

private:
    void checkLayout();

    void createCtrlButtons();

    // createCtrlButtons() 的平台实现（ui/KeyboardPalette_win.cpp）拿不到 Private 的定义，
    // 由它来分配并挂住 d->ctrlButtons。
    KeyboardButton **createCtrlButtonArray(int count);

    void setInput(const KeyboardLocale *locale);

    // Can be redefined under each platform
    void onLocaleChanged(KeyboardLocale *locale);

    struct Private;
    std::unique_ptr<Private> d;
};

class KeyboardButton : public QWidget {
    Q_OBJECT

public:
    KeyboardButton(KeyboardPalette *parent, QString contentImagePath);
    ~KeyboardButton();

protected:
    void paintEvent(QPaintEvent *event);

    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual void onPress() = 0;
    virtual void onRelease() = 0;
    virtual void paintContent(QPainter &painter) = 0;

    virtual bool isPressed();

    KeyboardPalette *keyboardPalette();
    ContentImage *contentImage() const;

    void sendUnicodeSymbol(KEYCODE keycode);
    void sendControlSymbol(int nSymbol);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class KeyButton : public KeyboardButton {
    Q_OBJECT

public:
    KeyButton(KeyboardPalette *parent);
    ~KeyButton();

    void setKeyBt(const KEYBT *keybt);

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter &painter);

private:
    bool shifted();

    struct Private;
    std::unique_ptr<Private> d;
};

class CntrlButton : public KeyboardButton {
    Q_OBJECT

public:
    CntrlButton(KeyboardPalette *parent, int _code, const QString &_contentImagePath);
    CntrlButton(KeyboardPalette *parent, const QString &_label, int _code);
    ~CntrlButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter &painter);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class CapsLockButton : public KeyboardButton {
    Q_OBJECT

public:
    CapsLockButton(KeyboardPalette *parent, const QString _contentImagePath);
    ~CapsLockButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter &painter);

protected:
    virtual bool isPressed();
};

class ShiftButton : public KeyboardButton {
    Q_OBJECT

public:
    ShiftButton(KeyboardPalette *parent, const QString _contentImagePath);
    ~ShiftButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter &painter);

protected:
    virtual bool isPressed();
};

class LocaleButton : public KeyboardButton {
    Q_OBJECT

public:
    LocaleButton(KeyboardPalette *parent);
    ~LocaleButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter &painter);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
