#include "platform/KeyboardSender.h"

#include <qt_windows.h>

#include "platform/PlatformUtils.h"

// 一次按键 = 按下 + 抬起，两个事件必须在同一次 SendInput 里交出去：分开两次
// 调用时，中间可能插进别的输入，焦点窗口会看到半次按键。

void KeyboardSender::sendUnicodeKey(const KEYCODE &keycode) {
    INPUT input[2]{};

    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wScan = static_cast<WORD>(keycode.symbol);
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;

    input[1] = input[0];
    input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

    ::SendInput(2, input, sizeof(input[0]));
}

void KeyboardSender::sendVirtualKey(int nativeVirtualKey) {
    INPUT input[2]{};

    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = static_cast<WORD>(nativeVirtualKey);

    input[1] = input[0];
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;

    ::SendInput(2, input, sizeof(input[0]));
}
