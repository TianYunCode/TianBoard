/**
 * @file KeyboardPalette.h
 * @brief 提供屏幕虚拟键盘调色板及其各类按键控件。
 */

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


/**
 * @brief 某一按键尺寸下的一套九宫格按键贴图。
 *
 * 左右两端与中段各有按下/未按下两张图，KeyboardButton 按控件宽度拉伸中段拼出
 * 完整按键。KeyboardPalette 在按键尺寸变化时重建本对象，由调色板独占持有。
 */
class BTNImages {
public:
    /**
     * @brief 按尺寸加载对应资源目录下的按键贴图。
     * @param _strHeight 资源目录名（如 "16"），同时用于拼接图片路径
     * @param _width 按键宽度
     * @param _height 按键高度
     */
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

/**
 * @brief 按键上叠加的图标图片（退格、Tab、Shift 等）。
 *
 * 路径由按键尺寸目录与内容名拼接而成；KeyboardButton 在尺寸变化时按需重建。
 */
class ContentImage {
public:
    /**
     * @brief 加载指定尺寸的内容图标。
     * @param strHeight 资源目录名
     * @param m_height 目标高度
     * @param strContentPath 内容名（不含扩展名）
     */
    ContentImage(QString strHeight, int m_height, QString strContentPath);

    QString m_strHeight;
    int m_height;

    QImage m_btnContent;

private:
    QString m_strContent;
};

/**
 * @brief 屏幕虚拟键盘调色板。
 *
 * 界面是 15 列 5 行的按键网格，另含退格、Tab、回车、Caps、Shift、输入法切换、
 * 空格等控制键。BoardPaletteManager 在平台支持虚拟键盘时创建（无父控件，作为
 * 不可激活的置顶窗口），由主窗口的“虚拟键盘”动作控制显隐；按键通过
 * KeyboardSender 发送到当前前台应用。按键布局与尺寸随设置中的键帽大小重建。
 */
class KeyboardPalette : public ActionPalette {
    Q_OBJECT

    friend class KeyboardButton;
    friend class CapsLockButton;
    friend class ShiftButton;
    friend class LocaleButton;
    friend class KeyButton;

public:
    /**
     * @brief 构造键盘：创建 47 个字符键与 9 个控制键，并载入当前输入法布局。
     * @param parent 父控件；通常为空
     */
    KeyboardPalette(QWidget *parent);
    ~KeyboardPalette();

    /** @brief 是否成功取得系统键盘布局；false 时键盘不可用。 */
    bool isEnabled();
    virtual QSize sizeHint() const;
    /** @brief 按键帽尺寸计算窗口大小，并以中心/底边为基准重新定位。 */
    virtual void adjustSizeAndPosition(bool pUp = true);
    /** @brief 当前键帽尺寸，格式为“宽x高”。 */
    QString getKeyButtonSize() const;
    /**
     * @brief 按“宽x高”字符串重建键帽尺寸与贴图。
     * @param strSize 形如 "36x36" 的尺寸串，格式不符时忽略
     */
    void setKeyButtonSize(const QString &strSize);

    /** @brief 逻辑上是否处于显示状态（由 showKeyboard() 维护）。 */
    bool keyboardVisible() const;

signals:
    /** @brief 键盘窗口被移动。 */
    void moved(const QPoint &);
    /** @brief 输入法布局切换到指定下标。 */
    void localeChanged(int);
    /** @brief 鼠标进入/离开键盘，参数表示是否激活。 */
    void keyboardActivated(bool);

private slots:
    /**
     * @brief 设置中的键帽尺寸变化时同步键盘。
     * @param size 新尺寸串
     */
    void keyboardPaletteButtonSizeChanged(QVariant size);

public slots:
    /**
     * @brief 记录显示状态。
     * @param show 新状态
     * @note 不直接调用 show()/hide()，实际显隐由 QAction 的 toggled 驱动。
     */
    void showKeyboard(bool show);
    /** @brief 通过触发“虚拟键盘”动作来关闭键盘。 */
    void hideKeyboard();

protected:
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);
    virtual void paintEvent(QPaintEvent *event);
    virtual void moveEvent(QMoveEvent *event);

    /**
     * @brief 切换输入法布局并保存到设置。
     * @param nLocale 布局下标
     */
    void setLocale(int nLocale);

    /** @brief 当前输入法名称，无布局时为 nullptr。 */
    const QString *getLocaleName();

    /** @brief 读取设置、连接动作并设置窗口不可激活。 */
    void init();

private:
    /** @brief 创建 9 个控制键。 */
    void createCtrlButtons();

