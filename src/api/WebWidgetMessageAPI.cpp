#include <QWebEnginePage>
#include "api/WebWidgetMessageAPI.h"

#include "core/Application.h"


WebWidgetMessageAPI::WebWidgetMessageAPI(GraphicsWidgetItem *graphicsWidgetItem, QObject *parent)
    : QObject(parent)
    , mGraphicsWidgetItem(graphicsWidgetItem)
{
    connect(WebWidgetMessageBroker::instance(), &WebWidgetMessageBroker::newMessage,
            this, &WebWidgetMessageAPI::onNewMessage, Qt::QueuedConnection);
}

WebWidgetMessageAPI::~WebWidgetMessageAPI()
{
    // NOOP
}


void WebWidgetMessageAPI::sendMessage(const QString& pTopicName, const QString& pMessage)
{
    WebWidgetMessageBroker::instance()->sendMessage(pTopicName, pMessage);
}


void WebWidgetMessageAPI::onNewMessage(const QString& pTopicName, const QString& pMessage)
{
	//20210202 zhusizhi
    if (mSubscribedTopics.contains(pTopicName))
    {
        //if (mGraphicsWidgetItem && mGraphicsWidgetItem->page() && mGraphicsWidgetItem->page()->mainFrame())
        {
            QString js;
            js += "if(widget && widget.messages && widget.messages.onmessage)";
            js += "{widget.messages.onmessage('";
            js += pMessage + "', '" + pTopicName + "')}";

            //mGraphicsWidgetItem->page()->
            //    mainFrame()->evaluateJavaScript(js);

        }
    }
}



WebWidgetMessageBroker* WebWidgetMessageBroker::sInstance = 0;


WebWidgetMessageBroker::WebWidgetMessageBroker(QObject *parent)
    : QObject(parent)
{
    // NOOP
}


WebWidgetMessageBroker::~WebWidgetMessageBroker()
{
    // NOOP
}


WebWidgetMessageBroker* WebWidgetMessageBroker::instance()
{
    if (!sInstance)
        sInstance = new WebWidgetMessageBroker(Application::staticMemoryCleaner);

    return sInstance;

}


void WebWidgetMessageBroker::sendMessage(const QString& pTopicName, const QString& pMessage)
{
    emit newMessage(pTopicName, pMessage);
}
