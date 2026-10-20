/**
 * @file GraphicsSvgItem.h
 * @brief 场景中 SVG 图元的声明。
 */

#pragma once

#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "whiteboard/Item.h"

#include "utils/Enums.h"

class GraphicsItemDelegate;
class GraphicsPixmapItem;

/**
 * @brief 场景中的 SVG 图元：以矢量方式渲染一份自带的 SVG 数据。
 *
 * SVG 的原始字节整体存在 mFileData 里，深拷贝与持久化都直接用这份内存数据，
 * 不再回磁盘读文件。矢量渲染在低倍缩放下代价明显，所以默认开
 * DeviceCoordinateCache 把内容缓存成位图，切到高质量档时才关掉缓存逐帧重画
 * （见 setRenderingQuality）。
 *
 * 交互（选中、拖动、旋转、右键菜单）全部交给 GraphicsItemDelegate；
 * Qt 自带的选中橡皮筋被 paint() 抹掉，选择框由委托绘制。
 *
 * 所有权与生命周期：图元归场景所有，委托由 GraphicsItem 基类析构时释放，
 * 渲染器以图元为父对象随之回收；仅在 GUI 线程使用。
 */
class GraphicsSvgItem : public QGraphicsSvgItem, public Item, public GraphicsItem {
public:
    /**
     * @brief 从磁盘上的 SVG 文件构造。
     * @param pFile SVG 文件路径，构造时会整份读入 mFileData。
     * @param parent 父图元，可为空。
     * @note 文件打不开时不报错也不重试，mFileData 留空，之后的深拷贝会得到空图元。
     */
    GraphicsSvgItem(const QString &pFile, QGraphicsItem *parent = 0);

    /**
     * @brief 从内存中的 SVG 字节数据构造。
     * @param pFileData SVG 文件的完整内容。
     * @param parent 父图元，可为空。
     * @note 会 new 一个以 this 为父的 QSvgRenderer 并设为共享渲染器。
     */
    GraphicsSvgItem(const QByteArray &pFileData, QGraphicsItem *parent = 0);

    /// @brief 公共初始化：层级数据、委托与框架能力、缓存策略与 UUID；两个构造函数都会调用。
    void init();

    /** @brief 析构；渲染器与委托分别由其父对象与基类释放。 */
    virtual ~GraphicsSvgItem();

    /// @brief 返回 SVG 的原始字节内容，供持久化与深拷贝使用。
    QByteArray fileData() const;

    /// @brief 替换 SVG 原始字节；只影响之后的拷贝与保存，不会重新加载渲染器。
    void setFileData(const QByteArray &pFileData) { mFileData = pFileData; }

    /// @brief 图元类型标识。
    enum { Type = GraphicsItemType::SvgItemType };

    /// @brief 返回图元类型，供 qgraphicsitem_cast 与场景分派使用。
    virtual int type() const { return Type; }

    /// @brief 用同一份 SVG 数据新建图元并复制几何与层级参数；返回值归调用方。
    virtual Item *deepCopy() const;

    /// @brief 把位置、变换、层级、锁定状态与来源复制到 copy；类型不匹配时静默不做事。
    virtual void copyItemParameters(Item *copy) const;

    /// @brief 切换渲染质量：高质量关掉位图缓存走矢量渲染，普通质量恢复缓存。
    virtual void setRenderingQuality(RenderingQuality pRenderingQuality);

    /// @brief 返回所属的画板场景；未加入场景或场景类型不符时返回空指针。
    virtual GraphicsScene *scene();

    /// @brief 转换成位图图元（新对象归调用方），用于降级渲染或导出。
    virtual GraphicsPixmapItem *toPixmapItem() const;

    /// @brief 只渲染出像素，不创建图形项（拖拽预览等场景用）。
    QPixmap renderToPixmap() const;

    /// @brief 设置 UUID，同时写入图元 data，便于场景内按 UUID 快速查找。
    virtual void setUuid(const QUuid &pUuid);

    /// @brief 删除文档持久化目录里对应的 .svg 文件；图元自身与 mFileData 不受影响。
    virtual void clearSource();

protected:
    /// @brief 按下时先生成拖拽用的位图与 MIME 数据，再交委托处理；委托不消费时回退基类。
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 移动优先由委托处理，未消费时回退基类。
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 释放时委托与基类都会收到，保证拖拽状态收尾。
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    /// @brief 绘制 SVG 内容，抹掉 Qt 的选中态，再由委托补画选择框。
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    /// @brief 几何与状态变更先交委托（撤销、层级修正），再回退基类。
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    QByteArray mFileData; ///< SVG 的原始字节内容，图元自持一份，不依赖磁盘文件
};
