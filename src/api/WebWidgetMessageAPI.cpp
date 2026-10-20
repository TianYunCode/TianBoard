#include <QWebEnginePage>
#include "api/WebWidgetMessageAPI.h"

#include "core/Application.h"


WebWidgetMessageAPI::WebWidgetMessageAPI(GraphicsWidgetItem *graphicsWidgetItem, QObject *parent)
    : QObject(parent), mGraphicsWidgetItem(graphicsWidgetItem) {
    // 跨线程队列连接：消息可能由任意线程发出，回调统一回到本对象所属线程
    connect(WebWidgetMessageBroker::instance(), &WebWidgetMessageBroker::newMessage, this,
            &WebWidgetMessageAPI::onNewMessage, Qt::QueuedConnection);
}

WebWidgetMessageAPI::~WebWidgetMessageAPI() {
    // 刻意留空：连接随 QObject 析构自动断开，无需手工注销
}


void WebWidgetMessageAPI::sendMessage(const QString &pTopicName, const QString &pMessage) {
    WebWidgetMessageBroker::instance()->sendMessage(pTopicName, pMessage);
}


void WebWidgetMessageAPI::onNewMessage(const QString &pTopicName, const QString &pMessage) {
    if (mSubscribedTopics.contains(pTopicName)) {
        {
            QString js;
            js += "if(widget && widget.messages && widget.messages.onmessage)";
            js += "{widget.messages.onmessage('";
            js += pMessage + "', '" + pTopicName + "')}";

            // 这里目前只拼装脚本，尚未下发给页面执行；页面侧 JS 通道恢复后再接上
        }
    }
}


WebWidgetMessageBroker *WebWidgetMessageBroker::sInstance = 0;


WebWidgetMessageBroker::WebWidgetMessageBroker(QObject *parent) : QObject(parent) {
    // 刻意留空
}


WebWidgetMessageBroker::~WebWidgetMessageBroker() {
    // 刻意留空
}


WebWidgetMessageBroker *WebWidgetMessageBroker::instance() {
    // 挂到 Application 的静态内存清理器上，保证退出阶段才释放
    if (!sInstance)
        sInstance = new WebWidgetMessageBroker(Application::staticMemoryCleaner);

    return sInstance;
}


void WebWidgetMessageBroker::sendMessage(const QString &pTopicName, const QString &pMessage) {
    emit newMessage(pTopicName, pMessage);
}
