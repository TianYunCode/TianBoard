/**
 * @file BoardController.h
 * @brief 画板主控制器声明：持有双视图、活动场景、调色板管理与页面级操作。
 */

#pragma once

#include <QtWidgets>

#include <QObject>
#include <QHBoxLayout>
#include <QUndoCommand>

#include "model/DocumentContainer.h"
#include "core/ApplicationController.h"

class MainWindow;
class Application;
class BoardView;

class DocumentController;
class MessageWindow;
class GraphicsScene;
class DocumentProxy;
class BlackoutWidget;
class ToolWidget;
class Version;
class GraphicsMediaItem;
class GraphicsWidgetItem;
class BoardPaletteManager;
class Item;
class GraphicsItem;

/**
 * @brief 画板的核心控制器：协调控制视图与演示视图、活动场景、页面切换、
 * 缩放、调色板、工具、下载与自动保存。
 *
 * 继承 DocumentContainer 复用文档缩略图管理。由 Application 创建并持有，
 * 生命周期覆盖整个应用；BoardView、GraphicsScene、BoardPaletteManager 等
 * 都由它创建或挂接，场景切换时只更新 mActiveScene 指针。
 *
 * 线程约束：构造、场景操作与 UI 更新都必须在 GUI 线程执行；下载完成回调
 * 经 Application 转发回本类槽函数后再触碰图元。
 */
class BoardController : public DocumentContainer
{
    Q_OBJECT

public:
    /// @brief 保存选项位标志。
    enum SaveFlag
    {
        sf_none         = 0x0, ///< 无额外选项
        sf_showProgress = 0x1  ///< 显示保存进度提示
    };
    Q_DECLARE_FLAGS(SaveFlags, SaveFlag)

public:
    /// @brief 构造控制器并绑定主窗口；真正的界面搭建在 init()/setupLayout() 中完成。
    BoardController(MainWindow *mainWindow);
    virtual ~BoardController();

    /// @brief 初始化视图、工具栏、调色板与自动保存等子系统。
    void init();
    /// @brief 建立控制视图与演示视图的布局。
    void setupLayout();

    /// @brief 当前活动场景（只读版本）。
    GraphicsScene *activeScene() const;
    /// @brief 当前活动场景的索引。
    int activeSceneIndex() const;
    /// @brief 切换活动场景索引，并同步视图与调色板。
    void setActiveSceneIndex(int i);
    /// @brief 演示视图的视口尺寸。
    QSize displayViewport();
    /// @brief 控制视图的视口尺寸。
    QSize controlViewport();
    /// @brief 控制视图在屏幕上的几何区域。
    QRectF controlGeometry();
    /// @brief 应用关闭前的收尾：停止自动保存、持久化场景等。
    void closing();

    /// @brief 当前页号（从 1 开始）。
    int currentPage();

    /// @brief 承载控制视图与调色板的容器控件。
    QWidget *controlContainer()
    {
        return mControlContainer;
    }

    /// @brief 控制视图（教师操作的视图）。
    BoardView *controlView()
    {
        return mControlView;
    }

    /// @brief 演示视图（投影/外接屏上的视图）。
    BoardView *displayView()
    {
        return mDisplayView;
    }

    /// @brief 当前活动场景（可写版本）。
    GraphicsScene *activeScene()
    {
        return mActiveScene;
    }

    /// @brief 设置深色背景下的画笔颜色，变化时发出 penColorChanged()。
    void setPenColorOnDarkBackground(const QColor &pColor)
    {
        if (mPenColorOnDarkBackground == pColor)
        {
            return;
        }

        mPenColorOnDarkBackground = pColor;
        emit penColorChanged();
    }

    /// @brief 设置浅色背景下的画笔颜色，变化时发出 penColorChanged()。
    void setPenColorOnLightBackground(const QColor &pColor)
    {
        if (mPenColorOnLightBackground == pColor)
        {
            return;
        }

        mPenColorOnLightBackground = pColor;
        emit penColorChanged();
    }

    /// @brief 设置深色背景下的记号笔颜色。
    void setMarkerColorOnDarkBackground(const QColor &pColor)
    {
        mMarkerColorOnDarkBackground = pColor;
    }

