/**
 * @file PropertyPalette.h
 * @brief 属性类调色板的公共基类声明：在 ActionPalette 基础上隐藏抓手。
 */

#pragma once

#include <QMouseEvent>
#include <QPoint>

#include <memory>

#include "ui/ActionPalette.h"

/**
 * @brief 承载“当前工具属性”按钮的调色板基类。
 *
 * 界面上与普通 ActionPalette 相同，只是不显示拖动抓手，适合放在工具栏旁
 * 作为笔色、线宽、橡皮尺寸等属性选择器。它本身不创建按钮，由派生类
 * （如桌面批注的 DesktopPropertyPalette）在构造后填入动作；实例随对应
 * 控制器/父部件一起销毁。
 */
class PropertyPalette : public ActionPalette
{
    Q_OBJECT
public:
    /// 以对象名构造竖向属性调色板。
    PropertyPalette(QWidget *parent = 0, const char *name = "propertyPalette");
    /// 指定排列方向构造属性调色板。
    PropertyPalette(Qt::Orientation orientation, QWidget *parent = 0);
    ~PropertyPalette();

private:
    struct Private;
    std::unique_ptr<Private> d;

private slots:
    /// 鼠标松开时的处理；当前实现为空，保留给派生类的自定义点击逻辑。
    void onMouseRelease();
};
