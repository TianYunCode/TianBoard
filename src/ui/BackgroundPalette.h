/**
 * @file BackgroundPalette.h
 * @brief 提供设置背景网格尺寸的浮动调色板。
 */

#pragma once

#include <memory>

#include "ui/ActionPalette.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsScene.h"

/**
 * @brief 背景设置浮动调色板：上方是背景动作按钮，下方是网格尺寸滑块与复位按钮。
 *
 * BoardPaletteManager 在 setupPalettes() 中用背景动作创建并持有本类实例，并把它
 * 与背景切换动作连接起来；面板作用于 BoardController 当前活动场景
 * （GraphicsScene）的背景网格。显示时会按当前场景的深浅色调整标签文字颜色。
 */
class BackgroundPalette : public ActionPalette
{
    Q_OBJECT

public:
    /**
     * @brief 用给定动作构造背景调色板。
     * @param actions 显示在顶部的动作（背景样式等）
     * @param parent 父控件
     */
    BackgroundPalette(QList<QAction *> actions, QWidget *parent = 0);
    /**
     * @brief 构造只有网格尺寸控制区的背景调色板。
     * @param parent 父控件
     */
    BackgroundPalette(QWidget *parent = 0);
    ~BackgroundPalette() override;

    /**
     * @brief 在顶部动作行追加一个按钮。
     * @param action 要追加的动作
     * @note 与基类不同，按钮放入私有顶部布局而不是主布局。
     */
    void addAction(QAction *action);
    /**
     * @brief 清空映射后按列表重建顶部按钮。
     * @param actions 动作列表
     */
    void setActions(QList<QAction *> actions);
    /** @brief 删除顶部/底部两行里的控件并清空动作列表。 */
    void clearLayout();

public slots:
    /**
     * @brief 显示时同步当前场景的网格尺寸并开始跟踪滑块。
     * @param event 显示事件
     * @note 每次显示都会重新 connect 滑块；Qt 会忽略重复连接。
     */
    void showEvent(QShowEvent *event);
    /** @brief 按当前场景背景深浅切换“Grid size”标签颜色。 */
    void backgroundChanged();
    /** @brief 重新读取当前场景的网格尺寸并刷新标签颜色。 */
    void refresh();

protected slots:
    /**
     * @brief 滑块移动时写入当前场景并记住到设置中。
     * @param value 新的网格尺寸
     */
    void sliderValueChanged(int value);
    /** @brief 把滑块恢复为设置里的默认网格尺寸。 */
    void defaultBackgroundGridSize();

protected:
    /** @brief 按按钮样式调整私有竖向布局的边距。 */
    virtual void updateLayout();
    /** @brief 建立滑块、标签和复位按钮所在的上下两行布局。 */
    void init();

    struct Private;
    std::unique_ptr<Private> d;
};
