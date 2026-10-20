/**
 * @file Item.h
 * @brief 画板图元的数据混入类与 GraphicsItem 辅助基类声明。
 */

#pragma once

#include <QtWidgets>
#include "utils/Enums.h"
#include "whiteboard/GraphicsItemDelegate.h"

class GraphicsScene;
class GraphicsItem;


/**
 * @brief 图元的数据混入类：UUID、渲染质量、来源 URL 与复制协议。
 *
 * 具体图元以多继承方式同时实现 QGraphicsItem 与 Item，Item 侧不参与 Qt 图元树，
 * 只保存与绘制无关的持久化数据。对象归场景所有，生命周期随图元销毁；
 * 仅在 GUI 线程读写。
 */
class Item {
protected:
    Item();

public:
    virtual ~Item();

    /// @brief 图元渲染质量档位。
    enum RenderingQuality {
        RenderingQualityNormal = 0, ///< 默认质量，优先性能
        RenderingQualityHigh        ///< 高质量，代价更高
    };

    /// @brief 图元唯一标识，用于撤销、持久化与查找。
    virtual QUuid uuid() const { return mUuid; }

    /// @brief 设置唯一标识；部分图元会同时写入图元 data 以加速查找。
    virtual void setUuid(const QUuid &pUuid) { mUuid = pUuid; }

    /// @brief 当前渲染质量档位。
    virtual RenderingQuality renderingQuality() const { return mRenderingQuality; }

    /// @brief 设置渲染质量档位，通常由缩放比例变化驱动。
    virtual void setRenderingQuality(RenderingQuality pRenderingQuality) {
        mRenderingQuality = pRenderingQuality;
    }

    /// @brief 深拷贝图元，返回由调用方接管的新实例。
    virtual Item *deepCopy() const = 0;

    /// @brief 把本图元的参数复制到另一个实例，供 deepCopy 与复制粘贴复用。
    virtual void copyItemParameters(Item *copy) const = 0;

    /// @brief 返回图元所在场景；没有场景关联的实现返回空指针。
    virtual GraphicsScene *scene() { return 0; }

    /// @brief 图元的来源地址（导入文件、网页等），用于重新加载。
    virtual QUrl sourceUrl() const { return mSourceUrl; }

    /// @brief 设置来源地址。
    virtual void setSourceUrl(const QUrl &pSourceUrl) { mSourceUrl = pSourceUrl; }

protected:
    QUuid mUuid; ///< 图元唯一标识

    RenderingQuality mRenderingQuality; ///< 渲染质量档位

    QUrl mSourceUrl; ///< 来源地址
};

/**
 * @brief 图元的辅助基类：持有委托（Delegate）并提供静态的跨图元查询工具。
 *
 * 与 Item 一样以多继承方式混入 QGraphicsItem 派生类。委托对象在构造时创建、
 * 在本基类析构时删除，负责交互与撤销；静态方法通过 QGraphicsItem::type()
 * 做安全的向下转换。仅限 GUI 线程使用。
 */
class GraphicsItem {
protected:
    GraphicsItem() : mDelegate(NULL) {}
    virtual ~GraphicsItem();
    /// @brief 绑定委托；每个图元只允许绑定一次，重复绑定会触发断言。
    void setDelegate(GraphicsItemDelegate *mDelegate);

public:
    /// @brief 图元类型，取值见 GraphicsItemType。
    virtual int type() const = 0;

    /// @brief 返回本图元绑定的委托。
    GraphicsItemDelegate *Delegate() const;

    /// @brief 设置 Z 值并同步到 ItemOwnZValue，便于撤销时恢复原始层级。
    static void assignZValue(QGraphicsItem *, qreal value);
    /// @brief 图元是否允许旋转（读取 ItemRotatable 数据）。
    static bool isRotatable(QGraphicsItem *item);
    /// @brief 图元是否允许翻转（读取 ItemFlippable 数据）。
    static bool isFlippable(QGraphicsItem *item);
    /// @brief 图元是否被锁定（读取 ItemLocked 数据）。
    static bool isLocked(QGraphicsItem *item);
    /// @brief 读取图元自身保存的 UUID；未设置时返回空 UUID。
    static QUuid getOwnUuid(QGraphicsItem *item);

    /// @brief 按图元类型分派，取得其委托；未知类型返回空指针。
    static GraphicsItemDelegate *Delegate(QGraphicsItem *pItem);

    /**
     * @brief 移除图元。
     * @param canUndo 为 true 时通过委托产生可撤销的删除记录。
     */
    void remove(bool canUndo = true);

    /// @brief 释放图元占用的外部资源（媒体、PDF 引用等），默认无操作。
    virtual void clearSource() {}

private:
    GraphicsItemDelegate *mDelegate; ///< 交互与撤销委托，析构时释放
};
