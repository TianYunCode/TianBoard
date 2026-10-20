/**
 * @file Globals.h
 * @brief 声明套用全局样式表与临时压制第三方警告的宏。
 */

#pragma once

#include <QString>
#include <QFile>

/// 把 :/style.qss 读入并套到当前控件上；样式资源不存在时清空样式。
#define SET_STYLE_SHEET()                              \
    {                                                  \
        setAttribute(Qt::WA_StyledBackground, true);   \
        QString style;                                 \
        QFile   f(":/style.qss");                      \
        if (f.exists() && f.open(QIODevice::ReadOnly)) \
            style = QString(f.readAll());              \
        setStyleSheet(style);                          \
    }

/// 保存并临时关闭警告，配对使用。
#define WARNINGS_DISABLE __pragma(warning(push, 0));
/// 恢复此前保存的警告状态。
#define WARNINGS_ENABLE  __pragma(warning(pop));

#ifdef NO_THIRD_PARTY_WARNINGS
// 第三方头文件：把警告级别降到 0，同时保存原状态
#define THIRD_PARTY_WARNINGS_DISABLE WARNINGS_DISABLE
#else
// 只保存原状态（这里需要一个非空宏体）
#define THIRD_PARTY_WARNINGS_DISABLE __pragma(warning(push));
#endif // #ifdef NO_THIRD_PARTY_WARNINGS

// 无论哪种配置，恢复动作都一样
#define THIRD_PARTY_WARNINGS_ENABLE WARNINGS_ENABLE
