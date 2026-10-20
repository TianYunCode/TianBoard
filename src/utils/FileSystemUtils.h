/**
 * @file FileSystemUtils.h
 * @brief 声明目录/文件操作、MIME 判定、临时目录与压缩包处理的静态工具集。
 */

#pragma once

#include <QtCore>
#include <QThread>

#include "utils/Enums.h"

class QuaZipFile;
class ProcessingProgressListener;

/**
 * @brief 文件系统相关的静态工具集。
 *
 * 位于 utils 层（纯工具）：只依赖 Qt 与 qsimplify 的 zip 封装，不引用画板业务类型。
 * 所有函数都是静态的，便于测试；失败时返回 false 或空字符串，不抛异常。
 * 临时目录的登记表是进程级静态状态，因此只能从主线程调用。
 */
class FileSystemUtils : public QObject {
    Q_OBJECT

public:
    /** @brief 构造工具对象；实际上所有功能都是静态的。 */
    FileSystemUtils();
    /** @brief 析构。 */
    virtual ~FileSystemUtils();

    /** @brief 去掉本地文件路径前的 file:// 前缀。 */
    static QString removeLocalFilePrefix(QString input);

    /**
     * @brief 返回随程序部署的资源根目录（etc/、library/ 等所在处）。
     *
     * 部署约定：etc/ library/ customizations/ Language/ 与可执行文件同级，
     * 由 src/app/CMakeLists.txt 的 POST_BUILD 铺好（tst_smoke 按这个前提校验）。
     */
    static QString applicationResourcesDirectory();

    /** @brief 默认临时目录名，取应用名。 */
    static QString defaultTempDirName() { return QCoreApplication::applicationName(); }
    /** @brief 默认临时目录的完整路径。 */
    static QString defaultTempDirPath();
    /**
     * @brief 在系统临时目录下创建一个唯一子目录并登记。
     * @param templateString 目录名前缀，默认为应用名。
     * @param autoDeleteOnExit 为 true 时在进程退出时自动删除。
     */
    static QString createTempDir(const QString &templateString = defaultTempDirName(),
                                 bool autoDeleteOnExit = true);
    /** @brief 清理上次异常退出遗留的同前缀临时目录。 */
    static void cleanupGhostTempFolders(const QString &templateString = defaultTempDirName());

    /** @brief 删除本次会话中创建并登记的全部临时目录。 */
    static void deleteAllTempDirCreatedDuringSession();

    /** @brief 列出目录下的全部条目（不含 . 与 ..）。 */
    static QFileInfoList allElementsInDirectory(const QString &pDirPath);

    /** @brief 列出目录下的文件；isRecurive 为 true 时递归子目录。 */
    static QStringList allFiles(const QString &pDirPath, const bool isRecurive = true);

    /** @brief 递归删除目录，成功返回 true。 */
    static bool deleteDir(const QString &pDirPath);

    /** @brief 递归复制目录，成功返回 true。 */
    static bool copyDir(const QString &pSourceDirPath, const QString &pTargetDirPath);

    /** @brief 移动目录，成功返回 true。 */
    static bool moveDir(const QString &pSourceDirPath, const QString &pTargetDirPath);

    /** @brief 复制单个文件，overwrite 控制是否覆盖已有目标。 */
    static bool copyFile(const QString &source, const QString &destination, bool overwrite = false);

    /** @brief 复制文件或目录，overwrite 控制是否覆盖已有目标。 */
    static bool copy(const QString &source, const QString &Destination, bool overwrite = false);

    /** @brief 把名称中文件系统不接受的字符替换掉。 */
    static QString cleanName(const QString &name);

    /** @brief 把数字补齐到固定位数，用于生成有序文件名。 */
    static QString digitFileFormat(const QString &s, int digit);

    /** @brief 由文档路径推导缩略图路径。 */
    static QString thumbnailPath(const QString &path);

    /** @brief 根据文件名后缀推断 MIME 类型字符串。 */
    static QString mimeTypeFromFileName(const QString &filename);

    /** @brief 根据 MIME 类型字符串返回首选文件后缀。 */
    static QString fileExtensionFromMimeType(const QString &pMimeType);

    /** @brief 把 MIME 类型字符串映射为 MimeType::Enum。 */
    static MimeType::Enum mimeTypeFromString(const QString &typeString);

    /** @brief 由 URL 推断 MimeType::Enum。 */
    static MimeType::Enum mimeTypeFromUrl(const QUrl &url);

    /** @brief 把路径规范化为本地绝对路径。 */
    static QString normalizeFilePath(const QString &pFilePath);

    /** @brief 返回文件名后缀（不含点）。 */
    static QString extension(const QString &filaname);

    /** @brief 返回路径的最后一段。 */
    static QString lastPathComponent(const QString &path);

    /** @brief 在目录下按列表顺序返回第一个存在的文件。 */
    static QString getFirstExistingFileFromList(const QString &path, const QStringList &files);

    /** @brief 判断文件是否为可展开的 zip 包；必要时回写修正后的路径。 */
    static bool isAZipFile(QString &filePath);

    /** @brief 删除单个文件，成功返回 true。 */
    static bool deleteFile(const QString &path);

    /**
     * @brief 把目录内容写入 zip 输出流。
     * @param pRootDocumentFolder 为 true 时按根文档目录布局写入。
     * @param progressListener 可为空，用于回报进度。
     */
    static bool compressDirInZip(const QDir &pDir, const QString &pDestDir, QuaZipFile *pOutZipFile,
                                 bool pRootDocumentFolder, ProcessingProgressListener *progressListener = 0);

    /** @brief 把 zip 解压到目标目录，成功返回 true。 */
    static bool expandZipToDir(const QFile &pZipFile, const QDir &pTargetDir);

    /** @brief 若文件已存在，则生成带序号的不重名文件名。 */
    static QString nextAvailableFileName(const QString &filename, const QString &inter = QString(""));

    /** @brief 以 UTF-8 读取文本文件内容；失败时返回空字符串。 */
    static QString readTextFile(QString path);

private:
    static QStringList sTempDirToCleanUp; ///< 待清理的临时目录列表。
};

/**
 * @brief zip 压缩/解压进度回调接口。
 *
 * 由调用方实现，用于把已处理数量与总数反馈给界面；回调在执行压缩的线程中触发。
 */
class ProcessingProgressListener {
public:
    /** @brief 构造空接口。 */
    ProcessingProgressListener() {}

    /** @brief 虚析构，允许经基类指针释放。 */
    virtual ~ProcessingProgressListener() {}

    /** @brief 进度回调；pOpType 为操作描述，pCurrent/pTotal 为当前与总数。 */
    virtual void processing(const QString &pOpType, int pCurrent, int pTotal) = 0;
};
