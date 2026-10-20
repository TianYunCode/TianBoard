/**
 * @file ImportDocumentSetAdaptor.h
 * @brief 导入 .tbx 文档集，把包内各文档目录复制到目标位置。
 */

#pragma once

#include <QtGui>
#include "content/ImportAdaptor.h"

class DocumentProxy;

/**
 * @brief .tbx 文档集导入器。
 *
 * 先解压到系统临时目录，再把顶层目录逐个复制到目标文档库；同名时用
 * PersistenceManager 生成唯一目录名，避免覆盖已有文档。导入结果是一组目录，
 * 由调用方决定如何刷新文档树。
 */
class ImportDocumentSetAdaptor : public ImportAdaptor {
    Q_OBJECT

public:
    ImportDocumentSetAdaptor(QObject *parent = 0);
    virtual ~ImportDocumentSetAdaptor();

    /** @brief 支持 "tbx"。 */
    virtual QStringList supportedExtentions();
    /** @brief 文件对话框过滤器 “TianBoard Document Set (*.tbx)”。 */
    virtual QString importFileFilter();

    /** @brief 解压 zipFile 并把顶层文档目录复制到 destination，返回新目录信息列表。 */
    QFileInfoList importData(const QString &zipFile, const QString &destination);

private:
    /** @brief 把文档集 zip 完整解到 pDir；失败返回 false。 */
    bool extractFileToDir(const QFile &pZipFile, const QString &pDir);
};