    /// @brief 设置浅色背景下的记号笔颜色。
    void setMarkerColorOnLightBackground(const QColor &pColor)
    {
        mMarkerColorOnLightBackground = pColor;
    }

    /// @brief 深色背景下的画笔颜色。
    QColor penColorOnDarkBackground()
    {
        return mPenColorOnDarkBackground;
    }

    /// @brief 浅色背景下的画笔颜色。
    QColor penColorOnLightBackground()
    {
        return mPenColorOnLightBackground;
    }

    /// @brief 深色背景下的记号笔颜色。
    QColor markerColorOnDarkBackground()
    {
        return mMarkerColorOnDarkBackground;
    }

    /// @brief 浅色背景下的记号笔颜色。
    QColor markerColorOnLightBackground()
    {
        return mMarkerColorOnLightBackground;
    }

    /// @brief 系统 DPI 缩放因子，联动界面尺寸换算。
    qreal systemScaleFactor()
    {
        return mSystemScaleFactor;
    }
    /// @brief 当前缩放比例（1.0 为原始大小）。
    qreal currentZoom();
    /// @brief 把当前视图位置记录到场景，供下次打开时恢复。
    void persistViewPositionOnCurrentScene();
    /**
     * @brief 持久化当前场景内容。
     * @param isAnAutomaticBackup 是否来自自动备份定时器。
     * @param forceImmediateSave 是否跳过延迟立即写盘。
     */
    void persistCurrentScene(bool isAnAutomaticBackup = false, bool forceImmediateSave = false);
    /// @brief 设置演示视图的投影区域（黑边/信箱模式）。
    void setBoxing(QRect displayRect);
    /// @brief 按当前语言刷新工具栏动作文本。
    void setToolbarTexts();
    /**
     * @brief 把压缩的网页小部件数据解包到临时目录。
     * @param pZipedData 压缩字节。
     * @param pExtension 目标扩展名，默认 wgt。
     * @return 解包后入口文件的 URL。
     */
    static QUrl expandWidgetToTempDir(const QByteArray &pZipedData, const QString &pExtension = QString("wgt"));

    /// @brief 调整当前页尺寸并维护相关动作状态。
    void setPageSize(QSize newSize);
    /// @brief 调色板管理器。
    BoardPaletteManager *paletteManager()
    {
        return mPaletteManager;
    }

    /// @brief 切换遮罩（幕布）可见性并通知相关控件。
    void notifyCache(bool visible);
    /// @brief 页切换后刷新页码、缩略图与动作状态。
    void notifyPageChanged();
    /// @brief 显示文档元数据（标题、作者等）到界面。
    void displayMetaData(QMap<QString, QString> metadatas);

    /**
     * @brief 收集撤销命令树中引用到的所有图元。
     * @param parent 命令树的根，递归遍历子命令。
     * @param itms 输出集合，调用方负责判空。
     */
    void findUniquesItems(const QUndoCommand *parent, QSet<QGraphicsItem *> &itms);
    /// @brief 清空撤销栈，通常在文档切换时调用。
    void ClearUndoStack();

    /**
     * @brief 切换到指定文档的场景。
     * @param pDocumentProxy 目标文档，可为空表示新建空文档。
     * @param pSceneIndex 目标页索引。
     * @param forceReload 是否强制重新加载场景内容。
     * @param onImport 是否处于导入流程（影响提示与保存策略）。
     */
    void setActiveDocumentScene(DocumentProxy *pDocumentProxy, int pSceneIndex = 0, bool forceReload = false, bool onImport = false);
    /// @brief 在当前文档内切换页。
    void setActiveDocumentScene(int pSceneIndex);

    /// @brief 移动页到新索引，并同步文档持久化。
    void moveSceneToIndex(int source, int target);
    /// @brief 复制指定页。
    void duplicateScene(int index);
    /// @brief 深拷贝一个图元并放入当前场景，返回新图元。
    GraphicsItem *duplicateItem(Item *item);
    /// @brief 删除指定页。
    void deleteScene(int index);

