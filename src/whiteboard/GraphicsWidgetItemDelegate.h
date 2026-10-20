/**
 * @file GraphicsWidgetItemDelegate.h
 * @brief 声明小部件图元的交互委托：冻结/解冻菜单与“转为工具”入口。
 */

#pragma once

#include <QtWidgets>

#include "whiteboard/GraphicsItemDelegate.h"
#include "whiteboard/GraphicsWidgetItem.h"

/**
 * @brief 挂在 GraphicsWidgetItem 上的委托。
 *
 * 在通用委托（选中框、缩放手柄、删除/复制）之外，往右键菜单里加两项：冻结与否的开关，
 * 以及当小部件声明自己可作工具时的“转为工具”动作——后者把它搬到控制视图上常驻。
 *
 * 生命周期与所委托的图元一致，由图元持有；菜单动作是本委托的子对象，随本对象析构。
 *
 * @note 线程约束：只在 GUI 线程操作，菜单动作直接改 UI 状态。
 */
class GraphicsWidgetItemDelegate : public GraphicsItemDelegate
{
    Q_OBJECT

public:
    /**
     * @brief 建立委托并记下小部件种类。
     * @param pDelegated 被委托的小部件图元。
     * @param widgetType WidgetType::Enum 里的值，决定菜单里能出现哪些动作。
     */
    GraphicsWidgetItemDelegate(GraphicsWidgetItem *pDelegated, int widgetType = 0);
    virtual ~GraphicsWidgetItemDelegate();

protected:
    /// @brief 追加“冻结”与“转为工具”两个菜单动作。
    virtual void decorateMenu(QMenu *menu);
    /// @brief 让冻结动作的勾选状态跟图元实际状态对齐。
    virtual void updateMenuActionState();

private slots:
    /// @brief 冻结或解冻图元；解冻后重新接受交互。
    void freeze(bool frozeon);
    /// @brief 把小部件搬到控制视图，作为常驻工具使用。
    void pin();

private:
    int mWidgetType; ///< 被委托图元的种类（WidgetType::Enum）

    /// @brief 把通用委托对象还原成小部件图元。
    GraphicsWidgetItem *delegated();

    QAction *freezeAction;    ///< 冻结开关，勾选态随图元状态刷新
    QAction *setAsToolAction; ///< “转为工具”动作，只有可作工具的小部件才有
};
