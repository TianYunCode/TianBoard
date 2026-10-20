#pragma once

#include <QString>
#include <QFile>

#define DELETEPTR(ptr) \
    if (NULL != ptr) { \
        delete ptr; \
        ptr = NULL; \
    }

#define SET_STYLE_SHEET() \
    { \
        setAttribute(Qt::WA_StyledBackground, true); \
        QString style; \
        QFile f(":/style.qss"); \
        if (f.exists() && f.open(QIODevice::ReadOnly)) \
            style = QString(f.readAll()); \
        setStyleSheet(style); \
    }

#define WARNINGS_DISABLE __pragma(warning(push, 0));
#define WARNINGS_ENABLE __pragma(warning(pop));

#ifdef NO_THIRD_PARTY_WARNINGS
// disabling warning level to 0 and save old state
#define THIRD_PARTY_WARNINGS_DISABLE WARNINGS_DISABLE
#else
// just save old state (needs for not empty define)
#define THIRD_PARTY_WARNINGS_DISABLE __pragma(warning(push));
#endif //#ifdef NO_THIRD_PARTY_WARNINGS

// anyway on any OS
#define THIRD_PARTY_WARNINGS_ENABLE WARNINGS_ENABLE
