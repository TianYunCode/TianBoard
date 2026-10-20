/**
 * @file GraphicsGroupContainerItemDelegate.h
 * @brief 声明图形项组合容器的委托，提供解散组合按钮与锁定/显示菜单项。
 */

#pragma once

#include "whiteboard/GraphicsItemDelegate.h"

class GraphicsGroupContainerItem;

/**
 * @brief 图形项组合（GraphicsGroupContainerItem）的交互委托。
 *
 * 在通用委托的基础上增加一个“解散组合”按钮，放在框架左上角；菜单中提供锁定与
 * “在扩展屏上显示”两个选项。委托不拥有组合对象，鼠标事件默认不消费，交由组合
 * 自身按普通图形项逻辑处理，只在按下时先释放当前交互的媒体子项。
 */
class GraphicsGroupContainerItemDelegate : public GraphicsItemDelegate {
    Q_OBJECT

public:
    /**
     * @brief 构造委托，使用通用标志并保持宽高比。
     * @param pDelegated 被委托的组合图形项，所有权不转移。
     * @param parent 父 QObject，可为空。
     */
    GraphicsGroupContainerItemDelegate(QGraphicsItem *pDelegated, QObject *parent = 0);
    /** @brief 把被委托项转换为组合类型，失败时返回空。 */
    GraphicsGroupContainerItem *delegated();

protected:
    /** @brief 在菜单中加入锁定与扩展屏显示两个可勾选项。 */
    virtual void decorateMenu(QMenu *menu);
    /** @brief 创建解散组合按钮并加入按钮列表。 */
    virtual void buildButtons();
    /** @brief 释放按钮并把解散按钮指针置空。 */
    virtual void freeButtons();

    /** @brief 按下时释放当前交互的媒体子项，不拦截事件。 */
    virtual bool mousePressEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 不处理移动事件。 */
    virtual bool mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 不处理释放事件。 */
    virtual bool mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private slots:
    /** @brief 解散当前组合。 */
    void destroyGroup();

private:
    DelegateButton *mDestroyGroupButton;
};
