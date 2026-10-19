#include "web/ToolBarSearch.h"
#include <QWebEngineProfile>

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEngineSettings>
#include <QMenu>
#include <QCompleter>

#include "core/AutoSaver.h"


/*
    ToolbarSearch is a very basic search widget that also contains a small history.
    Searches are turned into urls that use Google to perform search
 */
ToolbarSearch::ToolbarSearch(QWidget *parent)
    : SearchLineEdit(parent)
    , mAutosaver(new AutoSaver(this))
    , mMaxSavedSearches(10)
    , mStringListModel(new QStringListModel(this))
{
    QMenu *m = menu();
    connect(m, SIGNAL(aboutToShow()), this, SLOT(aboutToShowMenu()));
    connect(m, SIGNAL(triggered(QAction*)), this, SLOT(triggeredMenuAction(QAction*)));

    QCompleter *completer = new QCompleter(mStringListModel, this);
    completer->setCompletionMode(QCompleter::InlineCompletion);
    lineEdit()->setCompleter(completer);

    connect(lineEdit(), SIGNAL(returnPressed()), SLOT(searchNow()));
    setInactiveText(tr("Search"));
    load();
}

ToolbarSearch::~ToolbarSearch()
{
    mAutosaver->saveIfNeccessary();
}

void ToolbarSearch::save()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("toolbarsearch"));
    settings.setValue(QLatin1String("recentSearches"), mStringListModel->stringList());
    settings.setValue(QLatin1String("maximumSaved"), mMaxSavedSearches);
    settings.endGroup();
}

void ToolbarSearch::load()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("toolbarsearch"));
    QStringList list = settings.value(QLatin1String("recentSearches")).toStringList();
    mMaxSavedSearches = settings.value(QLatin1String("maximumSaved"), mMaxSavedSearches).toInt();
    mStringListModel->setStringList(list);
    settings.endGroup();
}

void ToolbarSearch::searchNow()
{
    QString searchText = lineEdit()->text();
    QStringList newList = mStringListModel->stringList();

    if (newList.contains(searchText))
        newList.removeAt(newList.indexOf(searchText));
    newList.prepend(searchText);

    if (newList.size() >= mMaxSavedSearches)
        newList.removeLast();

    QWebEngineSettings *globalSettings = QWebEngineProfile::defaultProfile()->settings();
    if (!globalSettings->testAttribute(QWebEngineSettings::JavascriptEnabled))//PrivateBrowsingEnabled
    {
        mStringListModel->setStringList(newList);
        mAutosaver->changeOccurred();
    }

    QUrl url(QLatin1String("http://www.baidu.com/"));
    QUrlQuery urlQuery;

    urlQuery.addQueryItem(QLatin1String("q"), searchText);
    urlQuery.addQueryItem(QLatin1String("ie"), QLatin1String("UTF-8"));
    urlQuery.addQueryItem(QLatin1String("oe"), QLatin1String("UTF-8"));
    url.setQuery(urlQuery);
    emit search(url);
}

void ToolbarSearch::aboutToShowMenu()
{
    lineEdit()->selectAll();
    QMenu *m = menu();
    m->clear();
    QStringList list = mStringListModel->stringList();
    if (list.isEmpty())
    {
        m->addAction(tr("No Recent Searches"));
        return;
    }

    QAction *recent = m->addAction(tr("Recent Searches"));
    recent->setEnabled(false);
    for (int i = 0; i < list.count(); ++i)
    {
        QString text = list.at(i);
        m->addAction(text)->setData(text);
    }
    m->addSeparator();
    m->addAction(tr("Clear Recent Searches"), this, SLOT(clear()));
}

void ToolbarSearch::triggeredMenuAction(QAction *action)
{
    QVariant v = action->data();
    if (v.canConvert<QString>())
    {
        QString text = v.toString();
        lineEdit()->setText(text);
        searchNow();
    }
}

void ToolbarSearch::clear()
{
    mStringListModel->setStringList(QStringList());
    mAutosaver->changeOccurred();;
}

