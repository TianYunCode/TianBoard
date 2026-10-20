#pragma once

struct KEYCODE;

// 把一次按键交给系统，让它落到当前焦点窗口上——屏幕键盘自己只懂界面，
// 注入这件事是平台的。KEYCODE 的字段含义见 platform/PlatformUtils.h。
class KeyboardSender {
public:
    // 屏幕键盘需要注入的那几个虚拟键码。数字属于平台，不该散在界面代码里。
    enum VirtualKey { Backspace = 0x08, Tab = 0x09, Enter = 0x0d, Space = 0x20 };

    static void sendUnicodeKey(const KEYCODE &keycode);
    static void sendVirtualKey(int nativeVirtualKey);
};