    /// @brief 遮罩（幕布）是否启用。
    bool cacheIsVisible()
    {
        return mCacheWidgetIsEnabled;
    }

    /// @brief “组合”动作的文本。
    QString actionGroupText()
    {
        return mActionGroupText;
    }
    /// @brief “取消组合”动作的文本。
    QString actionUngroupText()
    {
        return mActionUngroupText;
    }

public slots:
    /// @brief 打开文档选择对话框。
    void showDocumentsDialog();
    /// @brief 显示/隐藏虚拟键盘。
    void showKeyboard(bool show);
    /// @brief 进入黑屏（息屏）状态。
    void blackout();
    /// @brief 在当前文档末尾新建一页。
    void addScene();
    /**
     * @brief 在指定文档的指定位置新建页。
     * @param proxy 目标文档。
     * @param sceneIndex 插入位置。
     * @param replaceActiveIfEmpty 当前页为空时是否复用而不是新建。
     */
    void addScene(DocumentProxy *proxy, int sceneIndex, bool replaceActiveIfEmpty = false);
    /// @brief 把已有场景加入文档。
    void addScene(GraphicsScene *scene, bool replaceActiveIfEmpty = false);
    /// @brief 复制当前页。
    void duplicateScene();
    /// @brief 导入页（图片/PDF 等）。
    void importPage();
    /// @brief 清空当前场景的全部内容（可撤销）。
    void clearScene();
    /// @brief 清空当前场景的图元，保留背景。
    void clearSceneItems();
    /// @brief 清空当前场景的批注（笔画），保留对象。
    void clearSceneAnnotation();
    /// @brief 清空当前场景的背景。
    void clearSceneBackground();
    /// @brief 放大一档，可指定缩放中心（场景坐标）。
    void zoomIn(QPointF scenePoint = QPointF(0, 0));
    /// @brief 缩小一档，可指定缩放中心（场景坐标）。
    void zoomOut(QPointF scenePoint = QPointF(0, 0));
    /// @brief 恢复 100% 缩放。
    void zoomRestore();
    /// @brief 视图回到页面中心。
    void centerRestore();
    /// @brief 把指定场景点居中显示。
    void centerOn(QPointF scenePoint = QPointF(0, 0));
    /// @brief 以指定比例与中心缩放。
    void zoom(const qreal ratio, QPointF scenePoint);
    /// @brief 手型工具平移视图（像素增量）。
    void handScroll(qreal dx, qreal dy);
    /// @brief 切换到上一页。
    void previousScene();
    /// @brief 切换到下一页。
    void nextScene();
    /// @brief 切换到第一页。
    void firstScene();
    /// @brief 切换到最后一页。
    void lastScene();
    /**
     * @brief 下载 URL 内容并插入页面。
     * @param url 下载地址。
     * @param contentSourceUrl 内容来源页地址，用于相对链接与来源记录。
     * @param pPos 插入位置（场景坐标）。
     * @param pSize 期望尺寸；空尺寸表示按内容原始尺寸。
     * @param isBackground 是否作为页面背景插入。
     * @param internalData 数据是否来自内部（文档资源而非网络）。
     */
    void downloadURL(const QUrl &url, QString contentSourceUrl = QString(), const QPointF &pPos = QPointF(0.0, 0.0), const QSize &pSize = QSize(),
        bool isBackground = false, bool internalData = false);
    /**
     * @brief 下载完成后的回调：按内容类型创建图元并插入页面。
     * @return 新建的图元；失败或类型不支持时返回空指针。
     */
    Item *downloadFinished(bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pHeader, QByteArray pData, QPointF pPos, QSize pSize,
        bool isBackground = false, bool internalData = false);
    /// @brief 切换浅色/深色背景与背景样式。
    void changeBackground(bool isDark, PageBackground pageBackground);
    /// @brief 设置视图鼠标光标为指定工具的形状。
    void setToolCursor(int tool);
    /// @brief 显示状态栏消息，可选旋转进度指示。
    void showMessage(const QString &message, bool showSpinningWheel = false);
    /// @brief 隐藏状态栏消息。
    void hideMessage();
    /// @brief 禁用/启用画板交互（如模态流程期间）。
    void setDisabled(bool disable);
    /// @brief 设置当前调色板颜色索引。
    void setColorIndex(int pColorIndex);
    /// @brief 移除并销毁指定尺规工具控件。
    void removeTool(ToolWidget *toolWidget);
    /// @brief 隐藏画板窗口。
    void hide();
    /// @brief 显示画板窗口。
    void show();
    /// @brief 切换到宽页尺寸。
    void setWidePageSize(bool checked);
    /// @brief 切换到常规页尺寸。
    void setRegularPageSize(bool checked);
    /// @brief 工具切换后的视图光标与状态同步。
    void stylusToolChanged(int tool);
    /// @brief 抓取场景区域为图像（截图/导出）。
    void grabScene(const QRectF &pSceneRect);
    /**
     * @brief 插入视频图元。
     * @param pUrl 视频地址或本地路径。
     * @param startPlay 插入后是否立即播放。
     * @param pos 插入位置（场景坐标）。
     * @param bUseSource 是否按来源 URL 而非缓存数据处理。
     * @return 新建的视频图元。
     */
    GraphicsMediaItem *addVideo(const QUrl &pUrl, bool startPlay, const QPointF &pos, bool bUseSource = false);
    /// @brief 插入音频图元，参数含义同 addVideo()。
    GraphicsMediaItem *addAudio(const QUrl &pUrl, bool startPlay, const QPointF &pos, bool bUseSource = false);
    /// @brief 插入网页小部件。
    GraphicsWidgetItem *addWebWidget(const QUrl &pUrl, const QPointF &pos);
    /// @brief 根据外接屏数量与位置重新安排演示视图。
    void adjustDisplayViews();
    /// @brief 剪切选中内容到剪贴板。
    void cut();
    /// @brief 复制选中内容到剪贴板。
    void copy();
    /// @brief 从剪贴板粘贴。
    void paste();
    /// @brief 处理拖放/粘贴的 MIME 数据并插入到指定位置。
    void processMimeData(const QMimeData *pMimeData, const QPointF &pPos);
    /// @brief 把网页小部件从控制视图迁移到场景中。
    void moveGraphicsWidgetToControlView(GraphicsWidgetItem *graphicWidget);
    /// @brief 把尺规工具控件迁移到场景中。
    void moveToolWidgetToScene(ToolWidget *toolWidget);
    /// @brief 通过插入对话框添加新图元。
    void addItem();

