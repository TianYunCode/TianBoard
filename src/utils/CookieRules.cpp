#include "utils/CookieRules.h"

#include <QRegularExpression>

#include <algorithm>

namespace
{
    /// @brief 一个策略与它在配置里的名字；两个方向都查这张表，名字就不会写岔。
    struct PolicyEntry
    {
        const char         *name;
        CookieRules::Policy policy;
    };

    const PolicyEntry kPolicies[] = {
        {"DenyAll",        CookieRules::Policy::DenyAll       },
        {"DenyThirdParty", CookieRules::Policy::DenyThirdParty},
        {"AcceptAll",      CookieRules::Policy::AcceptAll     },
    };
} // namespace

namespace CookieRules
{
    Policy policyFromName(const QString &name, Policy fallback)
    {
        for (const PolicyEntry &entry : kPolicies)
        {
            if (name == QLatin1String(entry.name))
            {
                return entry.policy;
            }
        }

        return fallback;
    }

    QString policyName(Policy policy)
    {
        for (const PolicyEntry &entry : kPolicies)
        {
            if (entry.policy == policy)
            {
                return QLatin1String(entry.name);
            }
        }

        return QLatin1String("DenyThirdParty");
    }

    bool isKeptDomain(const QString &domain, const QStringList &keepDomains)
    {
        const QString host = domain.trimmed().toLower();

        if (host.isEmpty())
        {
            return false;
        }

        for (const QString &raw : keepDomains)
        {
            const QString pattern = raw.trimmed().toLower();

            if (pattern.isEmpty())
            {
                continue;
            }

            if (!pattern.startsWith('.'))
            {
                if (host == pattern)
                {
                    return true;
                }
                continue;
            }

            // ".example.com" 要覆盖顶级域自己：浏览器给 example.com 写的 cookie
            // 域名字符串里并没有前置点，只比后缀会把顶级域漏掉。
            const QString base = pattern.mid(1);

            if (host == base || host.endsWith(QLatin1Char('.') + base))
            {
                return true;
            }
        }

        return false;
    }

    QStringList parseDomainList(const QString &text)
    {
        static const QRegularExpression separators(QStringLiteral("[;,]"));

        QStringList parts = text.split(separators, Qt::SkipEmptyParts);
        parts.erase(std::remove_if(parts.begin(), parts.end(),
                        [](QString &part)
                        {
                            part = part.trimmed();
                            return part.isEmpty();
                        }),
            parts.end());

        return parts;
    }
} // namespace CookieRules
