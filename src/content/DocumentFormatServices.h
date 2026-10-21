/**
 * @file DocumentFormatServices.h
 * @brief 文档格式服务的 content 实现声明。
 */

#pragma once

#include "model/IDocumentFormatServices.h"

/**
 * @brief 把模型侧的格式服务请求转给 content 里现成的元数据读写器与缩略图读写器。
 *
 * 详细说明：这一层没有任何新逻辑，只是把 `model/IDocumentFormatServices` 的四个方法接到
 * `MetadataDcSubsetAdaptor` 与 `ThumbnailAdaptor` 的静态入口上——存在的意义是让依赖方向
 * 正确：content 认识 DocumentProxy（它本来就是读写文档的），模型不需要认识 content。
 * 无状态，装配点装一个 static 实例即可（见 app/main.cpp）。
 * 线程约束：只在 GUI 线程被调用，与底层读写器一致。
 */
class DocumentFormatServices : public IDocumentFormatServices
{
public:
    /// @brief 转给 MetadataDcSubsetAdaptor::load()。
    QMap<QString, QVariant> loadMetadata(const QString &documentPath) override;
    /// @brief 转给 MetadataDcSubsetAdaptor::persist()。
    void persistMetadata(DocumentProxy *proxy) override;
    /// @brief 转给 ThumbnailAdaptor::get()（返回值同样是新分配，调用方释放）。
    const QPixmap *pageThumbnail(DocumentProxy *proxy, int pageIndex) override;
    /// @brief 转给 ThumbnailAdaptor::load()（先补齐缺失缩略图，再逐页取）。
    void loadPageThumbnails(DocumentProxy *proxy, QList<const QPixmap *> &out) override;
};
