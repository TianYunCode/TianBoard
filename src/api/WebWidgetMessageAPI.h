#pragma once

#include <QtCore>

#include "whiteboard/GraphicsWidgetItem.h"

class WebWidgetMessageAPI : public QObject
{
    Q_OBJECT

    public:
        WebWidgetMessageAPI(GraphicsWidgetItem *graphicsWidgetItem, QObject *parent = 0);
        virtual ~WebWidgetMessageAPI();

    public slots:
        void sendMessage(const QString& pTopicName, const QString& pMessage);

        void subscribeToTopic(const QString& pTopicName)
        {
            mSubscribedTopics << pTopicName;
        }

        void unsubscribeFromTopic(const QString& pTopicName)
        {
            mSubscribedTopics.remove(pTopicName);
        }

    private slots:
         void onNewMessage(const QString& pTopicName, const QString& pMessage);

    private:
        QSet<QString> mSubscribedTopics;
        GraphicsWidgetItem *mGraphicsWidgetItem;
};

class WebWidgetMessageBroker : public QObject
{
    Q_OBJECT

    private:
        WebWidgetMessageBroker(QObject *parent = 0);
        ~WebWidgetMessageBroker();

    public:
        static WebWidgetMessageBroker* instance();

    public slots:
        void sendMessage(const QString& pTopicName, const QString& pMessage);

    signals:
        void newMessage(const QString& pTopicName, const QString& pMessage);

    private:
      static WebWidgetMessageBroker* sInstance;

};

