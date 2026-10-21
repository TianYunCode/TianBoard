/**
 * @file MainWindow.h
 * @brief 应用主窗口的声明：装配工具栏、页面栈以及若干消息框辅助函数。
 */

#pragma once

#include <QMainWindow>
#include <QMessageBox>

#include <memory>

#include "ui/MainWindowUi.h"

class QCloseEvent;
class QKeyEvent;
class QMenu;
class QStackedLayout;
class QWidget;

/**
 * @brief 装配完成后的应用主窗口。
 *
 * 界面由三块顶部工具栏（boardToolBar / webToolBar / documentToolBar，来自
 * MainWindowUi）和一个中央 QStackedLayout 组成：页面栈里依次可以放入
 * 白板页、网页页与文档管理页，切换时只改变当前页而不销毁其它页。
 * 由 Application 在启动时创建（无边框窗口），是整个应用唯一的主窗口，
 * 各调色板、对话框以及 board/web 控制器都通过 Application::mainWindow
 * 取到它。关闭事件不会真正关闭，而是转发给控制器决定是否隐藏或退出。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    /// 构造主窗口并完成界面装配；此时还没有任何页面被加入页面栈。
    MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~MainWindow() override;

    /// 返回由 setupUi 生成的界面对象，供控制器取用其中的 QAction。
    MainWindowUi &ui();

    /// 把白板页放入页面栈；只接受第一次调用，后续调用被忽略。
    void addBoardWidget(QWidget *pWidget);
    /// 把页面栈切到白板页；尚未加入时无动作。
    void switchToBoardWidget();

    /// 把网页页放入页面栈，并在其销毁时清空记录。
    void addWebWidget(QWidget *pWidget);
    /// 把页面栈切到网页页。
    void switchToWebWidget();

    /// 把文档管理页放入页面栈；只接受第一次调用。
    void addDocumentsWidget(QWidget *pWidget);
    /// 把页面栈切到文档管理页。
    void switchToDocumentsWidget();

    /// 弹出“是/否”询问框，返回用户是否点了“是”。
    bool yesNoQuestion(QString windowTitle, QString text);
    /// 弹出一个只有“确定”按钮的警告框。
    void warning(QString windowTitle, QString text);
    /// 弹出一个只有“确定”按钮的信息框。
    void information(QString windowTitle, QString text);

    /// 显示下载进度窗口（应用级模态）。
    void showDownloadWidget();
    /// 隐藏下载进度窗口。
    void hideDownloadWidget();

signals:
    /// 收到关闭请求时发出，由控制器决定后续动作；窗口自身保持打开。
    void closeEvent_Signal(QCloseEvent *event);

public slots:
    /// 导出结束后重新启用因文件对话框而被禁用的工具栏动作。
    void onExportDone();

protected:
    /// 生成只有一个“确定”按钮、可指定图标类型的消息框并模态显示。
    void oneButtonMessageBox(QString windowTitle, QString text, QMessageBox::Icon type);

    /// 键盘事件交给基类处理（快捷键由 QAction 负责）。
    void keyPressEvent(QKeyEvent *event) override;
    /// 忽略关闭事件并转发信号，避免窗口被直接销毁。
    void closeEvent(QCloseEvent *event) override;

    /// 明确不提供标题栏右键菜单。
    QMenu *createPopupMenu() override
    {
        return nullptr;
    }

private:
    /// 把 setupUi 造出来的动作按分组登记进 ShortcutManager：这是用户能改绑快捷键的前提。
    void registerShortcuts();

    struct Private;
    std::unique_ptr<Private> d;
};
