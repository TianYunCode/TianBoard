#pragma once

#include <QtCore>

class StringUtils
{
private:
    StringUtils() {}
    ~StringUtils() {}

public:
    static QStringList sortByLastDigit(const QStringList& source);

    static QString netxDigitizedName(const QString& source);

    static QString toCanonicalUuid(const QUuid& uuid);

    static QString toUtcIsoDateTime(const QDateTime& dateTime);
    static QDateTime fromUtcIsoDate(const QString& dateString);

};

