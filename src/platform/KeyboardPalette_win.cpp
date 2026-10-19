#include "platform/KeyboardPalette.h"

#include <qt_windows.h>

#include "core/Application.h"
#include "ui/MainWindow.h"


void KeyboardButton::sendUnicodeSymbol(KEYCODE keycode)
{
    INPUT input[2];
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = 0;
    input[0].ki.wScan = keycode.symbol;
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;
    input[0].ki.time = 0;
    input[0].ki.dwExtraInfo = 0;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = 0;
    input[1].ki.wScan = keycode.symbol;
    input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    input[1].ki.time = 0;
    input[1].ki.dwExtraInfo = 0;

    ::SendInput(2, input, sizeof(input[0]));
}

void KeyboardButton::sendControlSymbol(int nSymbol)
{
    INPUT input[2];
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = nSymbol;
    input[0].ki.wScan = 0;
    input[0].ki.dwFlags = 0;
    input[0].ki.time = 0;
    input[0].ki.dwExtraInfo = 0;

    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = nSymbol;
    input[1].ki.wScan = 0;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    input[1].ki.time = 0;
    input[1].ki.dwExtraInfo = 0;

    ::SendInput(2, input, sizeof(input[0]));
}

void KeyboardPalette::createCtrlButtons()
{
    int ctrlID = 0;
    KeyboardButton** ctrlButtons = createCtrlButtonArray(9);

    ctrlButtons[ctrlID++] = new CntrlButton(this, 0x08, "backspace");// Backspace
    ctrlButtons[ctrlID++] = new CntrlButton(this, 0x09, "tab");      // Tab
    ctrlButtons[ctrlID++] = new CapsLockButton(this, "capslock");    // Shift
    ctrlButtons[ctrlID++] = new CntrlButton(this, tr("Enter"), 0x0d);    // Enter
    ctrlButtons[ctrlID++] = new ShiftButton(this, "shift");    // Shift
    ctrlButtons[ctrlID++] = new ShiftButton(this, "shift");    // Shift
    ctrlButtons[ctrlID++] = new LocaleButton(this);                  // Language Switch 
    ctrlButtons[ctrlID++] = new CntrlButton(this, "", 0x20);         // Space
    ctrlButtons[ctrlID++] = new LocaleButton(this);                  // Language Switch 
}

void KeyboardPalette::checkLayout()
{}

void KeyboardPalette::onActivated(bool)
{}

void KeyboardPalette::onLocaleChanged(KeyboardLocale* )
{}

