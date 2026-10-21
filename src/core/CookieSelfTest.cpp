#include "core/CookieSelfTest.h"

#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QNetworkCookie>
#include <QStringList>
#include <QVariant>

#include "core/Application.h"
#include "core/CookieJar.h"
#include "core/NetworkAccessManager.h"
#include "settings/Setting.h"
#include "settings/Settings.h"
#include "utils/CookieRules.h"
#include "web/WebController.h"

namespace
{
    /**
     * @brief 造一条只属于自检的 Cookie。
     * @param name 名字，带 selftest 前缀，绝不与真实站点写进来的同名。
     * @param domain 域；带点与不带点各造几条，用来验证白名单两种写法。
     * @note 用 .invalid 结尾的域：RFC 2606 保留顶级域，永远不会有真的站点叫这个。
     */
    QNetworkCookie testCookie(const QByteArray &name, const QString &domain)
    {
        QNetworkCookie cookie(name, "1");
        cookie.setDomain(domain);
        cookie.setPath("/");
        cookie.setExpirationDate(QDateTime::currentDateTimeUtc().addDays(300));
        return cookie;
    }

    /// @brief 白名单里那条“带点的域”，按规则应当同时盖住顶级域与子域。
    const QString kKeepDomain = QStringLiteral(".kept.selftest.invalid");
} // namespace

namespace CookieSelfTest
{
    int run()
    {
        QStringList failures;
        Settings   *settings = Settings::settings();

        CookieJar *jar       = qobject_cast<CookieJar *>(NetworkAccessManager::defaultAccessManager()->cookieJar());

        if (!jar)
        {
            failures << QStringLiteral("网络管理器的 Cookie 罐不是 CookieJar，退出清理没法枚举 Cookie");
        }

        if (!Application::webController)
        {
            failures << QStringLiteral("网页控制器还没装配，计数与清理的入口都是空的");
        }

        if (jar && Application::webController)
        {
            // 三项设置必须按注册的名字读得到：键名与配置文件对不上时读到的是空值，
            // 表现是“策略没生效”，界面上完全看不出来。
            const QString configuredPolicy = settings->webCookiePolicy->get().toString();

            if (configuredPolicy.isEmpty())
            {
                failures << QStringLiteral("Web/CookiePolicy 读不到值，检查配置键名与注册是否一致");
            }
            else if (CookieRules::policyName(CookieRules::policyFromName(configuredPolicy)) != configuredPolicy)
            {
                failures << QStringLiteral("配置里的策略名“%1”不是三种之一，会静默回落到默认策略").arg(configuredPolicy);
            }

            const QVariant previousAutoDelete   = settings->webCookieAutoDelete->get();
            const QVariant previousKeepDomains  = settings->webCookieKeepDomains->get();

            const QList<QNetworkCookie> planted = {testCookie("selftestApex", QStringLiteral("kept.selftest.invalid")),
                testCookie("selftestSub", QStringLiteral("cdn.kept.selftest.invalid")),
                testCookie("selftestOther", QStringLiteral("dropped.selftest.invalid"))};

            // 1) 自动清除关掉：一条都不该算进去。
            settings->webCookieAutoDelete->set(false);
            settings->webCookieKeepDomains->set(QStringList{kKeepDomain});

            if (Application::webController->cookiesMarkedForDeletion() != 0)
            {
                failures << QStringLiteral("自动清除关着却仍算出要清 Cookie");
            }

            // 2) 开着，记下种东西之前的两个基线（罐里可能已有本次会话的真实 Cookie）。
            settings->webCookieAutoDelete->set(true);

            const int baselineKept = Application::webController->cookiesMarkedForDeletion();

            settings->webCookieKeepDomains->set(QStringList());
            const int baselineAll = Application::webController->cookiesMarkedForDeletion();

            for (const QNetworkCookie &cookie : planted)
            {
                if (!jar->insertCookie(cookie))
                {
                    failures << QStringLiteral("自检 Cookie 没能写进罐子：%1").arg(QString::fromUtf8(cookie.name()));
                }
            }

            // 3) 白名单 ".kept.selftest.invalid" 要留下顶级域与子域两条，只把另一家的计入。
            settings->webCookieKeepDomains->set(QStringList{kKeepDomain});
            const int afterKept = Application::webController->cookiesMarkedForDeletion();

            if (afterKept - baselineKept != 1)
            {
                failures << QStringLiteral("带点的白名单多算了 %1 条（期望 1 条：只算不匹配的那条）").arg(afterKept - baselineKept);
            }

            // 4) 白名单空掉：三条都算，走“整罐清空”那条路时数字也要与预期一致。
            settings->webCookieKeepDomains->set(QStringList());

            if (Application::webController->cookiesMarkedForDeletion() - baselineAll != planted.size())
            {
                failures << QStringLiteral("空白名单下该清的条数不等于种进去的三条");
            }

            // 5) 撤掉自检 Cookie，计数必须回到基线：证明算的是同一个罐，没有留下残渣。
            for (const QNetworkCookie &cookie : planted)
            {
                jar->deleteCookie(cookie);
            }

            settings->webCookieKeepDomains->set(QStringList{kKeepDomain});

            if (Application::webController->cookiesMarkedForDeletion() != baselineKept)
            {
                failures << QStringLiteral("撤掉自检 Cookie 之后计数没回到基线");
            }

            settings->webCookieAutoDelete->set(previousAutoDelete);
            settings->webCookieKeepDomains->set(previousKeepDomains);
        }

        for (const QString &failure : failures)
        {
            qWarning() << "cookie-selftest:" << failure;
        }

        if (failures.isEmpty())
        {
            qInfo() << "cookie-selftest: OK";
        }

        return failures.size();
    }
} // namespace CookieSelfTest
