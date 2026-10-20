#include "utils/StringUtils.h"
#include <algorithm>
#include <QRegExp>

QStringList StringUtils::sortByLastDigit(const QStringList &sourceList)
{
    // 匹配“非数字 + 数字 + 点”，取最后一处，即文件名里的序号
    QRegExp rx("\\D(\\d+)\\.");

    QMultiMap<int, QString> elements;

    foreach (QString source, sourceList)
    {
        int pos   = rx.lastIndexIn(source);

        // 没有序号的文件归入 -1 组，排在最前
        int digit = -1;

        if (pos >= 0)
        {
            digit = rx.cap(1).toInt();
        }

        elements.insert(digit, source);
    }

    QStringList result;

    // 按序号升序；同序号内部再按字符串排序，保证结果稳定
    QList<int> keys = elements.keys();
    std::sort(keys.begin(), keys.end());

    foreach (int key, keys)
    {
        QList<QString> values = elements.values(key);
        std::sort(values.begin(), values.end());
        foreach (QString val, values)
        {
            if (!result.contains(val))
            {
                result << val;
            }
        }
    }

    return result;
}

QString StringUtils::netxDigitizedName(const QString &source)
{
    // 匹配末尾的“非数字 + 数字”段，取最后一处
    QRegExp rx("\\D(\\d+)");

    int pos   = rx.lastIndexIn(source);

    int digit = -1;

    if (pos >= 0)
    {
        digit = rx.cap(1).toInt();
    }

    QString ret(source);

    if (digit == -1)
    {
        // 末尾没有数字：直接追加 “ 1”
        return ret + " 1";
    }
    else
    {
        QString s("%1");
        s = s.arg(digit + 1);
        return ret.replace(rx.cap(1), s);
    }
}

QString StringUtils::toCanonicalUuid(const QUuid &uuid)
{
    QString s = uuid.toString();

    // QUuid::toString() 会带花括号，规范形式要求去掉
    if (s.startsWith("{"))
    {
        s = s.right(s.length() - 1);
    }

    if (s.endsWith("}"))
    {
        s = s.left(s.length() - 1);
    }

    return s;
}

QString StringUtils::toUtcIsoDateTime(const QDateTime &dateTime)
{
    // Qt 6 对 UTC 时间的 ISO 输出已自带 "Z"；再手工加后缀会得到 "...09ZZ"
    return dateTime.toUTC().toString(Qt::ISODate);
}

QDateTime StringUtils::fromUtcIsoDate(const QString &dateString)
{
    QDateTime date = QDateTime::fromString(dateString, Qt::ISODate);
    // 文档存的是 UTC 文本，先按 UTC 解释再转本地时间
    date.setTimeZone(QTimeZone::UTC);
    return date.toLocalTime();
}
