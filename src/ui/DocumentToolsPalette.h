/**
 * @file DocumentToolsPalette.h
 * @brief 提供文档编辑界面中承载工具动作的浮动调色板。
 */

#pragma once

#include <QButtonGroup>
#include <QUrl>
#include <QMap>

#include <memory>

#include "ui/ActionPalette.h"


/**
 * @brief 文档模式工具调色板：只放与文档编辑相关的动作按钮。
 *
 * DocumentController 创建并持有本实例，停靠在文档控件右上角；当前只有平台支持
 * 虚拟键盘时才加入“虚拟键盘”动作。isEmpty() 用于判断是否还有按钮可显示。
 */
class DocumentToolsPalette : public ActionPalette {
    Q_OBJECT

public:
    /**
     * @brief 构造调色板并按平台能力填充动作。
     * @param parent 父控件
     */
    DocumentToolsPalette(QWidget *parent = 0);
    virtual ~DocumentToolsPalette();
    /** @brief 动作列表是否为空。 */
    bool isEmpty() { return actions().count() == 0; }

private:
    struct Private;
    std::unique_ptr<Private> d;
};
