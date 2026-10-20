/**
 * @file ImportImage.h
 * @brief 把图片文件作为页面背景导入当前页。
 */

#pragma once

#include <QtGui>
#include "content/ImportAdaptor.h"

class DocumentProxy;

/**
 * @brief 图片按页导入器。
 *
 * 用 QImageReader 支持的格式构造过滤器；导入结果是单张 GraphicsPixmapItem，
 * 放置时由场景转成背景对象，因此图片不会随页面数据另存，而是嵌入场景。
 *
 * @note 支持列表取自运行期 QImageReader，不同平台的可用格式可能不同。
 */
class ImportImage : public PageBasedImportAdaptor
{
    Q_OBJECT;

public:
    ImportImage(QObject *parent = 0);
    virtual ~ImportImage();

    /** @brief 返回当前 Qt 图像插件支持的小写扩展名。 */
    virtual QStringList supportedExtentions();
    /** @brief 拼接 “Image Format (*.png *.jpg ...)” 过滤器。 */
    virtual QString importFileFilter();

    /** @brief 加载图片并返回一个尚未加入场景的 GraphicsPixmapItem。 */
    virtual QList<GraphicsItem *> import(const QUuid &uuid, const QString &filePath);
    /** @brief 把图片设为场景背景并删除临时图元。 */
    virtual void placeImportedItemToScene(GraphicsScene *scene, GraphicsItem *item);
    /** @brief 图片无外部资源目录，固定返回空串。 */
    virtual const QString &folderToCopy();
};
