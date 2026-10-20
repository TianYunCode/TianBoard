/**
 * @file PdfPageItem.h
 * @brief PDF 单页图元声明。
 */

#pragma once

#include <QtWidgets>
#include <QtCore/QObject>

#include "whiteboard/PDFRenderer.h"


/**
 * @brief 在场景中显示 PDF 的一页，绘制委托给共享的 PDFRenderer。
 *
 * 构造时 attach 渲染器、析构时 detach，因此图元存活期间文档不会被释放。
 * 使用设备坐标缓存（DeviceCoordinateCache），缩放后按新的像素尺寸重新渲染。
 * 生命周期归场景所有，仅在 GUI 线程绘制。
 */
class PdfPageItem : public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem);

public:
    /**
     * @param renderer 共享的文档渲染器，本图元只增删引用计数，不拥有它。
     * @param pageNumber 页码，从 1 开始。
     * @param parentItem 父图元，可为空。
     */
    PdfPageItem(PDFRenderer *renderer, int pageNumber, QGraphicsItem *parentItem = 0);
    virtual ~PdfPageItem();

    /// @brief 页面图元的外接矩形；文档无效时为空。
    virtual QRectF boundingRect() const;

    /// @brief 把可见区域对应的页面内容绘制到目标画笔。
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    /// @brief 当前页码。
    int pageNumber() const { return mPageNumber; }
    /// @brief 所属文档的标识。
    QUuid fileUuid() const { return mRenderer->fileUuid(); }
    /// @brief 所属文档的原始字节。
    QByteArray fileData() const { return mRenderer->fileData(); }

protected:
    PDFRenderer *mRenderer; ///< 共享渲染器，不持有所有权
    int mPageNumber;        ///< 页码，从 1 开始
};
