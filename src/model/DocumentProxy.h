/**
 * @file DocumentProxy.h
 * @brief 声明文档代理 DocumentProxy：一份文档（元数据、页数、持久化路径）的轻量句柄。
 */

#pragma once

#include <QtWidgets>

#include "utils/StringUtils.h"

#include "settings/Settings.h"

class GraphicsScene;

/**
 * @brief 文档代理：封装单个文档的持久化路径、元数据、页数与 UUID。
 *
 * 位于 model 域的文档身份层，是 DocumentTreeModel 树节点、DocumentContainer 当前文档
 * 与 BoardController 活动文档共同引用的“文档句柄”。代理本身不加载页面内容，只读写
 * 元数据；内容由 PersistenceManager 按 persistencePath 定位。多个代理可通过相互复制
 * 表示同一份文档，相等性由持久化路径决定。
 */
class DocumentProxy : public QObject
{
    Q_OBJECT

    friend class PersistenceManager;

public:
    /**
     * @brief 创建一个全新的内存文档代理：生成 UUID、默认名称与默认页面尺寸。
     *
     * 此时还没有持久化路径，第一次保存时才由 PersistenceManager 分配。
     */
    DocumentProxy();

    /**
     * @brief 拷贝构造：复制持久化路径、元数据、修改标志与页数，不建立父子关系。
     */
    DocumentProxy(const DocumentProxy &rValue);

    /**
     * @brief 从磁盘上的文档目录构造代理。
     * @param pPersistencePath 文档在磁盘上的目录路径。
     * @note 构造时会读取 dc 元数据子集。
     */
    DocumentProxy(const QString &pPersistencePath);

    /**
     * @brief 用现成的元数据构造代理，不读取磁盘。
     * @param pPersistencePath 文档目录路径。
     * @param metadatas 初始元数据表。
     */
    DocumentProxy(const QString &pPersistencePath, QMap<QString, QVariant> metadatas);

    /**
     * @brief 析构。代理不拥有页面内容，因此没有额外清理。
     */
    virtual ~DocumentProxy();

    /**
     * @brief 创建内容相同但独立的代理副本。
     * @return 新分配的代理，调用方负责释放。
     */
    DocumentProxy *deepCopy() const;

    /**
     * @brief 判断两个代理是否指向同一份文档。
     * @param proxy 待比较的代理，可为空。
     * @return 持久化路径相同即视为同一文档；proxy 为空时返回 false。
     */
    bool theSameDocument(DocumentProxy *proxy);

    /** @brief 返回文档在磁盘上的目录路径。 */
    QString persistencePath() const;

    /**
     * @brief 设置文档目录路径。
     * @note 路径变化会把代理标记为已修改。
     */
    void setPersistencePath(const QString &pPersistencePath);

    /**
     * @brief 写入一项元数据。
     * @param pKey 元数据键，通常取自 Settings 中的常量。
     * @param pValue 新值。
     * @note 值未变化时不写并提前返回；写入 updatedAt 时还会通知 DocumentManager
     *       发出文档更新信号。
     */
    void setMetaData(const QString &pKey, const QVariant &pValue);

    /**
     * @brief 读取一项元数据。
     * @return 键不存在时返回空字符串并打印警告。
     */
    QVariant metaData(const QString &pKey) const;

    /** @brief 返回全部元数据的副本。 */
    QMap<QString, QVariant> metaDatas() const;

    /** @brief 返回文档显示名，取自元数据。 */
    QString name() const;

    /** @brief 返回文档所属分组（虚拟目录）名，取自元数据。 */
    QString groupName() const;

    /** @brief 返回文档创建时间；元数据缺失时退回当前时间。 */
    QDateTime documentDate();

    /** @brief 返回文档最后更新时间；元数据缺失时退回当前时间。 */
    QDateTime lastUpdate();

    /** @brief 返回文档默认页面尺寸；元数据缺失时退回全局设置值。 */
    QSize defaultDocumentSize() const;

    /** @brief 设置默认页面尺寸，会同时把代理标记为已修改。 */
    void setDefaultDocumentSize(QSize pSize);

    /** @brief 以宽高设置默认页面尺寸。 */
    void setDefaultDocumentSize(int pWidth, int pHeight);

    /**
     * @brief 返回文档 UUID。
     *
     * 元数据里存的是命名空间 URI + 规范化 UUID，这里去掉前缀后解析。
     */
    QUuid uuid() const;

    /** @brief 以“命名空间 URI/规范化 UUID”的形式写入文档标识元数据。 */
    void setUuid(const QUuid &uuid);

    /**
     * @brief 元数据是否被改动过，供保存时决定要不要重写 metadata.rdf。
     * @note 旗标只置真、没有清零路径：一旦改过元数据，之后每次保存都会重写元数据并
     *       刷新 updated-at。这是"updated-at 表示最后一次保存时间"这一语义所要求的，
     *       不能当成"还有未保存的改动"来用（真要那样必须先给保存成功加一条清零）。
     */
    bool isModified() const;

    /** @brief 返回文档的页数。 */
    int pageCount();

    /** @brief 返回文档页面的 DPI。 */
    int pageDpi();

    /** @brief 设置文档页面的 DPI。 */
    void setPageDpi(int dpi);

protected:
    /** @brief 直接设置页数，供持久化层加载时使用。 */
    void setPageCount(int pPageCount);

    /** @brief 页数加一；从 0 开始时结果至少为 1。 */
    int incPageCount();

    /** @brief 页数减一，下限为 0。 */
    int decPageCount();

private:
    /** @brief 初始化默认元数据：分组名、默认名称、UUID 与默认页面尺寸。 */
    void init();

    /** @brief 文档目录路径。 */
    QString mPersistencePath;

    /** @brief 文档元数据表，键由 Settings 定义。 */
    QMap<QString, QVariant> mMetaDatas;

    /** @brief 元数据改动旗标；置真后没有清零路径，见 isModified() 的说明。 */
    bool mIsModified;

    /** @brief 页数。 */
    int mPageCount;

    /** @brief 页面 DPI，0 表示未设置。 */
    int mPageDpi;
};

/**
 * @brief 按持久化路径比较两个文档代理是否指向同一文档。
 */
inline bool operator==(const DocumentProxy &proxy1, const DocumentProxy &proxy2)
{
    return proxy1.persistencePath() == proxy2.persistencePath();
}

/**
 * @brief 以持久化路径为键计算哈希，使 DocumentProxy 可用于 QHash/QSet。
 */
inline size_t qHash(const DocumentProxy &key, size_t seed = 0)
{
    return qHash(key.persistencePath(), seed);
}
