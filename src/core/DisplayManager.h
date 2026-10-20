/**
 * @file DisplayManager.h
 * @brief 声明多屏管理器：分配控制屏/展示屏/历史屏角色并安放对应窗口。
 */

#pragma once

#include <QtWidgets>

class BlackoutWidget;
class BoardView;
class VirtualDesktop;

/**
 * @brief 多屏管理器：根据屏幕数量与设置决定各屏幕的角色，并负责窗口几何与全屏切换。
 *
 * 详细说明：由 ApplicationController 持有，挂在 QObject 父对象下；它只操作外部传入的
 * 控件，不取得所有权。屏幕信息统一从 VirtualDesktop 读取，避免各模块自行枚举 QScreen。
 * 线程约束：GUI 线程专用。
 */
class DisplayManager : public QObject {
    Q_OBJECT

public:
    /// 构造管理器并连接屏幕几何变化信号，初始角色由设置与屏幕数量决定。
    DisplayManager(QObject *parent = 0);
    virtual ~DisplayManager();

    /// 返回当前屏幕数量。
    int numScreens();

    /// 返回已分配为“历史展示屏”的屏幕数量。
    int numPreviousViews();

    /**
     * @brief 注册控制屏窗口。
     * @param pControlWidget 要全屏铺到控制屏的控件。
     */
    void setControlWidget(QWidget *pControlWidget);

    /**
     * @brief 注册展示屏窗口。
     * @param pDisplayWidget 要全屏铺到展示屏的控件。
     */
    void setDisplayWidget(QWidget *pDisplayWidget);

    /**
     * @brief 注册桌面批注窗口。
     * @param pControlWidget 桌面批注层窗口。
     */
    void setDesktopWidget(QWidget *pControlWidget);

    /**
     * @brief 注册历史屏使用的画板视图列表。
     * @param pPreviousViews 与历史屏按下标一一对应的视图列表。
     */
    void setPreviousDisplaysWidgets(QList<BoardView *> pPreviousViews);

    /// 是否存在可用的控制屏。
    bool hasControl() { return mControlScreenIndex > -1; }

    /// 是否存在可用的展示屏。
    bool hasDisplay() { return mDisplayScreenIndex > -1; }

    /// 是否分配了至少一个历史屏。
    bool hasPrevious() { return !mPreviousScreenIndexes.isEmpty(); }

    /// 屏幕角色；Previous1..5 表示第 N 块额外的历史展示屏。
    enum DisplayRole {
        None = 0,  ///< 未分配角色
        Control,   ///< 控制屏
        Display,   ///< 展示屏
        Previous1, ///< 第 1 块历史屏
        Previous2, ///< 第 2 块历史屏
        Previous3, ///< 第 3 块历史屏
        Previous4, ///< 第 4 块历史屏
        Previous5  ///< 第 5 块历史屏
    };

    /// 是否启用多屏模式。
    bool useMultiScreen() { return mUseMultiScreen; }

    /// 设置多屏模式开关；只记录状态，重新布局需另行触发。
    void setUseMultiScreen(bool pUse);

    /// 返回控制屏下标，-1 表示未分配。
    int controleScreenIndex() { return mControlScreenIndex; }

    /// 返回展示屏下标，-1 表示未分配。
    int displayScreenIndex() const { return mDisplayScreenIndex; }

    /// 返回控制屏的几何区域。
    QRect controlGeometry();
    /// 返回展示屏的几何区域。
    QRect displayGeometry();

signals:
    /// 屏幕布局（角色或几何）发生变化。
    void screenLayoutChanged();
    /// 展示视图需要重新适配其所在屏幕。
    void adjustDisplayViewsRequired();

public slots:
    /**
     * @brief 重新枚举屏幕并安放各窗口。
     * @param bswap 预留参数，目前未使用。
     */
    void reinitScreens(bool bswap);

    /**
     * @brief 某块屏幕的几何或工作区变化后重新布局。
     * @param screen 变化的屏幕下标，目前未使用。
     */
    void adjustScreens(int screen);

    /// 在所有可见屏幕上覆盖黑屏窗口。
    void blackout();

    /// 关闭全部黑屏窗口并解除网页控件冻结。
    void unBlackout();

    /**
     * @brief 手工指定某块屏幕的角色。
     * @param role 目标角色。
     * @param screenIndex 屏幕下标。
     * @note 当前实现为空，保留接口。
     */
    void setRoleToScreen(DisplayRole role, int screenIndex);

    /**
     * @brief 交换展示屏与第一块历史屏。
     * @param swap 是否执行交换。
     */
    void swapDisplayScreens(bool swap);

private:
    /// 按当前下标把各窗口移动并全屏到对应屏幕。
    void positionScreens();

    /// 依据屏幕数量与设置重新计算控制屏/展示屏/历史屏下标。
    void initScreenIndexes();

    int mControlScreenIndex;

    int mDisplayScreenIndex;

    QList<int> mPreviousScreenIndexes;

    /// 屏幕信息服务，由 VirtualDesktop 单例提供，不持有所有权。
    VirtualDesktop *mDesktop;

    QWidget *mControlWidget;

    QWidget *mDisplayWidget;

    QWidget *mDesktopWidget;

    QList<BoardView *> mPreviousDisplayWidgets;

    QList<BlackoutWidget *> mBlackoutWidgets;

    QList<DisplayRole> mScreenIndexesRoles;

    bool mUseMultiScreen;
};
