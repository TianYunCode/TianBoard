/**
 * @file ExportWeb.h
 * @brief 把文档目录连同 Web 播放器页面导出为可直接浏览的目录。
 */

#pragma once

#include <QtCore>

#include "content/ExportAdaptor.h"

class DocumentProxy;

/**
 * @brief Web 导出器：复制文档持久化目录并放入 index.html 播放器。
 *
 * 输出不是单文件，而是一个目录；完成后用系统浏览器打开 index.html。
 * 播放器页面来自资源 :www/TianBoard-web-player.html，与文档数据格式无关。
 */
class ExportWeb : public ExportAdaptor
{
    Q_OBJECT;

public:
    ExportWeb(QObject *parent = 0);
    virtual ~ExportWeb();

    /** @brief 菜单文案：“Export to Web Browser”。 */
    virtual QString exportName();
    /** @brief 询问目录、复制文档目录并放入播放器页面。 */
    virtual void persist(DocumentProxy *pDocument, const QModelIndex &selectedTreeIndex);
};
