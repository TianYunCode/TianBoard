/**
 * @file ThumbnailAdaptor.h
 * @brief 页面缩略图：按需生成、加载与定位 pageNNN.thumbnail.jpg。
 */

#pragma once

#include <QtCore>

class Document;
class DocumentProxy;
class GraphicsScene;

/**
 * @brief 缩略图管理器。
 *
 * 缩略图与页面数据分离存放在文档目录下，文件名由页面索引决定。读取时若缺失则
 * 通过 SvgSubsetAdaptor 加载场景并就地补生成；写回前会检查场景是否修改过，
 * 避免每次保存都重绘全部页面。
 */
class ThumbnailAdaptor
{
    Q_DECLARE_TR_FUNCTIONS(ThumbnailAdaptor)

public:
    /** @brief 返回某页缩略图的 file:// URL，不检查文件是否存在。 */
    static QUrl thumbnailUrl(DocumentProxy *proxy, int pageIndex);

    /** @brief 渲染并保存缩略图；场景未修改且文件存在时跳过，overrideModified 可强制重写。 */
    static void persistScene(DocumentProxy *proxy, GraphicsScene *pScene, int pageIndex, bool overrideModified = false);

    /** @brief 读取某页缩略图，调用方负责 delete；缺失时触发批量补生成。 */
    static const QPixmap *get(DocumentProxy *proxy, int index);
    /** @brief 清空并重新填充整个文档的缩略图列表，旧指针会被释放。 */
    static void load(DocumentProxy *proxy, QList<const QPixmap *> &list);

private:
    /** @brief 扫描所有页面，为缺少缩略图的页面各加载一次场景并生成。 */
    static void generateMissingThumbnails(DocumentProxy *proxy);

    ThumbnailAdaptor()
    {
    }
};
