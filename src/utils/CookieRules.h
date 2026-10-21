/**
 * @file CookieRules.h
 * @brief Cookie 隐私规则的纯函数声明：策略取值与保留域匹配。
 */

#pragma once

#include <QString>
#include <QStringList>

/**
 * @brief 浏览器 Cookie 的隐私规则，只有字符串运算，不碰 QtWebEngine。
 *
 * 抽出来的理由：真正装过滤器、清 cookie 的代码要跑起整个应用才看得到结果，而这里两条
 * 规则（配置名到策略的映射、白名单怎么算命中）恰恰是最容易写错也最容易静默失效的部分——
 * 写错一处就是“用户以为只收第一方，其实全收了”或者“白名单里的站每次都掉登录”。
 * 因此 tests/tst_cookierules 只链接本文件单独跑。
 */
namespace CookieRules
{
    /// @brief 第一方之外的 Cookie 怎么处理，与 QWebEngineCookieStore 的过滤器一一对应。
    enum class Policy
    {
        DenyAll,        ///< 一律不收。
        DenyThirdParty, ///< 只收第一方，其它都拒。
        AcceptAll       ///< 全部收下。
    };

    /**
     * @brief 把配置里的策略名转成枚举。
     * @param name 配置值，取自 Web/CookiePolicy。
     * @param fallback 认不出来时的回落值；写成别的名字不会让策略失效。
     */
    Policy policyFromName(const QString &name, Policy fallback = Policy::DenyThirdParty);

    /// @brief 返回写进配置用的策略名，与 policyFromName 互逆。
    QString policyName(Policy policy);

    /**
     * @brief 判断某个 Cookie 域是否落在保留白名单里（大小写不敏感）。
     * @param domain Cookie 自己的域；浏览器给顶级域写的 cookie 形如 "example.com"，
     *               没有前置点。
     * @param keepDomains 白名单。以 '.' 开头的条目按后缀匹配且顶级域本身也算命中
     *                    （".example.com" 覆盖 example.com 与 a.example.com）；
     *                    其余条目要求整串相等（"example.com" 不覆盖子域）。
     * @note 退出网页模式时清 Cookie，命中的域保留、其余删除；空名单表示全清。
     */
    bool isKeptDomain(const QString &domain, const QStringList &keepDomains);

    /**
     * @brief 把用户在设置里填的一串域拆成白名单：逗号与分号都算分隔符，空白条目丢掉。
     * @note 写回配置时存的是 QStringList，QSettings 的 INI 形式按逗号落盘，
     *       所以这里再容忍分号只是为了让用户手填时不用记格式。
     */
    QStringList parseDomainList(const QString &text);
} // namespace CookieRules
