/**
 * @file KeyboardSender.h
 * @brief 声明把按键注入当前焦点窗口的 Win32 平台接口。
 */

#pragma once

struct KEYCODE;

/**
 * @brief 把一次按键交给系统，使其落到当前焦点窗口上。
 *
 * 位于 Win32 平台层：屏幕键盘只负责界面，按键注入由本类完成。
 * KEYCODE 的字段含义见 platform/PlatformUtils.h；两个静态方法均无返回值，
 * 注入失败时静默。调用方必须保证目标窗口已经取得前台焦点。
 */
class KeyboardSender
{
public:
    /** @brief 屏幕键盘需要注入的常用虚拟键码；数字属于平台，不散落在界面代码里。 */
    enum VirtualKey
    {
        Backspace = 0x08, ///< 退格键。
        Tab       = 0x09, ///< 制表键。
        Enter     = 0x0d, ///< 回车键。
        Space     = 0x20  ///< 空格键。
    };

    /** @brief 按 KEYCODE 中的符号与修饰键注入一次按键。 */
    static void sendUnicodeKey(const KEYCODE &keycode);
    /** @brief 直接注入一个虚拟键码（按下并抬起）。 */
    static void sendVirtualKey(int nativeVirtualKey);
};
