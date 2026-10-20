/**
 * @file ImportIwb.h
 * @brief 导入 IWB 1.0 文件（.iwb 压缩包或裸 content.xml）。
 */

#pragma once

#include <QtGui>
#include "content/ImportAdaptor.h"

class DocumentProxy;

/**
 * @brief IWB 按文档导入器。
 *
 * 先解压到临时目录取出 content.xml，再由 IwbSubsetAdaptor 逐页转换并写出标准文档包；
 * 若解压失败则把输入当作裸 XML 直接解析（有些工具导出的 .iwb 实际是未压缩 XML）。
 * 转换结果通过 PersistenceManager 注册成文档。
 */
class ImportIwb : public DocumentBasedImportAdaptor {
    Q_OBJECT;

public:
    ImportIwb(QObject *parent = 0);
    virtual ~ImportIwb();

    /** @brief 支持 "iwb"。 */
    virtual QStringList supportedExtentions();
    /** @brief 文件对话框过滤器 “Interactive Whiteboard File (*.iwb)”。 */
    virtual QString importFileFilter();

    /** @brief 把 IWB 内容合并进已有文档；导入的内容按页追加。 */
    virtual bool addFileToDocument(DocumentProxy *pDocument, const QFile &pFile);
    /** @brief 把 IWB 文件转换为新文档；pGroup 为文档分组名，失败返回 0。 */
    virtual DocumentProxy *importFile(const QFile &pFile, const QString &pGroup);

private:
    /** @brief 解压 IWB 包到临时目录，返回包根目录；不是 zip 时返回空串。 */
    QString expandFileToDir(const QFile &pZipFile, const QString &pDir);
};
