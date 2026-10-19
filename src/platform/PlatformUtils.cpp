#include "platform/PlatformUtils.h"


void PlatformUtils::destroy() {
    destroyKeyboardLayouts();
}

PlatformUtils::PlatformUtils() {
    // NOOP
}

PlatformUtils::~PlatformUtils() {
    // NOOP
}

bool PlatformUtils::hasVirtualKeyboard() {
    return keyboardLayouts != NULL && nKeyboardLayouts != 0;
}


KeyboardLocale::~KeyboardLocale() {
    if (varSymbols != NULL) {
        for (int i = 0; i < SYMBOL_KEYS_COUNT; i++)
            delete varSymbols[i];
        delete[] varSymbols;
    }
    delete icon;
}


int PlatformUtils::nKeyboardLayouts;
KeyboardLocale **PlatformUtils::keyboardLayouts;

KeyboardLocale **PlatformUtils::getKeyboardLayouts(int &nCount) {
    nCount = nKeyboardLayouts;
    return keyboardLayouts;
}
