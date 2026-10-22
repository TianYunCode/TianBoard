/**
 * @file MimeDataGraphicsItem.h
 * @brief 声明携带图形项副本的剪贴板数据：副本由它回收，场景不拥有。
 */

#pragma once

#include <QMimeData>
#include <QList>

class Item;

/**
 * @brief 携带图形项副本的剪贴板数据。
 *
 * 详细说明：构造时接管传入的 Item 指针副本，析构时统一释放；场景不拥有这些副本。
 * 它必须留在画板模块：释放副本要知道 `Item` 的完整类型，而 model 不该反向认识 whiteboard。
 * 与 model/MimeData 的区别就在载荷里装的东西：那边是"哪份文档的第几页"，这边是图形项本体。
 */
class MimeDataGraphicsItem : public QMimeData
{
    Q_OBJECT

public:
    /**
     * @brief 接管一组图形项副本。
     * @param pItems 图形项列表，所有权转移给本对象。
     */
    MimeDataGraphicsItem(QList<Item *> pItems);
    virtual ~MimeDataGraphicsItem();

    /// 返回持有的图形项副本。
    QList<Item *> items() const
    {
        return mItems;
    }

private:
    QList<Item *> mItems;
};
