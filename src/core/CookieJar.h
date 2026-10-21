/**
 * @file CookieJar.h
 * @brief 应用共享 Cookie 罐的声明：把内嵌浏览器的 Cookie 变成可枚举的一份镜像。
 */

#pragma once

#include <QNetworkCookie>
#include <QNetworkCookieJar>

/**
 * @brief 装在 NetworkAccessManager 上的 Cookie 罐，只多做一件事：允许枚举内部列表。
 *
 * 详细说明：QWebEngineCookieStore 没有“列出当前所有 Cookie”的接口，而“退出时清掉白名单
 * 以外的 Cookie”必须逐个拿到它们。做法是把 WebEngine 的 cookieAdded/cookieRemoved 同步进
 * 本罐（连线在 web/WebController），于是全应用可见的 Cookie 都有一份可枚举的镜像；
 * QNetworkCookieJar::allCookies() 是 protected，这里转一层公开出来。顺带的效果是应用自己
 * 发的网络请求与内嵌浏览器共享同一个会话。
 * 生命周期：由 NetworkAccessManager 在构造时 setCookieJar 装上并接管所有权，随它一起销毁。
 * 线程约束：GUI 线程专用（WebEngine 的 Cookie 信号本来就在 GUI 线程派发）。
 */
class CookieJar : public QNetworkCookieJar
{
    Q_OBJECT

public:
    /// @brief 构造 Cookie 罐；父对象通常是 NetworkAccessManager。
    explicit CookieJar(QObject *parent = nullptr);

    /// @brief 返回当前存着的全部 Cookie，供退出时按域过滤删除。
    QList<QNetworkCookie> storedCookies() const;
};
