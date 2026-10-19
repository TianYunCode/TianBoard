#include "utils/Version.h"
#include <QRegularExpression>

#include <QtCore>
#include <QStringList>


Version::Version(const QString &string)
{
    mString = string;
}

uint Version::toUInt() const
{
    uint result = 0;
    QStringList list = mString.split(QRegularExpression("[-.]"));
    switch (list.count()) {
    case 2:
        //short version  1.0
        result = (list.at(0).toUInt() * 100000000) + (list.at(1).toUInt() * 1000000) + (Release * 100);
        break;
    case 3:
        //release version 1.0.0
        result = (list.at(0).toUInt() * 100000000) + (list.at(1).toUInt() * 1000000) + list.at(2).toUInt()*10000 + (Release * 100);
        break;
    case 5:{
        //standard version  1.0.0.a/b/rc.0
        uint releaseStage = list.at(3).startsWith("a") ? Alpha :(list.at(3).startsWith("b") ? Beta : ReleaseCandidate);
        result = (list.at(0).toUInt() * 100000000) + (list.at(1).toUInt() * 1000000) + (list.at(2).toUInt() * 10000) + (releaseStage * 100) + list.at(4).toUInt();
        break;
    }
    default:
        qWarning() << "Unknown version format.";
        break;
    }
    return result;
}

Version::~Version()
{
    // NOOP
}

bool Version::operator < (const Version &otherVersion) const
{
    return toUInt() < otherVersion.toUInt();
}

bool Version::operator == (const Version &otherVersion) const
{
    return toUInt() == otherVersion.toUInt();
}

bool Version::operator > (const Version &otherVersion) const
{
    return toUInt() > otherVersion.toUInt();
}
