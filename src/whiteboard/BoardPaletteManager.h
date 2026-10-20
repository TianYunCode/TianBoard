/**
 * @file BoardPaletteManager.h
 * @brief 画板侧边调色板的创建、布局与交互协调声明。
 */

#pragma once

#include <QtWidgets>
#include <QtWebEngineWidgets>

#include "ui/LeftPalette.h"
#include "ui/RightPalette.h"
#include "ui/PageNavigationWidget.h"
#include "ui/CachePropertiesWidget.h"
#include "ui/DockDownloadWidget.h"
#include "core/ApplicationController.h"
#include "ui/FeaturesWidget.h"


class WebToolsPalette;
class StylusPalette;
class PageNumberPalette;
class ZoomPalette;
class ActionPalette;
class BackgroundPalette;
class BoardController;
class ServerXMLHttpRequest;
class KeyboardPalette;
class MainWindow;
class ApplicationController;


/**
 * @brief 管理画板四周的调色板：笔/橡皮/背景/页面/缩放/工具/添加素材等，
 * 并处理它们的停靠模式、显示隐藏与长按展开行为。
 *
 * 由 BoardController 创建并持有，容器控件与控制器都只保存裸指针。
 * 调色板对象以容器为父控件，随界面销毁；与 BoardController、DrawingController
 * 的信号连接在本类构造时建立，切换场景/模式时刷新可见性。
 *
 * 线程约束：只在 GUI 线程使用。
 */
class BoardPaletteManager : public QObject {
    Q_OBJECT

public:
    /**
     * @param container 调色板挂载的父控件，不接管所有权。
     * @param controller 画板控制器，用于页面、缩放与工具操作。
     */
    BoardPaletteManager(QWidget *container, BoardController *controller);
    virtual ~BoardPaletteManager();

    /// @brief 建立全部调色板并完成初始布局与连接。
    void setupLayout();
    /// @brief 左侧调色板。
    LeftPalette *leftPalette() { return mLeftPalette; }
    /// @brief 右侧调色板。
    RightPalette *rightPalette() { return mRightPalette; }
    /// @brief 手写笔（颜色/线宽）调色板。
    StylusPalette *stylusPalette() { return mStylusPalette; }
    /// @brief “添加素材”调色板。
    ActionPalette *addItemPalette() { return mAddItemPalette; }
    /// @brief 显示/隐藏虚拟键盘调色板。
    void showVirtualKeyboard(bool show = true);
    /// @brief 按保存的位置初始化各调色板的位置。
    void initPalettesPosAtStartup();
    /// @brief 刷新调色板内容（颜色、图标、可用状态）。
    void refreshPalettes();

    KeyboardPalette *mKeyboardPalette; ///< 虚拟键盘调色板

    /// @brief 记录当前打开的网页工具调色板，用于互斥关闭。
    void setCurrentWebToolsPalette(WebToolsPalette *palette) { mWebToolsCurrentPalette = palette; }
    WebToolsPalette *mWebToolsCurrentPalette; ///< 当前打开的网页工具调色板，可为空

    /// @brief 按停靠模式处理单个调色板的显示与定位。
    void processPalettersWidget(DockPalette *paletter, DockWidgetMode mode);
    /**
     * @brief 切换整体停靠模式。
     * @param newMode 目标模式。
     * @param isInit 是否为启动时的初始化调用（跳过部分动画）。
     */
    void changeMode(DockWidgetMode newMode, bool isInit = false);
    /// @brief 开始跟踪下载进度并在界面上显示。
    void startDownloads();
    /// @brief 停止下载进度显示。
    void stopDownloads();

public slots:
    /// @brief 活动场景变化后刷新页面/背景相关调色板。
    void activeSceneChanged();
    /// @brief 容器尺寸变化时重新布局调色板。
    void containerResized();
    /// @brief 把 URL 内容添加到当前页。
    void addItem(const QUrl &pUrl);
    /**
     * @brief 把位图添加到页面或素材库。
     * @param pPixmap 图像内容。
     * @param p 插入位置（场景坐标）。
     * @param scale 缩放比例。
     * @param sourceUrl 来源地址，可为空。
     */
    void addItem(const QPixmap &pPixmap, const QPointF &p = QPointF(0.0, 0.0), qreal scale = 1.0,
                 const QUrl &sourceUrl = QUrl());

    /// @brief 主模式切换后调整调色板集合。
    void slot_changeMainMode(ApplicationController::MainMode);
    /// @brief 桌面模式切换后调整调色板集合。
    void slot_changeDesktopMode(bool);

