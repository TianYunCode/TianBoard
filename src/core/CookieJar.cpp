#include "core/CookieJar.h"

CookieJar::CookieJar(QObject *parent) : QNetworkCookieJar(parent)
{
}

QList<QNetworkCookie> CookieJar::storedCookies() const
{
    // allCookies() 在基类里是 protected；这是本类存在的唯一理由。
    return allCookies();
}
