/**
 * @file Application.h
 * @brief 声明应用对象：进程入口、全局控制器持有者以及跨模块共享的 MIME 类型。
 */

#pragma once

#include <QtWidgets>
#include <QUndoStack>
#include <QToolBar>
#include <QMenu>

#include "third/singleapp/qtsingleapplication.h"

namespace Ui {
class MainWindow;
}

class BoardController;
class WebController;
class ControlView;
class PreferencesController;
class Resources;
class Settings;
class PersistenceManager;
class ApplicationController;
class DocumentController;
class MainWindow;

/**
 * @brief 应用对象：程序的单一入口，持有主窗口、撤销栈与各模式控制器的全局指针。
 *
 * 详细说明：处于架构最外层（装配层），在 exec() 中按“主窗口 → 各控制器 → 信号连线”的顺序
 * 完成组装，并负责退出时的销毁顺序；其它模块通过本类的静态成员与静态函数访问共享状态。
 * 生命周期覆盖整个进程，必须且只能有一个实例（由 main() 在栈上创建）。
 * 线程约束：所有成员都只能在 GUI 线程使用；exec() 返回后对象才可析构。
 */
class Application : public QtSingleApplication {
    Q_OBJECT

public:
    /**
     * @brief 构造应用对象，完成设置层、翻译器与样式的初始化。
     * @param id 单实例通道标识，用于把命令行参数转交给已运行的实例。
     * @param argc 命令行参数个数，由基类接管并可能被裁剪。
     * @param argv 命令行参数数组。
     * @note 此时尚未创建主窗口；exec() 之前不应假设界面存在。
     */
    Application(const QString &id, int &argc, char **argv);
    /// 按固定顺序销毁主窗口、各单例与平台资源。
    virtual ~Application();

    /**
     * @brief 创建主窗口与各控制器，随后进入 Qt 事件循环。
     * @param pFileToImport 启动时要导入的文件路径；为空则按设置进入画板或桌面模式。
     * @return QApplication::exec() 的退出码。
     * @warning 只应调用一次；重复调用会重复创建控制器并泄漏连接。
     */
    int exec(const QString &pFileToImport);

    /// 删除各控制器并把全局指针置空，供退出流程使用；不结束事件循环。
    void cleanup();

    /// 全局撤销栈，父对象为 staticMemoryCleaner。
    static QPointer<QUndoStack> undoStack;

    /// 应用控制器，由 exec() 创建，父对象为 staticMemoryCleaner。
    static ApplicationController *applicationController;
    /// 画板控制器，父对象为主窗口。
    static BoardController *boardController;
    /// 网页控制器，父对象为主窗口。
    static WebController *webController;
    /// 文档库控制器，父对象为主窗口。
    static DocumentController *documentController;

    /// 主窗口，由 exec() 创建并在析构中删除。
    static MainWindow *mainWindow;

    /**
     * @brief 返回可充当对话框父窗口的控件，默认即主窗口。
     * @return 主窗口指针，可能为空（exec() 之前）。
     * @note 只把主窗口当"对话框挂在谁身上"用的下层模块（content 的导出对话框）取这个，
     * 不必 include ui/MainWindow.h 去拿 MainWindow 的完整类型——见 docs/architecture.md
     * 拆环顺序第 6 步。定义在 Application.cpp，那里认识界面。
     */
    static QWidget *dialogHost();

    /// 以具体类型返回 qApp；调用方需保证当前存在 Application 实例。
    static Application *app() { return static_cast<Application *> qApp; }

    /// 文档库条目的拖放 MIME 类型。
    static const QString mimeTypeBoardDocument;
    /// 单个页面的拖放 MIME 类型。
    static const QString mimeTypeBoardPage;
    /// 页面内图形条目的拖放 MIME 类型。
    static const QString mimeTypeBoardPageItem;
    /// 页面缩略图的拖放 MIME 类型。
    static const QString mimeTypeBoardPageThumbnail;

