#pragma once

#include <QString>

class TextTools {
public:
    TextTools() {}
    virtual ~TextTools() {}

    static QString cleanHtmlCData(const QString &_html);
    static QString cleanHtml(const QString &_html);
};
