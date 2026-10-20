/**
 * @file LanguageManager.h
 * @brief 声明运行时语言管理单例：枚举目录中的翻译、装卸 QTranslator 并持久化选择。
 */

#pragma once

#include <QList>
#include <QObject>
#include <QString>

class QTranslator;

/**
 * @brief 运行时语言切换：装卸 QTranslator、持久化选择、并通知界面重新取词。
 *
 * 详细说明：进程级单例，首次调用 instance() 时创建；两个 QTranslator 由本类持有。
 * 源文本是英文，所以 "en" 不依赖任何 .qm —— 选中它等价于把翻译器摘掉。
 * 线程约束：GUI 线程专用，语言切换会触发所有窗口重新翻译。
 */
class LanguageManager : public QObject {
    Q_OBJECT

public:
    /// 一种可选语言。
    struct Language {
        QString code; ///< 语言代码，如 "en"、"fr"
        QString name; ///< 用于界面显示的本地化名称
    };

    /// 返回进程内唯一实例。
    static LanguageManager *instance();

    /// 返回所有可用语言，按显示名称排序。
    QList<Language> languages() const;
    /// 返回可用语言代码列表。
    QStringList availableCodes() const;
    /// 判断语言代码是否有对应翻译（源语言 en 始终可用）。
    bool isAvailable(const QString &code) const;

    /// 返回语言的显示名称；未知代码原样返回。
    QString displayName(const QString &code) const;

    /// 返回当前生效的语言代码。
    QString currentLanguage() const { return mCurrentLanguage; }

    /**
     * @brief 切换到指定语言并安装对应翻译器。
     * @param code 目标语言代码。
     * @return 切换成功返回 true；无对应翻译时返回 false 且保持当前语言。
     * @note 成功后发出 languageChanged。
     */
    bool setLanguage(const QString &code);

    // 上次选过的语言。直接读 ini，避免为了一个字符串提前构造整个设置对象图。
    /// 读取上次保存的语言代码；未保存过时为空串。
    QString savedPreference() const;

signals:
    /// 当前语言已切换。
    void languageChanged(const QString &code);

private:
    explicit LanguageManager(QObject *parent = nullptr);
    LanguageManager(const LanguageManager &) = delete;
    LanguageManager &operator=(const LanguageManager &) = delete;

    /**
     * @brief 从资源目录加载一个翻译文件到给定翻译器。
     * @param translator 目标翻译器。
     * @param prefix 文件名前缀（应用或 Qt 自带翻译）。
     * @param code 语言代码。
     * @return 加载成功返回 true。
     */
    bool loadCatalogue(QTranslator *translator, const QString &prefix, const QString &code) const;

    static LanguageManager *sInstance;

    QTranslator *mApplicationTranslator;
    QTranslator *mQtTranslator;
    QString mCurrentLanguage;
    bool mInstalled;
};
