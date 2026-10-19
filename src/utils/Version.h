#pragma once

#include <QString>

enum ReleaseStage { Alpha = 10, Beta = 11, ReleaseCandidate = 15, Release = 17 };

class Version
{
public:
    Version(const QString &string);
    virtual ~Version();

    uint toUInt() const;

    bool operator < (const Version &otherVersion) const;
    bool operator == (const Version &otherVersion) const;
    bool operator > (const Version &otherVersion) const;

private:
    QString mString;
};

