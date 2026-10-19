#include "utils/Version.h"

#include <QRegularExpression>
#include <QStringList>
#include <QtGlobal>

Version::Version(const QString &string) : mString(string) {}

Version::~Version() {
    // NOOP
}

uint Version::toUInt() const {
    const QStringList parts = mString.split(QRegularExpression(QStringLiteral("[-.]")));

    uint major = 0;
    uint minor = 0;
    uint patch = 0;
    uint revision = 0;
    uint stage = 0;

    switch (parts.count()) {
        case 2: // "1.0" 是 "1.0.0" 的简写
            major = parts.at(0).toUInt();
            minor = parts.at(1).toUInt();
            stage = static_cast<uint>(Stage::Release);
            break;
        case 3: // "1.0.0"
            major = parts.at(0).toUInt();
            minor = parts.at(1).toUInt();
            patch = parts.at(2).toUInt();
            stage = static_cast<uint>(Stage::Release);
            break;
        case 5: // "1.0.0.a.3" / "1.0.0.b.3" / "1.0.0.rc.3"
            major = parts.at(0).toUInt();
            minor = parts.at(1).toUInt();
            patch = parts.at(2).toUInt();
            stage = parts.at(3).startsWith(QLatin1Char('a'))   ? static_cast<uint>(Stage::Alpha)
                    : parts.at(3).startsWith(QLatin1Char('b')) ? static_cast<uint>(Stage::Beta)
                                                               : static_cast<uint>(Stage::ReleaseCandidate);
            revision = parts.at(4).toUInt();
            break;
        default: // 不是读写器会写出的串：当“没有可用版本”，而不是猜一个
            qWarning() << "Unknown version format:" << mString;
            return 0;
    }

    // 各位的权重留足余量：minor 每 +1 要跨 1e6，patch 最大 99，stage*100 + revision。
    return major * 100000000U + minor * 1000000U + patch * 10000U + stage * 100U + revision;
}

bool Version::operator<(const Version &otherVersion) const {
    return toUInt() < otherVersion.toUInt();
}

bool Version::operator<=(const Version &otherVersion) const {
    return toUInt() <= otherVersion.toUInt();
}

bool Version::operator==(const Version &otherVersion) const {
    return toUInt() == otherVersion.toUInt();
}

bool Version::operator>(const Version &otherVersion) const {
    return toUInt() > otherVersion.toUInt();
}
