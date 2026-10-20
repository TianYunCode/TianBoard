/**
 * @file PlatformUtils.h
 * @brief 声明键盘布局数据、平台工具函数与虚拟键盘/桌面模式的 Win32 入口。
 */

#pragma once

#include <QtCore>
#include <QIcon>
#include <QProcess>

class QMainWindow;

/// 键盘布局表中每种语言的符号键数量。
#define SYMBOL_KEYS_COUNT 47

/**
 * @brief 一次按键的符号与修饰键描述。
 *
 * 纯数据：symbol 为字符码，code 为平台虚拟键码，modifier 为修饰键位掩码。
 * symbol 为 0 表示空按键。
 */
struct KEYCODE
{
    /** @brief 构造空按键。 */
    KEYCODE() : symbol(0), code(0), modifier(0)
    {
    }

    /** @brief 仅由符号构造按键。 */
    KEYCODE(int _symbol) : symbol(_symbol), code(0), modifier(0)
    {
    }

    /** @brief 由符号、虚拟键码与修饰键构造按键。 */
    KEYCODE(int _symbol, unsigned char _code, int _modifier) : symbol(_symbol), code(_code), modifier(_modifier)
    {
    }

    /** @brief 没有符号内容时返回 true。 */
    bool empty() const
    {
        return symbol == 0;
    }

    int           symbol;   ///< 字符码。
    unsigned char code;     ///< 平台虚拟键码。
    int           modifier; ///< 修饰键位掩码。
};

/**
 * @brief 键盘布局中的一个按键定义。
 *
 * 描述某个字符在普通与 Shift 状态下的显示符号，以及最多 8 种组合对应的
 * KEYCODE；capsLockSwitch 表示该键是否受大写锁定影响。
 */
struct KEYBT
{
    QChar   symbol1;        ///< 普通状态显示的字符。
    QChar   symbol2;        ///< Shift 状态显示的字符。
    bool    capsLockSwitch; ///< 是否随大写锁定切换。
    int     modifier1;      ///< 第一组修饰键。
    int     modifier2;      ///< 第二组修饰键。
    KEYCODE codes[8];       ///< 各种组合下的按键定义。

    /** @brief 用一组可选 KEYCODE 构造按键定义，未给出的位置保持空按键。 */
    KEYBT(QChar _symbol1, QChar _symbol2, bool _capsLockSwitch, int _modifier1, int _modifier2, KEYCODE c1 = 0, KEYCODE c2 = 0, KEYCODE c3 = 0,
        KEYCODE c4 = 0, KEYCODE c5 = 0, KEYCODE c6 = 0, KEYCODE c7 = 0, KEYCODE c8 = 0)
        : symbol1(_symbol1), symbol2(_symbol2), capsLockSwitch(_capsLockSwitch), modifier1(_modifier1), modifier2(_modifier2)
    {
        codes[0] = c1;
        codes[1] = c2;
        codes[2] = c3;
        codes[3] = c4;
        codes[4] = c5;
        codes[5] = c6;
        codes[6] = c7;
        codes[7] = c8;
    }

    /** @brief 平凡析构，按键表由静态数据持有。 */
    ~KEYBT()
    {
    }
};

/**
 * @brief 一种键盘语言的元数据与按键表。
 *
 * 按键表可能是静态数组（constSymbols）或按需分配的指针数组（varSymbols），
 * operator[] 负责统一两种形态；icon 与按键表的所有权归调用方，本类不释放。
 */
class KeyboardLocale
{
public:
    /** @brief 用指针数组形式的按键表构造。 */
    KeyboardLocale(const QString &_fullName, const QString &_name, const QString &_id, QIcon *_icon, KEYBT **_symbols)
        : fullName(_fullName), name(_name), id(_id), icon(_icon), constSymbols(NULL), varSymbols(_symbols)
    {
    }
    /** @brief 用静态数组形式的按键表构造。 */
    KeyboardLocale(const QString &_fullName, const QString &_name, const QString &_id, QIcon *_icon, KEYBT _symbols[])
        : fullName(_fullName), name(_name), id(_id), icon(_icon), constSymbols(_symbols), varSymbols(NULL)
    {
    }

    /** @brief 析构；是否释放按键表取决于具体平台实现。 */
    ~KeyboardLocale();

    const QString fullName; ///< 语言的完整名称。
    const QString name;     ///< 语言的短名称。
    const QString id;       ///< 语言的标识符。
    QIcon        *icon;     ///< 语言图标，不拥有所有权。
    /** @brief 按索引取按键定义，兼容静态数组与指针数组两种存储。 */
    KEYBT *operator[](int index) const
    {
        return (varSymbols == NULL) ? constSymbols + index : varSymbols[index];
    }

private:
    KEYBT  *constSymbols; ///< 静态按键表，非空时 varSymbols 为空。
    KEYBT **varSymbols;   ///< 动态按键表指针数组。
};

/**
 * @brief 平台相关工具函数的静态入口。
 *
 * 位于 Win32 平台层，向上提供语言、翻译路径、桌面模式、窗口前台化、屏幕键盘等
 * 能力。init()/destroy() 成对管理键盘布局静态数据；其余函数大多直接调用系统 API，
 * 失败时返回空值或静默，不抛异常。部分函数会改变窗口 Z 序或前台焦点，
 * 只能在 GUI 线程调用。
 */
class PlatformUtils
{
    Q_DECLARE_TR_FUNCTIONS(PlatformUtils)

private:
    /** @brief 私有构造，禁止实例化。 */
    PlatformUtils();
    /** @brief 私有析构。 */
    virtual ~PlatformUtils();

    static void             initializeKeyboardLayouts(); ///< 构建键盘布局表。
    static void             destroyKeyboardLayouts();    ///< 释放键盘布局表。
    static int              nKeyboardLayouts;            ///< 已加载的键盘布局数量。
    static KeyboardLocale **keyboardLayouts;             ///< 键盘布局表。

public:
    /** @brief 初始化平台层静态数据，进程内只需调用一次。 */
    static void init();
    /** @brief 释放平台层静态数据。 */
    static void destroy();
    /** @brief 拼接翻译文件路径；找不到时可能返回空路径。 */
    static QString translationPath(QString pFilePrefix, QString pLanguage);
    /** @brief 返回系统当前语言标识。 */
    static QString systemLanguage();
    /** @brief 当前平台是否提供虚拟键盘。 */
    static bool hasVirtualKeyboard();
    /** @brief 把先前启动的本程序实例切到前台。 */
    static void bringPreviousProcessToFront();
    /** @brief 返回当前登录用户名。 */
    static QString osUserLoginName();
    /** @brief 切换桌面模式（普通桌面 / 白板全屏）。 */
    static void setDesktopMode(bool desktop);
    /** @brief 设置或清除窗口的不可激活标志。 */
    static void setWindowNonActivableFlag(QWidget *widget, bool nonAcivable);
    /** @brief 返回计算机名。 */
    static QString computerName();
    /** @brief 返回键盘布局表并输出数量；返回的数组归平台层所有。 */
    static KeyboardLocale **getKeyboardLayouts(int &nCount);
    /** @brief 列出可用翻译文件对应的语言。 */
    static QStringList availableTranslations();
    /** @brief 把本进程窗口切到前台。 */
    static void setFrontProcess();
    /** @brief 以全屏方式显示窗口。 */
    static void showFullScreen(QWidget *pWidget);
    /** @brief 显示或隐藏系统屏幕键盘。 */
    static void showOSK(bool show);
};
