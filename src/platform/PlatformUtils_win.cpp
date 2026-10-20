#include "platform/PlatformUtils.h"

#include <QtWidgets>
#include <QProcess>
#include <qt_windows.h>
#include <shellapi.h>

#include "utils/FileSystemUtils.h"

void PlatformUtils::init() {
    initializeKeyboardLayouts();
}

QStringList PlatformUtils::availableTranslations() {
    // 翻译文件随程序部署在 Language/ 下，文件名格式为 <前缀><语言>.qm
    QString translationsPath = FileSystemUtils::applicationResourcesDirectory() + "/" + "Language" + "/";
    QStringList translationsList = FileSystemUtils::allFiles(translationsPath);

    QStringList languages;

    foreach (const QString &translationFile, translationsList) {
        QFileInfo fileInfo(translationFile);

        // 只认 .qm，忽略目录里的其他文件
        if (fileInfo.suffix().compare("qm", Qt::CaseInsensitive) != 0)
            continue;

        const QString prefix = QString("TianBoard_");

        if (fileInfo.fileName().startsWith(prefix))
            languages << fileInfo.completeBaseName().mid(prefix.length());
    }

    return languages;
}

QString PlatformUtils::translationPath(QString pFilePrefix, QString pLanguage) {
    QString qmPath = FileSystemUtils::applicationResourcesDirectory() + "/" + "Language" + "/" + pFilePrefix +
                     pLanguage + ".qm";
    return qmPath;
}

QString PlatformUtils::systemLanguage() {
    return QLocale::system().name();
}

void PlatformUtils::bringPreviousProcessToFront() {
    // Windows 下由单实例消息机制处理，这里无需动作
}


QString PlatformUtils::osUserLoginName() {
    // GetUserName 的缓冲区长度按字符计；返回前已把长度换算成不含结尾 NUL 的字符数
    WCHAR winUserName[256 + 1];
    DWORD winUserNameSize = sizeof(winUserName);
    GetUserName(winUserName, &winUserNameSize);
    QString userName = QString::fromWCharArray(winUserName, winUserNameSize - 1);

    return userName;
}


QString PlatformUtils::computerName() {
    // 同样按字符计长度，winComputerNameSize 返回时不含结尾 NUL
    WCHAR winComputerName[256 + 1];
    DWORD winComputerNameSize = sizeof(winComputerName);
    GetComputerName(winComputerName, &winComputerNameSize);
    QString computerName = QString::fromWCharArray(winComputerName, winComputerNameSize - 1);

    return computerName;
}


void PlatformUtils::setDesktopMode(bool desktop) {
    Q_UNUSED(desktop);
}

void PlatformUtils::setWindowNonActivableFlag(QWidget *widget, bool nonAcivable) {}

// Qt6 起 QChar 的构造函数是 explicit，宏里必须显式包一层 QChar()。
#define KEYBTDECL(s1, s2, clSwitch) KEYBT(QChar(s1), QChar(s2), clSwitch, 0, 0, KEYCODE(s1), KEYCODE(s2))

