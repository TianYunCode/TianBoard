/**
 * @file MetadataDcSubsetAdaptor.h
 * @brief 读写文档目录下的 metadata.rdf（RDF/DC 元数据子集）。
 */

#pragma once

#include <QtWidgets>

class DocumentProxy;

/**
 * @brief 文档元数据的持久化适配器。
 *
 * metadata.rdf 记录标题、分组、日期、标识、页面尺寸、文件版本与页数。persist() 从
 * DocumentProxy 取值覆盖写出；load() 在读取阶段把 rdf 解析回键值表交给
 * PersistenceManager 构造 DocumentProxy。缺失字段（尺寸、更新时间、版本）在此处
 * 按兼容规则补默认值，因此读取旧文档时不必另行迁移。
 */
class MetadataDcSubsetAdaptor
{
public:
    MetadataDcSubsetAdaptor();
    virtual ~MetadataDcSubsetAdaptor();

    /** @brief 把文档元数据写到 proxy 持久化目录下的 metadata.rdf；目录不存在时跳过。 */
    static void persist(DocumentProxy *proxy);
    /** @brief 读取 pPath 下的 metadata.rdf，返回以 Settings 键组织的元数据。 */
    static QMap<QString, QVariant> load(QString pPath);

    /// RDF 命名空间。
    static const QString nsRdf;
    /// Dublin Core 命名空间。
    static const QString nsDc;
    /// 元数据文件名，固定为 metadata.rdf。
    static const QString metadataFilename;

    /// 文档标识的生成域前缀（UUID 与域名组合成 about/identifier）。
    static const QString rdfIdentifierDomain;
};
