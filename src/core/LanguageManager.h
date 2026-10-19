#pragma once

#include <QList>
#include <QObject>
#include <QString>

class QTranslator;

// 运行时语言切换：装卸 QTranslator、持久化选择、并通知界面重新取词。
// 源文本是英文，所以 "en" 不依赖任何 .qm —— 选中它等价于把翻译器摘掉。
class LanguageManager : public QObject {
    Q_OBJECT

public:
    struct Language {
        QString code;
        QString name;
    };

    static LanguageManager *instance();

    QList<Language> languages() const;
    QStringList availableCodes() const;
    bool isAvailable(const QString &code) const;

    QString displayName(const QString &code) const;

    QString currentLanguage() const { return mCurrentLanguage; }

    bool setLanguage(const QString &code);

    // 上次选过的语言。直接读 ini，避免为了一个字符串提前构造整个设置对象图。
    QString savedPreference() const;

signals:
    void languageChanged(const QString &code);

private:
    explicit LanguageManager(QObject *parent = nullptr);
    LanguageManager(const LanguageManager &) = delete;
    LanguageManager &operator=(const LanguageManager &) = delete;

    bool loadCatalogue(QTranslator *translator, const QString &prefix, const QString &code) const;

    static LanguageManager *sInstance;

    QTranslator *mApplicationTranslator;
    QTranslator *mQtTranslator;
    QString mCurrentLanguage;
    bool mInstalled;
};
