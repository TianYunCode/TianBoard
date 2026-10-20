/**
 * @file ImportDocument.h
 * @brief 导入 .tbz 文档包，可选地并入现有文档。
 */

#pragma once

#include <QtGui>
#include "content/ImportAdaptor.h"

class DocumentProxy;

/**
 * @brief .tbz 文档导入器。
 *
 * 先解压到用户文档目录下的唯一子目录，再由 PersistenceManager 包装成 DocumentProxy；
 * addFileToDocument() 则解压到临时目录后把内容并入目标文档，最后清理临时目录。
 */
class ImportDocument : public DocumentBasedImportAdaptor {
    Q_OBJECT;

public:
    ImportDocument(QObject *parent = 0);
    virtual ~ImportDocument();

    /** @brief 支持 "tbz"。 */
    virtual QStringList supportedExtentions();
    /** @brief 文件对话框过滤器 “TianBoard (*.tbz)”。 */
    virtual QString importFileFilter();

    /** @brief 解压并创建新文档；pGroup 作为文档分组名，失败返回 0。 */
    virtual DocumentProxy *importFile(const QFile &pFile, const QString &pGroup);
    /** @brief 把 .tbz 内容合并进 pDocument；失败返回 false。 */
    virtual bool addFileToDocument(DocumentProxy *pDocument, const QFile &pFile);

private:
    /** @brief 把 zip 解到 pDir 下的唯一目录，并通过 documentRoot 返回该目录路径。 */
    bool extractFileToDir(const QFile &pZipFile, const QString &pDir, QString &documentRoot);
};
