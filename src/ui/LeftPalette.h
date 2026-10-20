/**
 * @file LeftPalette.h
 * @brief 提供左侧停靠面板，并持久化其宽度与折叠状态。
 */

#pragma once

#include "ui/DockPalette.h"

#include <memory>


/**
 * @brief 左侧停靠面板。
 *
 * BoardPaletteManager 创建并持有，放在白板容器左缘，承载页面导航等标签页。
 * 构造时和每次切换模式时从设置中恢复宽度与是否折叠；用户拖动改宽后立即写回设置，
 * 分别记录 Board 与 Desktop 两种模式下的宽度。
 */
class LeftPalette : public DockPalette {
public:
    /**
     * @brief 构造左侧面板并恢复上次的宽度/折叠状态。
     * @param parent 容器控件
     * @param name 对象名
     */
    LeftPalette(QWidget *parent = 0, const char *name = "LeftPalette");
    ~LeftPalette();

    /**
     * @brief 切换模式前先恢复对应模式保存的宽度。
     * @param mode 目标模式
     * @return 基类 switchMode() 的结果
     */
    bool switchMode(DockWidgetMode mode);

public slots:
    /**
     * @brief 设置新文档后强制记住 Board 模式的第一个标签页。
     * @param documentProxy 新文档（未使用）
     */
    void onDocumentSet(DocumentProxy *documentProxy);

protected:
    /** @brief 最大宽度限制为父控件宽度的 45%。 */
    void updateMaxWidth();
    /** @brief 改宽时把宽度与折叠状态写回设置。 */
    void resizeEvent(QResizeEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
