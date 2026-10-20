/**
 * @file ApplicationController.h
 * @brief 声明应用模式总控器：在画板、网页、文档库、桌面批注之间切换，并协调多屏布局。
 */

#pragma once

#include <QtWidgets>
#include <QtNetwork>
#include <QNetworkAccessManager>

#include <QtNetwork/QHttpPart>
#include <new>
#include <memory>
#include <QJSEngine>

class BoardView;
class DocumentProxy;
class GraphicsScene;
class DesktopAnnotationController;
class ScreenMirror;
class MainWindow;
class DisplayManager;
class Version;
class QNetworkAccessManager;
class QNetworkReply;
class RightPalette;

/**
 * @brief 应用模式总控器：决定当前显示哪种模式，并协调相关视图、调色板与多屏布局。
 *
 * 详细说明：位于装配层与 whiteboard/web/document 各控制器之间，Application::exec() 创建
 * 并把它挂在 staticMemoryCleaner 下；视图与主窗口都由外部传入，本类只使用、不拥有。
 * 线程约束：QObject 亲和于 GUI 线程，所有槽与信号都只能在 GUI 线程收发。
 */
class ApplicationController : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造总控器并建立与各视图、多屏管理器及录屏/镜像对象的连接。
     * @param pControlView 控制屏上的画板视图，由 BoardController 拥有。
     * @param pDisplayView 展示屏上的画板视图，可为空。
     * @param pMainWindow 主窗口，用于状态栏提示与模式切换。
     * @param parent 父对象，用于生命周期管理。
     * @param rightPalette 画板右侧调色板，用于切换模式时的显隐。
     */
    ApplicationController(BoardView *pControlView, BoardView *pDisplayView, MainWindow *pMainWindow,
                          QObject *parent, RightPalette *rightPalette);
    virtual ~ApplicationController();

    /// 画板视图的初始水平滚动位置。
    int initialHScroll() { return mInitialHScroll; }
    /// 画板视图的初始垂直滚动位置。
    int initialVScroll() { return mInitialVScroll; }

    /// 按当前模式与工具栏设置调整工具栏内容。
    void adaptToolBar();
    /// 让展示屏视图与控制屏视图的缩放和位置保持一致。
    void adjustDisplayView();
    /**
     * @brief 页面切换后同步其它视图（展示屏与历史视图）的滚动位置。
     * @param pActiveSceneIndex 新激活的页面下标。
     * @param pActiveDocument 新激活页面所属文档。
     */
    void adjustPreviousViews(int pActiveSceneIndex, DocumentProxy *pActiveDocument);

    /// 临时让屏幕变黑（黑屏模式），可再次触发恢复。
    void blackout();

    /**
     * @brief 初始化多屏布局。
     * @param useMultiscreen 是否启用多屏（控制屏 + 展示屏）。
     * @note 只应在主窗口显示前调用一次。
     */
    void initScreenLayout(bool useMultiscreen);

    /// 退出前的收尾：停止录屏、断开镜像并恢复显示状态。
    void closing();

    /**
     * @brief 指定镜像源的控件。
     * @note 传入空指针表示取消镜像。
     */
    void setMirrorSourceWidget(QWidget *);

    /**
     * @brief 开启或关闭屏幕镜像。
     */
    void mirroringEnabled(bool);

    /**
     * @brief 恢复画板视图的初始滚动状态。
     * @param horizontalPosition 水平位置。
     * @param verticalPosition 垂直位置。
     */
    void initViewState(int horizontalPosition, int verticalPosition);

    /// 切换到画板模式。
    void showBoard();

    /// 切换到网页模式。
    void showInternet();

    /**
     * @brief 切到浏览器模式并打开这个地址。
     * @param url 目标地址。
     * @note 画板里那三条"去来源页 / 购买链接 / 点开的链接"都只要这一句，
     * 不必认识 web/WebController.h——装配层本来就认识所有控制器。
     */
    void openInternetUrl(const QUrl &url);

    /// 切换到文档库模式。
    void showDocument();

    /**
     * @brief 在主窗口显示提示。
     * @param message 提示文字。
     * @param showSpinningWheel 是否同时显示忙碌指示。
     */
    void showMessage(const QString &message, bool showSpinningWheel);

    /**
     * @brief 导入一个外部文件（文档包或可识别的内容文件）。
     * @param pFilePath 文件系统路径。
     */
    void importFile(const QString &pFilePath);

    /// 返回多屏管理器，调用方不得取得所有权。
    DisplayManager *displayManager() { return mDisplayManager; }

    /// 返回桌面批注控制器，调用方不得取得所有权。
    DesktopAnnotationController *uninotesController() { return mUninoteController; }

    /**
     * @brief 返回桌面批注模式下可承载画板调色板的宿主窗口。
     * @return 宿主窗口指针；桌面批注模式未激活时可能为空。
     * @note 桌面批注模式下承载画板调色板的宿主窗口，以及它的尺寸变化通知。画板只需要
     * "一个能当父控件的窗口"，不需要认识 DesktopAnnotationController 本身——装配层
     * 提供这两个入口，whiteboard 就不必为这事 include desktop/。
     */
    QWidget *desktopDrawingHost();
    /// 通知总控器桌面批注宿主尺寸已变化，触发重新布局。
    void desktopDrawingResized();

    /// 应用的主显示模式。
    enum MainMode {
        Board = 0,  ///< 画板模式
        Internet,   ///< 内嵌浏览器模式
        Document,   ///< 文档库模式
        WebDocument ///< 以网页形式呈现文档的模式
    };

    /// 返回当前主显示模式。
    MainMode displayMode() { return mMainMode; }


    /// 桌面批注是否正显示在前台。
    bool isShowingDesktop() { return mIsShowingDesktop; }

