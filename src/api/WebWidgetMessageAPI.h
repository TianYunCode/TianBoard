/**
 * @file WebWidgetMessageAPI.h
 * @brief 声明网页小部件之间的主题消息订阅接口与进程内消息代理。
 */

#pragma once

#include <QtCore>

#include "whiteboard/GraphicsWidgetItem.h"

/**
 * @brief 单个网页小部件使用的主题消息端点。
 *
 * 位于 api 层：构造时向 WebWidgetMessageBroker 单例注册，把订阅集合交给代理过滤，
 * 只把本端点订阅过的主题回调给图形对象。生命周期由 WebWidgetBoardAPI 持有，
 * 随网页小部件销毁；回调发生在发出消息的线程（实际为 GUI 线程）。
 */
class WebWidgetMessageAPI : public QObject {
    Q_OBJECT

public:
    /** @brief 构造端点并挂到全局消息代理上。 */
    WebWidgetMessageAPI(GraphicsWidgetItem *graphicsWidgetItem, QObject *parent = 0);
    /** @brief 从消息代理注销本端点。 */
    virtual ~WebWidgetMessageAPI();

public slots:
    /** @brief 以主题名广播一条消息。 */
    void sendMessage(const QString &pTopicName, const QString &pMessage);

    /** @brief 订阅主题；重复订阅无副作用。 */
    void subscribeToTopic(const QString &pTopicName) { mSubscribedTopics << pTopicName; }

    /** @brief 取消订阅主题；未订阅时无副作用。 */
    void unsubscribeFromTopic(const QString &pTopicName) { mSubscribedTopics.remove(pTopicName); }

private slots:
    /** @brief 仅在主题属于本端点订阅集合时转交给图形对象。 */
    void onNewMessage(const QString &pTopicName, const QString &pMessage);

private:
    QSet<QString> mSubscribedTopics;         ///< 本端点订阅的主题集合。
    GraphicsWidgetItem *mGraphicsWidgetItem; ///< 接收消息的图形对象，不拥有所有权。
};

/**
 * @brief 进程内按主题转发的消息代理。
 *
 * 纯工具/服务型单例：所有网页小部件共用同一个实例，负责把消息广播给订阅者。
 * 首次 instance() 时创建，随进程退出销毁；不跨进程、不持久化。
 */
class WebWidgetMessageBroker : public QObject {
    Q_OBJECT

private:
    /** @brief 私有构造，保证单例。 */
    WebWidgetMessageBroker(QObject *parent = 0);
    /** @brief 私有析构。 */
    ~WebWidgetMessageBroker();

public:
    /** @brief 返回全局唯一实例。 */
    static WebWidgetMessageBroker *instance();

public slots:
    /** @brief 向所有订阅者广播消息。 */
    void sendMessage(const QString &pTopicName, const QString &pMessage);

signals:
    /** @brief 有新消息时发出，由各端点按订阅集合过滤。 */
    void newMessage(const QString &pTopicName, const QString &pMessage);

private:
    static WebWidgetMessageBroker *sInstance; ///< 进程级单例指针。
};
