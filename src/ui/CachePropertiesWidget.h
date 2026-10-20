/**
 * @file CachePropertiesWidget.h
 * @brief 提供右侧停靠面板中编辑“遮罩缓存”颜色、形状与尺寸的页面。
 */

#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QVector>

#include "ui/DockPaletteWidget.h"
#include "whiteboard/GraphicsCache.h"

/** 遮罩形状的最大边长（像素），同时作为尺寸滑块的上限。 */
#define MAX_SHAPE_WIDTH 200

/**
 * @brief 遮罩缓存属性面板，作为右侧 DockPalette 的一个标签页。
 *
 * 界面上包含标题、颜色按钮、方/圆形状按钮、尺寸滑块和 Close 按钮。
 * BoardPaletteManager 创建后注册到 RightPalette，但初始不显示；当前白板页面出现
 * 标记为 "Cache" 的 GraphicsCache 时，本页通过 showTab() 自动出现，关闭按钮会
 * 把该缓存从场景和静态缓存列表中移除。面板只在 Board 模式可见。
 */
class CachePropertiesWidget : public DockPaletteWidget
{
    Q_OBJECT
public:
    /**
     * @brief 构造面板并建立全部子控件与信号连接。
     * @param parent 父控件
     * @param name 对象名
     */
    CachePropertiesWidget(QWidget *parent = 0, const char *name = "CachePropertiesWidget");
    ~CachePropertiesWidget();

    /**
     * @brief 该面板只在 Board 模式显示。
     * @param mode 当前界面模式
     * @return mode 为 Board 时返回 true
     */
    bool visibleInMode(DockWidgetMode mode)
    {
        return mode == DockWidgetMode::Board;
    }

public slots:
    /**
     * @brief 在活动场景中查找缓存并同步界面。
     *
     * 找到 "Cache" 条目时启用面板并发出 showTab()，把颜色、形状和尺寸回填到控件；
     * 当前不是 Board 模式或页面没有缓存时发出 hideTab() 并禁用面板。
     */
    void updateCurrentCache();

private slots:
    /** @brief Close 按钮：从场景移除当前缓存并隐藏本标签页。 */
    void onCloseClicked();
    /**
     * @brief 更新颜色按钮上的色块并把颜色应用到当前缓存。
     * @param color 新颜色
     */
    void updateCacheColor(QColor color);
    /** @brief 弹出颜色对话框选择新颜色。 */
    void onColorClicked();
    /** @brief 根据获得焦点的形状按钮切换缓存遮罩形状。 */
    void updateShapeButtons();
    /**
     * @brief 把滑块值应用为当前缓存宽度。
     * @param newSize 新的形状宽度
     */
    void onSizeChanged(int newSize);
    /** @brief 缓存功能被启用时自动显示本标签页。 */
    void onCacheEnabled();

private:
    QVBoxLayout   *mpLayout;
    QLabel        *mpCachePropertiesLabel;
    QLabel        *mpColorLabel;
    QLabel        *mpShapeLabel;
    QLabel        *mpSizeLabel;
    QPushButton   *mpColor;
    QPushButton   *mpSquareButton;
    QPushButton   *mpCircleButton;
    QPushButton   *mpCloseButton;
    QSlider       *mpSizeSlider;
    QHBoxLayout   *mpColorLayout;
    QHBoxLayout   *mpShapeLayout;
    QHBoxLayout   *mpSizeLayout;
    QHBoxLayout   *mpCloseLayout;
    QWidget       *mpProperties;
    QVBoxLayout   *mpPropertiesLayout;
    QColor         mActualColor;
    eMaskShape     mActualShape;
    GraphicsCache *mpCurrentCache;
};