signals:
    /// 主显示模式发生变化。
    void mainModeChanged(ApplicationController::MainMode pMode);
    /// 桌面批注层显示或隐藏。
    void desktopMode(bool displayed);

public slots:
    /**
     * @brief 把抓取到的屏幕或页面图像加入当前文档。
     * @param pPixmap 图像内容。
     * @param pageMode true 表示作为新页面插入，false 表示放入当前页面。
     * @param sourceUrl 图像来源地址，用于记录出处。
     */
    void addCapturedPixmap(const QPixmap &pPixmap, bool pageMode, const QUrl &sourceUrl = QUrl());

    /**
     * @brief 把网页嵌入代码添加到当前文档。
     * @param embedCode 网页提供的嵌入代码片段。
     */
    void addCapturedEmbedCode(const QString &embedCode);

    /// 屏幕布局变化后的统一刷新入口。
    void screenLayoutChanged();

    /**
     * @brief 显示桌面批注层。
     * @param dontSwitchFrontProcess 为 true 时不抢占前台进程焦点。
     */
    void showDesktop(bool dontSwitchFrontProcess = false);

    /// 隐藏桌面批注层并回到之前的模式。
    void hideDesktop();

    /**
     * @brief 显示或隐藏录屏调色板。
     * @param visible 目标可见性。
     */
    void toggleRecordingPalette(bool visible);

    /**
     * @brief 启用或停用多屏模式。
     * @param use 是否启用。
     */
    void useMultiScreen(bool use);

    /// 对当前焦点执行剪切。
    void actionCut();
    /// 对当前焦点执行复制。
    void actionCopy();
    /// 对当前焦点执行粘贴。
    void actionPaste();

private slots:

protected:
    /// 桌面批注控制器，由本类持有。
    DesktopAnnotationController *mUninoteController;

    /// 主窗口，不持有所有权。
    MainWindow *mMainWindow;

    /// 控制屏画板视图，不持有所有权。
    BoardView *mControlView;
    /// 展示屏画板视图，不持有所有权。
    BoardView *mDisplayView;
    /// 此前使用过的画板视图，用于恢复滚动状态。
    QList<BoardView *> mPreviousViews;

    /// 黑屏模式下覆盖屏幕的场景。
    GraphicsScene *mBlackScene;

    /// 屏幕镜像对象，由本类持有。
    ScreenMirror *mMirror;

    int mInitialHScroll, mInitialVScroll;

private:
    MainMode mMainMode;

    /// 多屏管理器，由本类持有。
    DisplayManager *mDisplayManager;


    bool mIsShowingDesktop;
};
