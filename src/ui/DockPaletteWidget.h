/**
 * @file DockPaletteWidget.h
 * @brief 提供可放入左/右停靠调色板的页面基类。
 */

#pragma once

#include <QWidget>
#include <QPixmap>
#include <QString>

#include <memory>

/** 白板当前处于哪个界面，决定哪些停靠面板可见。 */
enum class DockWidgetMode
{
    Board,
    Web,
    Document,
    Desktop
};

/**
 * @brief 停靠调色板页面的抽象基类：名称、左右图标、可见状态与显示请求。
 *
 * FeaturesWidget、PageNavigationWidget、CachePropertiesWidget、DockDownloadWidget
 * 等都继承它。BoardPaletteManager 创建具体页面后用 DockPalette::registerWidget()
 * 注册，父控件转交给对应的 DockPalette；页面通过 showTab()/hideTab() 请求调色板
 * 增加或移除自己的标签。页面本身不负责停靠布局。
 */
class DockPaletteWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief 构造页面。
     * @param parent 父控件
     * @param name 对象名
     */
    DockPaletteWidget(QWidget *parent = 0, const char *name = "DockPaletteWidget");
    ~DockPaletteWidget();

    /** @brief 展开状态下标签使用的图标（标签在右侧时）。 */
    QPixmap iconToRight();
    /** @brief 收起状态下标签使用的图标（标签在左侧时）。 */
    QPixmap iconToLeft();
    /** @brief 页面名称，用作标签识别与当前页比较。 */
    QString name();

    /**
     * @brief 页面在给定模式下是否应该出现。
     * @param mode 当前界面模式
     * @return 纯虚函数，由子类决定
     */
    virtual bool visibleInMode(DockWidgetMode mode) = 0;

    /**
     * @brief 登记页面支持的界面模式。
     * @param mode 模式
     * @note 只记录，不触发显示状态变化。
     */
    void registerMode(DockWidgetMode mode);

    /** @brief 页面是否处于“应在停靠面板中显示”的逻辑状态。 */
    bool visibleState();
    /**
     * @brief 设置逻辑可见状态，由 DockPalette 在增删标签时维护。
     * @param state 新状态
     */
    void setVisibleState(bool state);

signals:
    /** @brief 请求从停靠面板移除本页标签。 */
    void hideTab(DockPaletteWidget *widget);
    /** @brief 请求在停靠面板中显示本页标签。 */
    void showTab(DockPaletteWidget *widget);

public slots:
    /**
     * @brief 按新模式刷新实际可见性。
     * @param newMode 新的界面模式
     */
    void slot_changeMode(DockWidgetMode newMode);

protected:
    /** @brief 设置标签名称。 */
    void setName(const QString &name);
    /** @brief 设置展开状态下的标签图标。 */
    void setIconToRight(const QPixmap &icon);
    /** @brief 设置收起状态下的标签图标。 */
    void setIconToLeft(const QPixmap &icon);

    struct Private;
    std::unique_ptr<Private> d;
};
