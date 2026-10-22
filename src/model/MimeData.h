/**
 * @file MimeData.h
 * @brief 声明"文档页面引用"这一种拖放载荷：文档代理 + 页面下标。
 */

#pragma once

#include <QMimeData>
#include <QList>

class DocumentProxy;

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
 * @brief 携带一组页面引用的剪贴板数据。
 *
 * 详细说明：载荷里只有文档引用，所以它住在 model——文档库的树模型要解析它（拖页面进文档），
 * 画板与界面也要造它。携带图形项*副本*的那一种是 whiteboard/MimeDataGraphicsItem，
 * 那个必须留在画板模块里，因为它负责释放副本。
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
