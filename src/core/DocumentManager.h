/**
 * @file DocumentManager.h
 * @brief 声明文档导入/导出适配器的注册表与统一入口。
 */

#pragma once

#include <QtCore>

class ExportAdaptor;
class ImportAdaptor;
class DocumentProxy;

/**
 * @brief 文档管理器：持有全部导入/导出适配器，并按扩展名把请求分发给合适的适配器。
 *
 * 详细说明：进程级单例，父对象为 qApp，首次调用 documentManager() 时创建；适配器都以
 * 本对象为父，随其析构。导入会读写文档包并调用 PersistenceManager，属于 GUI 线程操作。
 */
class DocumentManager : public QObject
{
    Q_OBJECT

public:
    /// 返回进程内唯一实例，首次调用时创建。
    static DocumentManager *documentManager();
    virtual ~DocumentManager();

    /**
     * @brief 生成文件对话框用的导入过滤器。
     * @param notUbx 为 true 时排除文档集格式（tbx）。
     */
    QString importFileFilter(bool notUbx = false);

    /**
     * @brief 列出所有导入适配器支持的扩展名。
     * @param notUbx 为 true 时排除文档集格式（tbx）。
     */
    QStringList importFileExtensions(bool notUbx = false);

    /**
     * @brief 导入一个文档集文件并解包到目标目录。
     * @param Incomingfile 待导入的文档集路径。
     * @param destination 解包目标目录。
     * @return 解包出的文件列表；无可用适配器时为空。
     */
    QFileInfoList importUbx(const QString &Incomingfile, const QString &destination);
    /**
     * @brief 导入单个文件为文档。
     * @param pFile 已打开的待导入文件。
     * @param pGroup 目标文档分组。
     * @return 新建的文档；无适配器或导入失败时为空。
     * @note 导入期间会禁用画板交互。
     */
    DocumentProxy *importFile(const QFile &pFile, const QString &pGroup);

    /**
     * @brief 把多个文件作为页面追加到已有文档。
     * @param pDocument 目标文档。
     * @param fileNames 待导入文件路径列表。
     * @return 成功导入的文件数。
     */
    int addFilesToDocument(DocumentProxy *pDocument, QStringList fileNames);

    /**
     * @brief 把一个目录中的图片导入为新文档。
     * @param pDir 源目录。
     * @param pGroup 目标文档分组。
     * @return 新建文档；目录中没有可导入文件时该文档已被删除。
     */
    DocumentProxy *importDir(const QDir &pDir, const QString &pGroup);
    /**
     * @brief 把目录中的图片按文件名自然序追加到文档。
     * @param pDir 源目录。
     * @param pDocument 目标文档。
     * @return 成功导入的文件数。
     */
    int addImageDirToDocument(const QDir &pDir, DocumentProxy *pDocument);

    /// 返回全部已注册的导出适配器，调用方不得删除。
    QList<ExportAdaptor *> supportedExportAdaptors();
    /// 对外转发 documentUpdated 信号。
    void emitDocumentUpdated(DocumentProxy *pDocument);

signals:
    /// 文档内容或元数据已更新。
    void documentUpdated(DocumentProxy *pDocument);

private:
    /// 私有构造：注册全部导入/导出适配器并接好警告转发。
    DocumentManager(QObject *parent = 0);
    QList<ExportAdaptor *> mExportAdaptors;
    QList<ImportAdaptor *> mImportAdaptors;

    static DocumentManager *sDocumentManager;
};
