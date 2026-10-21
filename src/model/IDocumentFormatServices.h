/**
 * @file IDocumentFormatServices.h
 * @brief 文档格式服务的抽象接口与模型侧调用点声明。
 */

#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>

class DocumentProxy;
class QPixmap;

/**
 * @brief 文档“格式侧”的两件事：读 metadata.rdf，取/刷单页缩略图。
 *
 * 详细说明：真正干活的是 content 层（RDF 解析与缩略图 PNG），但模型反过来 include
 * content 就把依赖方向写反了——content 本来就认识 DocumentProxy。因此接口住在模型侧，
 * 实现（content/DocumentFormatServices）由装配点在启动时装上，见 app/main.cpp。
 * 这一层是**进程级的**，没有实例状态。
 *
 * 所有权：installDocumentFormatServices() 不接管指针，装进去的对象由装配点自己活到底
 * （main.cpp 里是一个 static 对象）。没装上也不会崩：模型侧的四个自由函数会打一条
 * 告警并给出空结果（元数据空表、缩略图空白），表现是“文档库看不到标题与预览”。
 * 线程约束：全部在 GUI 线程调用（元数据随文档操作读写，缩略图随文档库界面刷新）。
 */
class IDocumentFormatServices
{
public:
    virtual ~IDocumentFormatServices()                                                 = default;

    /// @brief 读某份文档目录下的 metadata.rdf，返回键值表；文件缺失或损坏时返回空表。
    virtual QMap<QString, QVariant> loadMetadata(const QString &documentPath)          = 0;

    /// @brief 把文档当前的元数据写回 metadata.rdf（覆盖写出）。
    virtual void persistMetadata(DocumentProxy *proxy)                                 = 0;

    /**
     * @brief 取一页缩略图。
     * @return **新分配**的 QPixmap，调用方负责释放；缩略图文件不存在时先补生成再取。
     */
    virtual const QPixmap *pageThumbnail(DocumentProxy *proxy, int pageIndex)          = 0;

    /**
     * @brief 按页序填满一叠缩略图：先补齐缺失的缩略图，再逐页取。
     * @param out 调用方的容器，本函数负责释放其中旧条目后重填；填进去的都是新分配的图。
     */
    virtual void loadPageThumbnails(DocumentProxy *proxy, QList<const QPixmap *> &out) = 0;
};

/// @brief 装上格式服务；只在应用启动的装配阶段调用一次。
void installDocumentFormatServices(IDocumentFormatServices *services);

/// @brief 读文档元数据；未装上服务时告警一次并返回空表。
QMap<QString, QVariant> loadDocumentMetadata(const QString &documentPath);
/// @brief 写回文档元数据；未装上服务时告警一次并什么都不做。
void persistDocumentMetadata(DocumentProxy *proxy);
/// @brief 取一页缩略图（新分配，调用方释放）；未装上服务时返回一张空图。
const QPixmap *documentPageThumbnail(DocumentProxy *proxy, int pageIndex);
/// @brief 重填一叠页缩略图；未装上服务时清空容器。
void loadDocumentPageThumbnails(DocumentProxy *proxy, QList<const QPixmap *> &out);
