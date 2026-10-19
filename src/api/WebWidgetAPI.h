#pragma once

#include <QtCore>

class GraphicsWebWidgetItem;
class WebWidgetPreferenceAPI;
class WebWidget;

class WebWidgetAPI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id READ id SCRIPTABLE true);
    Q_PROPERTY(QString name READ name SCRIPTABLE true);
    Q_PROPERTY(QString description READ description SCRIPTABLE true);
    Q_PROPERTY(QString author READ author SCRIPTABLE true);

    Q_PROPERTY(QString authorEmail READ authorEmail SCRIPTABLE true);
    Q_PROPERTY(QString authorHref READ authorHref SCRIPTABLE true);
    Q_PROPERTY(QString version READ version SCRIPTABLE true);

    Q_PROPERTY(int width READ width SCRIPTABLE true);
    Q_PROPERTY(int height READ height SCRIPTABLE true);

    Q_PROPERTY(QObject* preferences READ preferences SCRIPTABLE true);

    // 供脚本桥按 uuid 定位控件的扩展属性    Q_PROPERTY(QString uuid READ uuid SCRIPTABLE true);

    public:
        WebWidgetAPI(GraphicsWebWidgetItem *graphicsWidget, QObject *parent = 0);

        virtual ~WebWidgetAPI();

        QString uuid();

        QString id();
        QString name();
        QString description();
        QString author();
        QString authorEmail();
        QString authorHref();
        QString version();

        QObject* preferences();

        int width();
        int height();

        void openURL(const QString& url);

    private:
        GraphicsWebWidgetItem* mGraphicsWebWidget;
        WebWidgetPreferenceAPI* mPreferencesAPI;
};

class WebWidgetStorage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int length READ length SCRIPTABLE true);

    public:
        WebWidgetStorage(QObject *parent = 0)
        : QObject(parent){/* NOOP */}
        virtual ~WebWidgetStorage(){/* NOOP */}

    public slots:

        virtual QString key(int index) = 0;
        virtual QString getItem(const QString& key) = 0;
        virtual void setItem(const QString& key, const QString& value) = 0;
        virtual void removeItem(const QString& key) = 0;
        virtual void clear() = 0;

    protected:
        virtual int length() = 0;

};

class WebWidgetPreferenceAPI : public WebWidgetStorage
{
    Q_OBJECT

    public:
        WebWidgetPreferenceAPI(GraphicsWebWidgetItem *graphicsWidget, QObject *parent = 0);

        virtual ~WebWidgetPreferenceAPI();

    public slots:
        virtual QString key(int index);
        virtual QString getItem(const QString& key);
        virtual void setItem(const QString& key, const QString& value);
        virtual void removeItem(const QString& key);
        virtual void clear();

    protected:
        virtual int length();

    private:
        GraphicsWebWidgetItem* mGraphicsWebWidget;

};
