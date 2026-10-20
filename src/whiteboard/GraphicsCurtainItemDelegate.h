/**
 * @file GraphicsCurtainItemDelegate.h
 * @brief 声明幕布图形项的委托，负责幕布的选中框架、菜单与删除行为。
 */

#pragma once

#include <QtWidgets>

#include <QtSvg>
#include <QtSvgWidgets>

#include "utils/Enums.h"
#include "whiteboard/GraphicsItemDelegate.h"

class QGraphicsSceneMouseEvent;
class QGraphicsItem;
class GraphicsCurtainItem;

/**
 * @brief 幕布图形项（GraphicsCurtainItem）的交互委托。
 *
 * 幕布用于整屏遮挡或揭示内容，是场景中的控制类图形项。本委托在通用委托基础上：
 * - 只构建基础按钮，隐藏上移/下移层级的按钮（幕布不参与普通对象的层级调整）；
 * - 创建框架后立即隐藏，幕布在被选中前不显示选择框；
 * - 删除时不真正销毁对象，只隐藏并发出移除信号，以便后续恢复。
 *
 * 委托不拥有被委托的图形项，其生命周期由图形项负责；框架与按钮由基类统一管理。
 */
class GraphicsCurtainItemDelegate : public GraphicsItemDelegate
{
    Q_OBJECT

public:
    /**
     * @brief 构造委托。
     * @param pDelegated 被委托的幕布图形项，不能为空，所有权不转移。
     * @param parent 父 QObject，可为空。
     */
    GraphicsCurtainItemDelegate(GraphicsCurtainItem *pDelegated, QObject *parent = 0);

    /** @brief 析构；控件由基类析构时释放。 */
    virtual ~GraphicsCurtainItemDelegate();

    /**
     * @brief 处理鼠标按下：开始一个撤销步骤，但不再拦截事件。
     * @return 固定返回 false，让图形项继续按默认方式处理选中。
     */
    virtual bool mousePressEvent(QGraphicsSceneMouseEvent *event);

    /**
     * @brief 转发图形项状态变化；可见性变化目前不做额外处理。
     * @return 基类处理后的值。
     */
    virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);

    /** @brief 摆放选择框与按钮，并隐藏层级调整按钮。 */
    virtual void positionHandles();

    /**
     * @brief 首次初始化时创建控件并隐藏框架与层级按钮。
     * @note 重复调用无副作用，已存在框架时直接返回。
     */
    virtual void init();

public slots:
    /**
     * @brief 隐藏幕布并通知场景其已被移除。
     * @param checked 按钮选中态，未使用。
     * @param canUndo 是否允许撤销，当前未使用：幕布删除不进入撤销栈。
     */
    virtual void remove(bool checked, bool canUndo = true);
};
