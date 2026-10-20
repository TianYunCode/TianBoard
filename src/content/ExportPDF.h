/**
 * @file ExportPDF.h
 * @brief 以光栅方式把文档逐页渲染进 PDF。
 */

#pragma once

#include <QtCore>
#include "content/ExportAdaptor.h"

class DocumentProxy;

/**
 * @brief 简单 PDF 导出器：每页用 GraphicsScene 渲染到 QPdfWriter。
 *
 * 页面尺寸取场景的 sceneSize，按屏幕 DPI 换算成点；渲染期间临时把背景改为纯白、
 * 切到非屏幕高质量渲染，结束后恢复，避免污染屏幕上正在显示的场景。
 */
class ExportPDF : public ExportAdaptor {
    Q_OBJECT

public:
    ExportPDF(QObject *parent = 0);
    virtual ~ExportPDF();

    /** @brief 菜单文案：“Export to PDF”。 */
    virtual QString exportName();
    /** @brief 扩展名 ".pdf"。 */
    virtual QString exportExtention();
    /** @brief 询问保存路径并导出。 */
    virtual void persist(DocumentProxy *pDocument);
    /** @brief 目录节点不可导出。 */
    virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);

    /** @brief 逐页渲染写出 PDF；失败时返回 false。 */
    virtual bool persistsDocument(DocumentProxy *pDocument, const QString &filename);
};
