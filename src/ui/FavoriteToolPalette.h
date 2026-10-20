/**
 * @file FavoriteToolPalette.h
 * @brief 提供展示内置工具与本地收藏应用快捷方式的浮动调色板。
 */

#pragma once

#include <QtWidgets>

#include <memory>

#include "ui/ActionPalette.h"


/**
 * @brief 收藏工具调色板：以 4 列图标网格展示可拖放到白板的工具和应用。
 *
 * 构造时读取设置中的内置工具 URI 列表与用户收藏目录，生成动作按钮；动作被触发
 * 时把工具的 URL 交给 BoardController::downloadURL()，由下载器加载到当前页。
 * 类本身不在调色板管理器里实例化，保留给需要“收藏工具”入口的调用方使用。
 */
class FavoriteToolPalette : public ActionPalette {
    Q_OBJECT

public:
    /**
     * @brief 构造 4 列网格，填入收藏工具并建立互斥按钮组。
     * @param parent 父控件
     */
    FavoriteToolPalette(QWidget *parent = 0);
    virtual ~FavoriteToolPalette();

private:
    struct Private;
    std::unique_ptr<Private> d;

private slots:
    /** @brief 动作触发：把其 data 中的 URL 下载到当前页。 */
    void addFavorite();
};