    /// @brief 展开/收起橡皮调色板。
    void toggleErasePalette(bool ckecked);

private:
    /// @brief 创建各类调色板对象。
    void setupPalettes();
    /// @brief 建立调色板与控制器之间的信号连接。
    void connectPalettes();
    /// @brief 外接屏存在时放置自由浮动的演示调色板。
    void positionFreeDisplayPalette();
    /// @brief 创建并组装停靠式调色板控件。
    void setupDockPaletteWidgets();

    QWidget *mContainer;              ///< 调色板容器，不持有所有权
    BoardController *mBoardControler; ///< 画板控制器，不持有所有权

    StylusPalette *mStylusPalette; ///< 手写笔调色板

    ZoomPalette *mZoomPalette;   ///< 缩放调色板
    LeftPalette *mLeftPalette;   ///< 左侧调色板
    RightPalette *mRightPalette; ///< 右侧调色板

    BackgroundPalette *mBackgroundsPalette; ///< 背景调色板
    ActionPalette *mToolsPalette;           ///< 工具调色板
    ActionPalette *mAddItemPalette;         ///< 添加素材调色板
    ActionPalette *mErasePalette;           ///< 橡皮调色板
    ActionPalette *mPagePalette;            ///< 页导航调色板

    QUrl mItemUrl;      ///< 待添加素材的 URL
    QPixmap mPixmap;    ///< 待添加的位图
    QPointF mPos;       ///< 待添加素材的位置
    qreal mScaleFactor; ///< 待添加素材的缩放比例

    QTime mPageButtonPressedTime;   ///< 页按钮按下时刻，用于区分单击与长按
    bool mPendingPageButtonPressed; ///< 页按钮是否处于长按等待状态

    QTime mZoomButtonPressedTime;   ///< 缩放按钮按下时刻
    bool mPendingZoomButtonPressed; ///< 缩放按钮是否处于长按等待状态

    QTime mPanButtonPressedTime;   ///< 平移按钮按下时刻
    bool mPendingPanButtonPressed; ///< 平移按钮是否处于长按等待状态

    QTime mEraseButtonPressedTime;   ///< 橡皮按钮按下时刻
    bool mPendingEraseButtonPressed; ///< 橡皮按钮是否处于长按等待状态

    PageNavigationWidget *mpPageNavigWidget; ///< 页导航控件

    CachePropertiesWidget *mpCachePropWidget; ///< 遮罩属性面板

    FeaturesWidget *mpFeaturesWidget; ///< 素材库面板

    DockDownloadWidget *mpDownloadWidget; ///< 下载进度面板

    bool mDownloadInProgress; ///< 是否有下载正在进行

private slots:
    /// @brief 打开背景选择。
    void changeBackground();

    /// @brief 展开/收起背景调色板。
    void toggleBackgroundPalette(bool checked);
    /// @brief 背景调色板关闭后的状态同步。
    void backgroundPaletteClosed();

    /// @brief 展开/收起手写笔调色板。
    void toggleStylusPalette(bool checked);

    /// @brief 橡皮按钮按下：记录时间并进入长按等待。
    void erasePaletteButtonPressed();
    /// @brief 橡皮按钮松开：区分单击与长按。
    void erasePaletteButtonReleased();

    /// @brief 橡皮调色板关闭后的状态同步。
    void erasePaletteClosed();

    /// @brief 展开/收起页导航调色板。
    void togglePagePalette(bool ckecked);
    /// @brief 页调色板关闭后的状态同步。
    void pagePaletteClosed();

    /// @brief 页按钮按下：记录时间并进入长按等待。
    void pagePaletteButtonPressed();
    /// @brief 页按钮松开：区分单击与长按。
    void pagePaletteButtonReleased();

    /// @brief 把待添加素材加入当前页。
    void addItemToCurrentPage();
    /// @brief 把待添加素材加入新页。
    void addItemToNewPage();
    /// @brief 把待添加素材加入素材库。
    void addItemToLibrary();

    /// @brief 打开购买链接。
    void purchaseLinkActivated(const QString &);

    /// @brief 打开网页工具中的链接。
    void linkClicked(const QUrl &url);

    /// @brief 缩放按钮按下：记录时间并进入长按等待。
    void zoomButtonPressed();
    /// @brief 缩放按钮松开：区分单击与长按。
    void zoomButtonReleased();
    /// @brief 平移按钮按下：记录时间并进入长按等待。
    void panButtonPressed();
    /// @brief 平移按钮松开：区分单击与长按。
    void panButtonReleased();

    /// @brief 切换手写笔调色板方向。
    void changeStylusPaletteOrientation(QVariant var);
};