    /// @brief 冻结/解冻所有网页小部件（切换页面或投影时降低开销）。
    void freezeWebWidgets(bool freeze);
    /// @brief 冻结/解冻单个图元内的网页小部件。
    void freezeWebWidget(QGraphicsItem *item, bool freeze);
    /// @brief 启动文档脚本。
    void startScript();
    /// @brief 停止文档脚本。
    void stopScript();

    /// @brief 保存当前文档。
    void saveData(SaveFlags fls = sf_none);

public slots:
    /// @brief 主模式（教学/桌面等）切换后的界面与视图调整。
    void appMainModeChanged(ApplicationController::MainMode);

signals:
    /// @brief 活动场景已切换。
    void activeSceneChanged();
    /// @brief 缩放比例变化，参数为新比例。
    void zoomChanged(qreal pZoomFactor);
    /// @brief 画笔颜色变化。
    void penColorChanged();
    /// @brief 控制视图视口尺寸变化。
    void controlViewportChanged();
    /// @brief 页面背景变化。
    void backgroundChanged();
    /// @brief 遮罩（幕布）被启用。
    void cacheEnabled();
    /// @brief 请求界面显示文档元数据。
    void displayMetadata(QMap<QString, QString> metadata);
    /// @brief 当前页选择变化，参数为页索引。
    void pageSelectionChanged(int index);

protected:
    /// @brief 创建并配置控制视图与演示视图。
    void setupViews();
    /// @brief 构建工具栏。
    void setupToolbar();
    /// @brief 连接工具栏动作。
    void connectToolbar();
    /// @brief 初始化工具栏文本。
    void initToolbarTexts();
    /// @brief 根据当前选中内容与场景状态刷新动作可用性。
    void updateActionStates();
    /// @brief 重新计算系统缩放因子。
    void updateSystemScaleFactor();
    /// @brief 按像素宽度截断文本并加省略号。
    QString truncate(QString text, int maxWidth);

public slots:
    /// @brief 选中集合变化时刷新工具栏与选中框。
    void selectionChanged();
    /// @brief 撤销/重做可用状态变化时刷新动作。
    void undoRedoStateChange(bool canUndo);
    /// @brief 文档场景发生变化（加载/切换）后的同步。
    void documentSceneChanged(DocumentProxy *proxy, int pIndex);

private slots:
    /// @brief 自动保存定时器到期：按设置决定是否落盘。
    void autosaveTimeout();

private:
    /// @brief 初始化背景网格尺寸。
    void initBackgroundGridSize();
    /// @brief 刷新页面尺寸相关动作的勾选状态。
    void updatePageSizeState();
    /// @brief 记录当前视图状态（缩放、位置）到场景。
    void saveViewState();
    /// @brief 从设置读取自动保存间隔（毫秒）。
    int autosaveTimeoutFromSettings();

