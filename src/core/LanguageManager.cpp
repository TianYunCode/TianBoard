#include "core/LanguageManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QLocale>
#include <QSettings>
#include <QTranslator>

#include "core/Settings.h"
#include "platform/PlatformUtils.h"


LanguageManager* LanguageManager::sInstance = nullptr;

namespace
{
    // 源文本是英文，因此 en 不需要 .qm。新增语言时只要 .ts 里出现该代码，
    // languages() 就会自动带上它，name 表只负责显示成本地文字。
    struct NativeName
    {
        const char *code;
        const char *name;
    };

    const NativeName sNativeNames[] = {
        { "en",      "English" },
        { "zh_CN",   "简体中文" },
        { "zh_TW",   "繁體中文" },
        { "fr",      "Français" },
        { "fr_FR",   "Français" },
        { "es",      "Español" },
        { "de",      "Deutsch" },
        { "it",      "Italiano" },
        { "pt",      "Português" },
        { "ru",      "Русский" },
        { "ja",      "日本語" },
        { "ko",      "한국어" },
        { "ar",      "العربية" },
        { "nl",      "Nederlands" },
        { "pl",      "Polski" },
        { "tr",      "Türkçe" },
        { "cs",      "Čeština" },
        { "sv",      "Svenska" },
        { "uk",      "Українська" },
        { "hi",      "हिन्दी" },
        { "id",      "Bahasa Indonesia" },
        { "vi",      "Tiếng Việt" },
    };

    const char *const kSourceLanguage = "en";

    // 语言偏好直接读写 ini，而不是走 Settings::settings()：启动流程必须在
    // Settings 构造之前就把翻译器装好（Setting 会连带构造 ToolsManager，
    // 后者的工具名带 tr()），所以这里不能提前把整个设置对象图拉起来。
    const char *const kLanguageKey = "App/PreferredLanguage";

    QString userConfigPath()
    {
        return Settings::userDataDirectory() + "/" + QCoreApplication::applicationName() + "User.config";
    }

    QString appConfigPath()
    {
        return PlatformUtils::applicationResourcesDirectory() + "/etc/" + QCoreApplication::applicationName() + ".config";
    }

    QString readSavedLanguage()
    {
        QSettings user(userConfigPath(), QSettings::IniFormat);
        QString language = user.value(QLatin1String(kLanguageKey)).toString();

        if (language.isEmpty())
        {
            QSettings app(appConfigPath(), QSettings::IniFormat);
            language = app.value(QLatin1String(kLanguageKey)).toString();
        }

        return language;
    }

    void writeSavedLanguage(const QString &code)
    {
        QSettings user(userConfigPath(), QSettings::IniFormat);
        user.setValue(QLatin1String(kLanguageKey), code);
    }
}

LanguageManager::LanguageManager(QObject *parent)
    : QObject(parent)
    , mApplicationTranslator(new QTranslator(this))
    , mQtTranslator(new QTranslator(this))
    , mCurrentLanguage(QString::fromLatin1(kSourceLanguage))
    , mInstalled(false)
{
    // NOOP
}

LanguageManager* LanguageManager::instance()
{
    if (!sInstance)
        sInstance = new LanguageManager(QCoreApplication::instance());

    return sInstance;
}

QString LanguageManager::displayName(const QString &code) const
{
    for (const NativeName &entry : sNativeNames)
    {
        if (QString::fromLatin1(entry.code).compare(code, Qt::CaseInsensitive) == 0)
            return QString::fromUtf8(entry.name);
    }

    return code;
}

QList<LanguageManager::Language> LanguageManager::languages() const
{
    QList<Language> result;

    Language source;
    source.code = QString::fromLatin1(kSourceLanguage);
    source.name = displayName(source.code);
    result << source;

    const QStringList catalogues = PlatformUtils::availableTranslations();
    for (const QString &code : catalogues)
    {
        if (code.compare(source.code, Qt::CaseInsensitive) == 0)
            continue;

        Language language;
        language.code = code;
        language.name = displayName(code);
        result << language;
    }

    return result;
}

QStringList LanguageManager::availableCodes() const
{
    QStringList codes;

    for (const Language &language : languages())
        codes << language.code;

    return codes;
}

bool LanguageManager::isAvailable(const QString &code) const
{
    return availableCodes().contains(code, Qt::CaseInsensitive);
}

bool LanguageManager::loadCatalogue(QTranslator *translator, const QString &prefix, const QString &code) const
{
    QString path = PlatformUtils::translationPath(prefix, code);

    if (!QFile::exists(path) && code.length() > 2)
    {
        const QString shortCode = code.left(2);
        const QString shortPath = PlatformUtils::translationPath(prefix, shortCode);
        path = QFile::exists(shortPath) ? shortPath : QString();
    }

    if (path.isEmpty() || !QFile::exists(path))
        return false;

    return translator->load(path);
}

bool LanguageManager::setLanguage(const QString &code)
{
    if (code.isEmpty())
        return false;

    const bool isSourceLanguage = (code.compare(QLatin1String(kSourceLanguage), Qt::CaseInsensitive) == 0);

    if (!isSourceLanguage)
    {
        QString resolved = code;

        // 目录里可能只有两位语言的 .qm（zh_CN 请求命中 zh），先按可用清单解析。
        if (!PlatformUtils::availableTranslations().contains(code, Qt::CaseInsensitive))
        {
            resolved.clear();
            for (const QString &available : PlatformUtils::availableTranslations())
            {
                if (available.compare(code, Qt::CaseInsensitive) == 0
                    || (code.length() > 2 && available.compare(code.left(2), Qt::CaseInsensitive) == 0))
                {
                    resolved = available;
                    break;
                }
            }

            if (resolved.isEmpty())
                return false;
        }

        if (!loadCatalogue(mApplicationTranslator, QStringLiteral("TianBoard_"), resolved))
            return false;

        if (!mInstalled)
        {
            QCoreApplication::installTranslator(mApplicationTranslator);
            mInstalled = true;
        }

        if (loadCatalogue(mQtTranslator, QStringLiteral("qt_"), resolved))
        {
            QCoreApplication::installTranslator(mQtTranslator);
        }
        else
        {
            qDebug() << "Qt gui translation in " << resolved << " is not available";
        }

        mCurrentLanguage = resolved;
    }
    else
    {
        if (mInstalled)
        {
            QCoreApplication::removeTranslator(mApplicationTranslator);
            QCoreApplication::removeTranslator(mQtTranslator);
            mInstalled = false;
        }

        mCurrentLanguage = QString::fromLatin1(kSourceLanguage);
    }

    QLocale::setDefault(QLocale(mCurrentLanguage));

    writeSavedLanguage(mCurrentLanguage);

    emit languageChanged(mCurrentLanguage);

    return true;
}

QString LanguageManager::savedPreference() const
{
    return readSavedLanguage();
}