    /**
     * @brief 在主窗口上显示一条提示消息。
     * @param message 要显示的文字。
     * @param showSpinningWheel 为 true 时同时显示忙碌指示。
     * @note 应用控制器尚未创建时静默忽略。
     */
    static void showMessage(const QString &message, bool showSpinningWheel = false);
    /// 启用或禁用画板交互；委托给 BoardController。
    static void setDisabled(bool disable);

    /// 承载应用级静态对象（撤销栈等）的清理宿主，随 Application 析构删除。
    static QObject *staticMemoryCleaner;

    /**
     * @brief 给动作对应的工具栏按钮挂上下拉菜单。
     * @param action 需要装饰的动作；其关联控件中只有尚未带菜单的 QToolButton 会被处理。
     */
    void decorateActionMenu(QAction *action);
    /**
     * @brief 在工具栏的指定动作前插入一个空白占位控件。
     * @param toolbar 目标工具栏。
     * @param action 作为插入锚点的动作。
     * @param width 固定宽度（像素）；负值表示使用可伸缩的空白。
     */
    void insertSpaceToToolbarBeforeAction(QToolBar *toolbar, QAction *action, int width = -1);

    /// 返回画板工具栏当前高度，供布局计算使用。
    int toolBarHeight();
    /// 处理文件打开、手写笔离开、应用激活等应用级事件；始终转发给基类过滤器。
    bool eventFilter(QObject *obj, QEvent *event);

    /// 是否处于详细日志模式（由命令行参数决定）。
    bool isVerbose() { return mIsVerbose; }
    /// 设置详细日志模式开关。
    void setVerbose(bool verbose) { mIsVerbose = verbose; }
    /**
     * @brief 从 HTML 片段中提取根元素的 src 地址。
     * @param html 可能包含注释与空字符的 HTML 文本。
     * @return 提取到的地址；结构不符合预期时为空串。
     */
    static QString urlFromHtml(QString html);
    /// 判断 URL 是否指向网络资源（本地路径与内部协议返回 false）。
    static bool isFromWeb(QString url);

    /// 返回画板控制视图所在屏幕；越界时回退到主屏。
    static QScreen *controlScreen();
    /// 返回控制屏在屏幕列表中的下标。
    static int controlScreenIndex();

public slots:
    /// 切换到画板模式。
    void showBoard();
    /// 切换到网页模式，并要求网页视图把标签栏放到顶部。
    void showInternet();
    /// 切换到文档库模式。
    void showDocument();
    /// 解除网页控件的冻结状态。
    void startScript();
    /// 冻结网页控件，避免网页内容遮挡批注。
    void stopScript();

    /**
     * @brief 响应“工具栏在顶部/底部”设置变化，重新安放三个工具栏。
     * @param topOrBottom 非零表示顶部。
     */
    void toolBarPositionChanged(QVariant topOrBottom);
    /**
     * @brief 响应“工具栏显示文字”设置变化。
     * @param display 非零表示图标下带文字。
     */
    void toolBarDisplayTextChanged(QVariant display);

    /// 处理主窗口关闭事件，转入统一的退出流程。
    void closeEvent(QCloseEvent *event);

    /**
     * @brief 处理单实例通道收到的消息：ping 直接应答，其余按待导入文件路径处理。
     * @param pMessage 收到的消息文本。
     * @return 恒为 true，表示消息已消费。
     */
    bool handleOpenMessage(const QString &pMessage);

private slots:
    /// 退出前的收尾：清理回收站、通知各控制器关闭并保存设置。
    void closing();
    /// 屏幕数量变化时更新多屏动作的可用状态。
    void onScreenCountChanged(int newCount);

private:
    /// 依据设置启用此前被隐藏的多屏动作与菜单。
    void updateProtoActionsState();
    /**
     * @brief 解析命令行与设置，安装界面翻译器。
     * @param args 命令行参数，识别 `-lang`。
     */
    void setupTranslators(const QStringList &args);
    QList<QMenu *> mProtoMenus;
    bool mIsVerbose;

protected:
    /// 偏好设置控制器，父对象为主窗口。
    PreferencesController *mPreferencesController;
};
