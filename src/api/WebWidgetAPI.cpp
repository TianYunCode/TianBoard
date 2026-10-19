#include "api/WebWidgetAPI.h"

#include <QtWidgets>
#include <QtWebEngineWidgets>

#include "core/Application.h"
#include "core/ApplicationController.h"

#include "web/WebController.h"

#include "whiteboard/GraphicsWidgetItem.h"
#include "whiteboard/GraphicsProxyWidget.h"

#include "api/WebWidgetMessageAPI.h"


WebWidgetAPI::WebWidgetAPI(GraphicsWebWidgetItem *graphicsWidget, QObject *parent)
    : QObject(parent)
    , mGraphicsWebWidget(graphicsWidget)
{
    mPreferencesAPI = new WebWidgetPreferenceAPI(graphicsWidget, parent);

}

WebWidgetAPI::~WebWidgetAPI()
{
    // NOOP
}


QString WebWidgetAPI::uuid()
{
    if (mGraphicsWebWidget)
        return mGraphicsWebWidget->uuid().toString();
    else
        return "";
}


int WebWidgetAPI::width()
{
    return mGraphicsWebWidget->nominalSize().width();
}


int WebWidgetAPI::height()
{
    return mGraphicsWebWidget->nominalSize().height();
}


QString WebWidgetAPI::id()
{
    return mGraphicsWebWidget->metadatas().id;
}


QString WebWidgetAPI::name()
{
    return mGraphicsWebWidget->metadatas().name;
}


QString WebWidgetAPI::description()
{
    return mGraphicsWebWidget->metadatas().description;
}


QString WebWidgetAPI::author()
{
    return mGraphicsWebWidget->metadatas().author;
}


QString WebWidgetAPI::authorEmail()
{
    return mGraphicsWebWidget->metadatas().authorEmail;
}


QString WebWidgetAPI::authorHref()
{
    return mGraphicsWebWidget->metadatas().authorHref;
}


QString WebWidgetAPI::version()
{
    return mGraphicsWebWidget->metadatas().version;
}

QObject* WebWidgetAPI::preferences()
{
    return mPreferencesAPI;
}


void WebWidgetAPI::openURL(const QString& url)
{
    Application::webController->loadUrl(QUrl(url));
}


WebWidgetPreferenceAPI::WebWidgetPreferenceAPI(GraphicsWebWidgetItem *graphicsWidget, QObject *parent)
    : WebWidgetStorage(parent)
    , mGraphicsWebWidget(graphicsWidget)
{
    // NOOP
}

WebWidgetPreferenceAPI::~WebWidgetPreferenceAPI()
{
    // NOOP
}


QString WebWidgetPreferenceAPI::key(int index)
{
  QMap<QString, GraphicsWebWidgetItem::PreferenceValue> prefs = mGraphicsWebWidget->preferences();

  if (index < prefs.size())
    return prefs.keys().at(index);
  else
    return "";
}

QString WebWidgetPreferenceAPI::getItem(const QString& key)
{
  if (mGraphicsWebWidget) {
    QMap<QString, QString> docPref = mGraphicsWebWidget->GraphicsWidgetItem::preferences();
    if (docPref.contains(key))
      return docPref.value(key);
  

    QMap<QString, GraphicsWebWidgetItem::PreferenceValue> prefs = mGraphicsWebWidget->preferences();

    if (prefs.contains(key)) {
      GraphicsWebWidgetItem::PreferenceValue pref = prefs.value(key);
      return pref.value;
    }
  }
  return QString();
}

int WebWidgetPreferenceAPI::length()
{
   QMap<QString, GraphicsWebWidgetItem::PreferenceValue> prefs = mGraphicsWebWidget->preferences();

   return prefs.size();
}


void WebWidgetPreferenceAPI::setItem(const QString& key, const QString& value)
{
  if (mGraphicsWebWidget) {
    QMap<QString, GraphicsWebWidgetItem::PreferenceValue> prefs = mGraphicsWebWidget->preferences();

    if (prefs.contains(key) && !prefs.value(key).readonly)
      mGraphicsWebWidget->setPreference(key, value);
  }
}




void WebWidgetPreferenceAPI::removeItem(const QString& key)
{
  if (mGraphicsWebWidget)
    mGraphicsWebWidget->removePreference(key);
}


void WebWidgetPreferenceAPI::clear()
{
  if (mGraphicsWebWidget)
    mGraphicsWebWidget->removeAllPreferences();
}




