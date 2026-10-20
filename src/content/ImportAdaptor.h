/**
 * @file ImportAdaptor.h
 * @brief 导入适配器基类，以及“按页导入”和“按文档导入”两类接口。
 */

#pragma once

#include <QtGui>

class GraphicsItem;
class GraphicsScene;
class DocumentProxy;

/**
 * @brief 导入适配器的公共基类。
 *
 * 只保存“导入结果是独立文档还是页面元素”这一标志，供 ImportDocument/ImportImage 等
 * 子类与调用方分派；具体的文件解析、文档包装由子类完成。
 */
class ImportAdaptor : public QObject {
    Q_OBJECT;

protected:
    /** @brief 由子类指定导入类型；documentBased 为 true 表示导入后生成文档。 */
    ImportAdaptor(bool _documentBased, QObject *parent = 0);
    virtual ~ImportAdaptor();

public:
    /** @brief 返回不带点的可导入扩展名列表，用于文件对话框过滤。 */
    virtual QStringList supportedExtentions() = 0;
    /** @brief 返回文件对话框的过滤器文案。 */
    virtual QString importFileFilter() = 0;

    /** @brief 导入结果是否为一份文档（而非页面上的图元）。 */
    bool isDocumentBased() { return documentBased; }

private:
    bool documentBased;
};

/**
 * @brief 按页导入：把文件解析成可放置到当前页的图元。
 *
 * import() 只负责构造图元，placeImportedItemToScene() 负责挂到场景并调整位置；
 * 需要随图元拷贝的媒体目录由 folderToCopy() 给出。
 */
class PageBasedImportAdaptor : public ImportAdaptor {
protected:
    PageBasedImportAdaptor(QObject *parent = 0);

public:
    /** @brief 解析文件并返回图元列表；失败时返回空列表。 */
    virtual QList<GraphicsItem *> import(const QUuid &uuid, const QString &filePath) = 0;
    /** @brief 把 import() 产出的图元放入场景，完成后图元归场景所有。 */
    virtual void placeImportedItemToScene(GraphicsScene *scene, GraphicsItem *item) = 0;
    /** @brief 返回需要一并拷贝到文档目录的资源子目录；无资源时返回空串。 */
    virtual const QString &folderToCopy() = 0;
};

/**
 * @brief 按文档导入：把文件展开成一份完整的 DocumentProxy。
 *
 * 供文档管理器调用：importFile() 生成新文档，addFileToDocument() 把内容并入已有文档。
 */
class DocumentBasedImportAdaptor : public ImportAdaptor {
protected:
    DocumentBasedImportAdaptor(QObject *parent = 0);

public:
    /** @brief 从文件创建新文档；失败时返回 0。 */
    virtual DocumentProxy *importFile(const QFile &pFile, const QString &pGroup) = 0;
    /** @brief 把文件内容合并进已有文档；失败时返回 false。 */
    virtual bool addFileToDocument(DocumentProxy *pDocument, const QFile &pFile) = 0;
};
