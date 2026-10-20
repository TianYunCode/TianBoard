/**
 * @file MimeData.h
 * @brief 声明剪贴板/拖放使用的自定义 MIME 负载类型。
 */

#pragma once

#include <QtWidgets>

class DocumentProxy;
class Item;

/**
 * @brief 一条“文档页面”引用：文档代理 + 页面下标。
 *
 * 详细说明：只保存指针与下标，不持有文档；文档在粘贴前被关闭时引用即失效。
 */
struct MimeDataItem
{
public:
    /**
     * @brief 构造页面引用。
     * @param proxy 所属文档，调用方保证其生命周期覆盖剪贴板数据。
     * @param sceneIndex 页面下标。
     */
    MimeDataItem(DocumentProxy *proxy, int sceneIndex);
    virtual ~MimeDataItem();

    /// 返回所属文档。
    DocumentProxy *documentProxy() const
    {
        return mProxy;
    }
    /// 返回页面下标。
    int sceneIndex() const
    {
        return mSceneIndex;
    }

private:
    DocumentProxy *mProxy;
    int            mSceneIndex;
};

/**
 * @brief 携带图形项副本的剪贴板数据。
 *
 * 详细说明：构造时接管传入的 Item 指针副本，析构时统一释放；场景不拥有这些副本。
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

/**
 * @brief 携带一组页面引用的剪贴板数据，并声明页面拖放 MIME 类型。
 */
class MimeData : public QMimeData
{
    Q_OBJECT

public:
    /**
     * @brief 构造页面引用集合。
     * @param items 页面引用列表。
     */
    MimeData(const QList<MimeDataItem> &items);
    virtual ~MimeData();

    /// 返回页面引用列表。
    QList<MimeDataItem> items() const
    {
        return mItems;
    }

private:
    QList<MimeDataItem> mItems;
};
