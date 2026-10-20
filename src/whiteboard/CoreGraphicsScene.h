/**
 * @file CoreGraphicsScene.h
 * @brief 带延迟删除与脏标记的 QGraphicsScene 基类声明。
 */

#pragma once

#include <QtWidgets>
#include <QGraphicsScene>
#include <QGraphicsItem>


/**
 * @brief QGraphicsScene 的轻量基类，提供“先移出场景、再择机删除”的延迟删除语义。
 *
 * 从场景移除的图元先记入 mItemsToDelete，只有真正销毁（deleteItem）或场景析构时
 * 才释放，这样撤销栈仍能安全引用已移除的图元。GraphicsScene 在此基础上扩展画板
 * 业务；本类只承担对象生命周期与脏标记。
 *
 * 线程约束：与 Qt 图形视图一致，只在 GUI 线程访问。
 */
class CoreGraphicsScene : public QGraphicsScene {
public:
    CoreGraphicsScene(QObject *parent = 0);
    virtual ~CoreGraphicsScene();

    /**
     * @brief 把图元加入场景，同时记入待删除集合。
     * @note 若图元是含子项的组容器，其子项会从待删除集合中移除，避免组内图元被误删。
     */
    virtual void addItem(QGraphicsItem *item);

    /**
     * @brief 把图元移出场景。
     * @param forceDelete 为 true 时立即从待删除集合中释放该图元。
     */
    virtual void removeItem(QGraphicsItem *item, bool forceDelete = false);

    /**
     * @brief 仅当图元在待删除集合中时才真正 delete。
     * @return 删除成功返回 true；图元不在待删除集合中返回 false。
     */
    virtual bool deleteItem(QGraphicsItem *item);

    /// @brief 把图元从待删除集合中摘出（图元被重新使用或改挂到其它父项时调用）。
    void removeItemFromDeletion(QGraphicsItem *item);
    /// @brief 把图元登记为待删除，场景析构时统一释放。
    void addItemToDeletion(QGraphicsItem *item);

    /// @brief 场景自上次保存后是否被修改。
    bool isModified() const { return mIsModified; }

    /// @brief 设置脏标记，通常在内容变更或保存完成后调用。
    void setModified(bool pModified) { mIsModified = pModified; }

private:
    QSet<QGraphicsItem *> mItemsToDelete; ///< 已移出场景但尚未释放的图元

    bool mIsModified; ///< 内容脏标记
};