KEYBT ENGLISH_LOCALE[] = {
    /* ` ~ */ KEYBTDECL(0x60, 0x7e, false),
    /* 1 ! */ KEYBTDECL(0x31, 0x21, false),
    /* 2 @ */ KEYBTDECL(0x32, 0x40, false),
    /* 3 # */ KEYBTDECL(0x33, 0x23, false),
    /* 4 $ */ KEYBTDECL(0x34, 0x24, false),
    /* 5 % */ KEYBTDECL(0x35, 0x25, false),
    /* 6 ^ */ KEYBTDECL(0x36, 0x5e, false),
    /* 7 & */ KEYBTDECL(0x37, 0x26, false),
    /* 8 * */ KEYBTDECL(0x38, 0x2a, false),
    /* 9 ( */ KEYBTDECL(0x39, 0x28, false),
    /* 0 ) */ KEYBTDECL(0x30, 0x29, false),
    /* - _ */ KEYBTDECL(0x2d, 0x5f, false),
    /* = + */ KEYBTDECL(0x3d, 0x2b, false),

    /* q Q */ KEYBTDECL(0x71, 0x51, true),
    /* w W */ KEYBTDECL(0x77, 0x57, true),
    /* e E */ KEYBTDECL(0x65, 0x45, true),
    /* r R */ KEYBTDECL(0x72, 0x52, true),
    /* t T */ KEYBTDECL(0x74, 0x54, true),
    /* y Y */ KEYBTDECL(0x79, 0x59, true),
    /* u U */ KEYBTDECL(0x75, 0x55, true),
    /* i I */ KEYBTDECL(0x69, 0x49, true),
    /* o O */ KEYBTDECL(0x6f, 0x4f, true),
    /* p P */ KEYBTDECL(0x70, 0x50, true),
    /* [ { */ KEYBTDECL(0x5b, 0x7b, false),
    /* ] } */ KEYBTDECL(0x5d, 0x7d, false),

    /* a A */ KEYBTDECL(0x61, 0x41, true),
    /* s S */ KEYBTDECL(0x73, 0x53, true),
    /* d D */ KEYBTDECL(0x64, 0x44, true),
    /* f F */ KEYBTDECL(0x66, 0x46, true),
    /* g G */ KEYBTDECL(0x67, 0x47, true),
    /* h H */ KEYBTDECL(0x68, 0x48, true),
    /* j J */ KEYBTDECL(0x6a, 0x4a, true),
    /* k K */ KEYBTDECL(0x6b, 0x4b, true),
    /* l L */ KEYBTDECL(0x6c, 0x4c, true),
    /* ; : */ KEYBTDECL(0x3b, 0x3a, false),
    /* ' " */ KEYBTDECL(0x27, 0x22, false),
    /* \ | */ KEYBTDECL(0x5c, 0x7c, false),

    /* z Z */ KEYBTDECL(0x7a, 0x5a, true),
    /* x X */ KEYBTDECL(0x78, 0x58, true),
    /* c C */ KEYBTDECL(0x63, 0x43, true),
    /* v V */ KEYBTDECL(0x76, 0x56, true),
    /* b B */ KEYBTDECL(0x62, 0x42, true),
    /* n N */ KEYBTDECL(0x6e, 0x4e, true),
    /* m M */ KEYBTDECL(0x6d, 0x4d, true),
    /* , < */ KEYBTDECL(0x2c, 0x3c, false),
    /* . > */ KEYBTDECL(0x2e, 0x3e, false),
    /* / ? */ KEYBTDECL(0x2f, 0x5f, false)};


void PlatformUtils::initializeKeyboardLayouts() {
    nKeyboardLayouts = 1;
    keyboardLayouts = new KeyboardLocale *[nKeyboardLayouts];
    keyboardLayouts[0] =
        new KeyboardLocale(tr("English"), "en", "", new QIcon(":/images/flags/en.png"), ENGLISH_LOCALE);
}

void PlatformUtils::destroyKeyboardLayouts() {
    for (int i = 0; i < nKeyboardLayouts; i++)
        delete keyboardLayouts[i];
    delete[] keyboardLayouts;
    keyboardLayouts = NULL;
}


void PlatformUtils::setFrontProcess() {
    // Windows 下窗口激活由系统策略决定，这里无需动作
}


void PlatformUtils::showFullScreen(QWidget *pWidget) {
    pWidget->showFullScreen();
}

void PlatformUtils::showOSK(bool show) {
    if (show) {
        QString windir = qgetenv("WINDIR");
        QString osk_path = windir + "\\System32\\osk.exe";

        QProcess oskProcess;
        // osk.exe 需要管理员权限才能直接启动，借 explorer.exe 代为拉起
        oskProcess.startDetached("explorer.exe", QStringList() << osk_path);
    }

    else {
        // 关闭时按窗口类名找到屏幕键盘，发送系统关闭命令
        HWND oskWindow = ::FindWindow(TEXT("OSKMainClass"), NULL);
        if (oskWindow)
            PostMessage(oskWindow, WM_SYSCOMMAND, SC_CLOSE, 0);
    }
}