    MainWindow                              *mMainWindow;                   ///< 主窗口，不持有所有权
    GraphicsScene                           *mActiveScene;                  ///< 当前活动场景，归文档/场景所有
    int                                      mActiveSceneIndex;             ///< 活动场景索引
    BoardPaletteManager                     *mPaletteManager;               ///< 调色板管理器
    MessageWindow                           *mMessageWindow;                ///< 状态消息窗口
    BoardView                               *mControlView;                  ///< 控制视图
    BoardView                               *mDisplayView;                  ///< 演示视图
    QWidget                                 *mControlContainer;             ///< 控制视图容器
    QHBoxLayout                             *mControlLayout;                ///< 控制视图布局
    qreal                                    mZoomFactor;                   ///< 当前缩放比例
    bool                                     mIsClosing;                    ///< 是否正在关闭
    QColor                                   mPenColorOnDarkBackground;     ///< 深色背景下的画笔颜色
    QColor                                   mPenColorOnLightBackground;    ///< 浅色背景下的画笔颜色
    QColor                                   mMarkerColorOnDarkBackground;  ///< 深色背景下的记号笔颜色
    QColor                                   mMarkerColorOnLightBackground; ///< 浅色背景下的记号笔颜色
    qreal                                    mSystemScaleFactor;            ///< 系统缩放因子
    bool                                     mCleanupDone;                  ///< 清理是否已完成，避免重复收尾
    QMap<QAction *, QPair<QString, QString>> mActionTexts;                  ///< 动作文本缓存（快捷键提示/纯文本）
    bool                                     mCacheWidgetIsEnabled;         ///< 遮罩是否启用
    QGraphicsItem                           *mLastCreatedItem;              ///< 最近创建的图元，供插入后选中
    int                                      mDeletingSceneIndex;           ///< 正在删除的页索引
    int                                      mMovingSceneIndex;             ///< 正在移动的页索引
    QString                                  mActionGroupText;              ///< “组合”动作文本
    QString                                  mActionUngroupText;            ///< “取消组合”动作文本

    QTimer *mAutosaveTimer;                                                 ///< 自动保存定时器

public slots:
    /// @brief 工具按钮双击：切换回上一次使用的工具。
    void stylusToolDoubleClicked(int tool);
    /// @brief 控制视图尺寸变化后的视图适配。
    void boardViewResized(QResizeEvent *event);
    /// @brief 文档即将删除时的清理与场景摘除。
    void documentWillBeDeleted(DocumentProxy *pProxy);
    /// @brief 背景变化后同步背景相关动作的勾选状态。
    void updateBackgroundActionsState(bool isDark, PageBackground pageBackground);
    /// @brief 调色板颜色变化后刷新画笔/记号笔颜色。
    void colorPaletteChanged();
    /// @brief 素材库对话框关闭后的收尾。
    void libraryDialogClosed(int ret);
    /// @brief 最后一个窗口关闭时的应用退出处理。
    void lastWindowClosed();
    /// @brief 下载模态框结束后的收尾。
    void onDownloadModalFinished();
};
