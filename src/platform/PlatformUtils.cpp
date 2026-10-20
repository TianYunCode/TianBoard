#include "platform/PlatformUtils.h"

void PlatformUtils::destroy()
{
    destroyKeyboardLayouts();
}

PlatformUtils::PlatformUtils()
{
    // 刻意留空：纯静态入口，不应实例化
}

PlatformUtils::~PlatformUtils()
{
    // 刻意留空
}

bool PlatformUtils::hasVirtualKeyboard()
{
    // 布局表非空即表示平台提供了虚拟键盘定义
    return keyboardLayouts != NULL && nKeyboardLayouts != 0;
}

KeyboardLocale::~KeyboardLocale()
{
    // 只有动态分配的指针数组需要逐个释放；静态数组归编译期常量
    if (varSymbols != NULL)
    {
        for (int i = 0; i < SYMBOL_KEYS_COUNT; i++)
        {
            delete varSymbols[i];
        }
        delete[] varSymbols;
    }
    delete icon;
}

int              PlatformUtils::nKeyboardLayouts;
KeyboardLocale **PlatformUtils::keyboardLayouts;

KeyboardLocale **PlatformUtils::getKeyboardLayouts(int &nCount)
{
    nCount = nKeyboardLayouts;
    return keyboardLayouts;
}
