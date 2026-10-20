/**
 * @file ForeignObjectsHandler.h
 * @brief 声明文档包内页面外部对象（媒体、PDF、控件）的修复与页面复制工具。
 */

#pragma once

#include <QList>
#include <QUrl>
#include <algorithm>

class ForeighnObjectsHandlerPrivate;

/**
 * @brief 页面外部对象处理门面：清理文档包中未被引用的资源，并在复制页面时同步复制其资源。
 *
 * 详细说明：无 QObject 基类，按需在栈上创建；实现全部放在私有类中（d 指针），
 * 本类只做转发并拥有 d。所有操作都是同步文件 I/O，调用方需自行决定放在哪个线程。
 */
class ForeighnObjectsHandler {
public:
    ForeighnObjectsHandler();
    ~ForeighnObjectsHandler();

    /**
     * @brief 依次处理多个文档目录。
     * @param dirs 文档包目录列表。
     */
    void cure(const QList<QUrl> &dirs);
    /**
     * @brief 修复单个文档目录：清理未引用资源并校正来源路径。
     * @param dir 文档包目录。
     */
    void cure(const QUrl &dir);

    /**
     * @brief 复制一页到另一个文档，并复制该页引用的媒体资源（生成新的资源标识）。
     * @param fromDir 源文档目录。
     * @param fromIndex 源页下标。
     * @param toDir 目标文档目录。
     * @param toIndex 目标页下标，同时用于重写 PDF 的页锚点。
     */
    void copyPage(const QUrl &fromDir, int fromIndex, const QUrl &toDir, int toIndex);

private:
    ForeighnObjectsHandlerPrivate *d;

    friend class ForeighnObjectsHandlerPrivate;
};
