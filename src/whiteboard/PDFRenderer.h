/**
 * @file PDFRenderer.h
 * @brief PDF 文档渲染器抽象接口与按 UUID 共享的实例表声明。
 */

#pragma once

#include <QObject>
#include <QSizeF>
#include <QRect>
#include <QByteArray>
#include <QUuid>
#include <QMap>
#include <QPointer>

class QPainter;


/**
 * @brief PDF 文档渲染的抽象接口，同一份文档在应用内按 UUID 共享。
 *
 * 页面图元（PdfPageItem）各自 attach()，析构时 detach()；引用计数归零即从
 * sRenderers 移除并自删，因此除 attach/detach 外不要 delete 本类实例。
 * 渲染结果与 Qt 的 QPdfDocument 一样偏向 GUI 线程使用，不要在后台线程调用。
 */
class PDFRenderer : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 取得（或创建）指定 UUID 的渲染器。
     * @param uuid 文档标识，同一 UUID 复用同一实例。
     * @param filename 文档路径，仅在首次创建时用于加载。
     * @param importingFile 是否处于导入流程，交由具体实现解释。
     * @note 首次创建会读取整个文件内容并缓存，并按主屏 DPI 设置渲染精度。
     */
    static PDFRenderer *rendererForUuid(const QUuid &uuid, const QString &filename,
                                        bool importingFile = false);
    virtual ~PDFRenderer();

    /// @brief 文档是否可渲染。
    virtual bool isValid() const = 0;

    /// @brief 页数；无效文档返回 0。
    virtual int pageCount() const = 0;

    /// @brief 指定页（从 1 开始）的尺寸，单位为按 DPI 缩放后的像素。
    virtual QSizeF pageSizeF(int pageNumber) const = 0;

    /// @brief 指定页的旋转角度（度）。
    virtual int pageRotation(int pageNumber) const = 0;

    /// @brief 文档标题元数据。
    virtual QString title() const = 0;

    /// @brief 增加引用计数，页面图元创建时调用。
    void attach();
    /// @brief 减少引用计数，归零时从共享表移除并自删。
    void detach();

    /// @brief 文档标识。
    QUuid fileUuid() const { return mFileUuid; }
    /// @brief 文档原始字节，供保存/另存时复用。
    QByteArray fileData() const { return mFileData; }

    /// @brief 设置渲染 DPI，影响 pageSizeF 与位图尺寸。
    void setDPI(int desiredDPI) { this->dpiForRendering = desiredDPI; }

public slots:
    /**
     * @brief 把指定页绘制到画笔上。
     * @param p 目标画笔，其世界变换决定输出分辨率。
     * @param pageNumber 页码，从 1 开始。
     * @param bounds 需要绘制的目标区域（场景坐标）；为空时绘制整页。
     */
    virtual void render(QPainter *p, int pageNumber, const QRectF &bounds = QRectF()) = 0;

private:
    QAtomicInt mRefCount; ///< 引用计数，attach/detach 维护
    QByteArray mFileData; ///< 文档字节缓存
    QUuid mFileUuid;      ///< 文档标识

    void setRefCount(const QAtomicInt &refCount);
    void setFileData(const QByteArray &fileData);
    void setFileUuid(const QUuid &fileUuid);

    static QMap<QUuid, QPointer<PDFRenderer>> sRenderers; ///< 共享实例表，值为弱引用

protected:
    int dpiForRendering; ///< 渲染 DPI，默认 96
    PDFRenderer();
};