    /**
     * @brief 用给定布局填充 47 个字符键。
     * @param locale 键盘布局；为空时隐藏键盘
     */
    void setInput(const KeyboardLocale *locale);

    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 虚拟键盘所有按键的基类：绘制按键底图、跟踪悬停与按下状态。
 *
 * 子类实现 onPress()/onRelease()/paintContent() 决定按下时发送什么键、键面画什么。
 * 按键由 KeyboardPalette 创建并 setGeometry() 定位，父控件是调色板。
 */
class KeyboardButton : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造按键。
     * @param parent 所属键盘
     * @param contentImagePath 键面图标名，空表示只画文字
     */
    KeyboardButton(KeyboardPalette *parent, QString contentImagePath);
    ~KeyboardButton();

protected:
    void paintEvent(QPaintEvent *event);

    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    /** @brief 按下时执行的动作。 */
    virtual void onPress() = 0;
    /** @brief 松开时执行的动作。 */
    virtual void onRelease() = 0;
    /** @brief 在按键底图上绘制键面内容。 */
    virtual void paintContent(QPainter &painter) = 0;

    /** @brief 是否以按下态绘制；默认取实际按压状态，子类可改为锁定状态。 */
    virtual bool isPressed();

    /** @brief 所属键盘。 */
    KeyboardPalette *keyboardPalette();
    /** @brief 当前键面图标，尚未加载时为 nullptr。 */
    ContentImage *contentImage() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 字符键：按 Shift/Caps 状态选择符号并发送 Unicode 字符。
 *
 * 部分键还带特殊修饰符（modifier1/modifier2），按下后先记住修饰符，等下一个键
 * 一起发送，行为类似 AltGr 组合键。
 */
class KeyButton : public KeyboardButton {
    Q_OBJECT

public:
    /**
     * @brief 构造尚未绑定布局数据的字符键。
     * @param parent 所属键盘
     */
    KeyButton(KeyboardPalette *parent);
    ~KeyButton();

    /**
     * @brief 绑定布局中的按键描述。
     * @param keybt 指向键盘布局常量数据的指针，布局对象需长于本键
     */
    void setKeyBt(const KEYBT *keybt);

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter &painter);

private:
    /** @brief 当前是否应输出上档字符（综合 Shift 与 Caps）。 */
    bool shifted();

    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 控制键：发送固定虚拟键码，键面显示标签或图标。
 */
class CntrlButton : public KeyboardButton {
    Q_OBJECT

public:
    /**
     * @brief 用图标构造控制键。
     * @param parent 所属键盘
     * @param _code 虚拟键码
     * @param _contentImagePath 图标名
     */
    CntrlButton(KeyboardPalette *parent, int _code, const QString &_contentImagePath);
    /**
     * @brief 用文字构造控制键。
     * @param parent 所属键盘
     * @param _label 键面文字
     * @param _code 虚拟键码
     */
    CntrlButton(KeyboardPalette *parent, const QString &_label, int _code);
    ~CntrlButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter &painter);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief Caps Lock 键：切换键盘的 capsLock 标志，自身以锁定态显示。
 */
class CapsLockButton : public KeyboardButton {
    Q_OBJECT

public:
    /**
     * @brief 构造 Caps Lock 键。
     * @param parent 所属键盘
     * @param _contentImagePath 图标名
     */
    CapsLockButton(KeyboardPalette *parent, const QString _contentImagePath);
    ~CapsLockButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter &painter);

protected:
    virtual bool isPressed();
};

/**
 * @brief Shift 键：切换键盘的 shift 标志，自身以按下态显示。
 */
class ShiftButton : public KeyboardButton {
    Q_OBJECT

public:
    /**
     * @brief 构造 Shift 键。
     * @param parent 所属键盘
     * @param _contentImagePath 图标名
     */
    ShiftButton(KeyboardPalette *parent, const QString _contentImagePath);
    ~ShiftButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter &painter);

protected:
    virtual bool isPressed();
};

/**
 * @brief 输入法切换键：松开时弹出布局菜单并切换键盘布局。
 *
 * 菜单弹出期间会把键盘标记为“语言菜单激活”，避免鼠标离开窗口时误触发隐藏逻辑。
 */
class LocaleButton : public KeyboardButton {
    Q_OBJECT

public:
    /**
     * @brief 构造按键并按平台布局列表填充菜单。
     * @param parent 所属键盘
     */
    LocaleButton(KeyboardPalette *parent);
    ~LocaleButton();

    virtual void onPress();
    virtual void onRelease();
    virtual void paintContent(QPainter &painter);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
