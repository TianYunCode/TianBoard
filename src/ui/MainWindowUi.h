/**
 * @file MainWindowUi.h
 * @brief 主窗口界面的数据化描述：集中保存全部 QAction、工具栏和中央部件指针。
 */

#pragma once


#include <QtGui/QAction>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

class QMainWindow;

/**
 * @brief 从 Designer 表单移植过来的主窗口界面数据。
 *
 * 只负责创建并持有所有 QAction 以及三条顶部工具栏，不参与业务逻辑；
 * MainWindow 构造时调用 setupUi 把它填满，此后控制器通过
 * Application::mainWindow->ui() 取用其中的动作与工具栏。
 */
struct MainWindowUi {
    QAction *actionStylus;                  ///< 显示/隐藏触控笔面板（可勾选，默认选中）。
    QAction *actionBackgrounds;             ///< 打开背景选择面板。
    QAction *actionUndo;                    ///< 撤销。
    QAction *actionRedo;                    ///< 重做。
    QAction *actionBack;                    ///< 上一页。
    QAction *actionForward;                 ///< 下一页。
    QAction *actionDocument;                ///< 切换到文档管理页。
    QAction *actionWeb;                     ///< 切换到网页页。
    QAction *actionLineSmall;               ///< 细线条宽度。
    QAction *actionLineMedium;              ///< 中等线条宽度。
    QAction *actionLineLarge;               ///< 粗线条宽度。
    QAction *actionQuit;                    ///< 退出应用。
    QAction *actionEraserSmall;             ///< 小号橡皮。
    QAction *actionEraserMedium;            ///< 中号橡皮（默认选中）。
    QAction *actionEraserLarge;             ///< 大号橡皮。
    QAction *actionColor0;                  ///< 第 1 个笔色按钮。
    QAction *actionColor1;                  ///< 第 2 个笔色按钮。
    QAction *actionColor2;                  ///< 第 3 个笔色按钮。
    QAction *actionColor3;                  ///< 第 4 个笔色按钮。
    QAction *actionColor4;                  ///< 第 5 个笔色按钮。
    QAction *actionWebBack;                 ///< 网页后退。
    QAction *actionWebForward;              ///< 网页前进。
    QAction *actionWebReload;               ///< 重新加载网页。
    QAction *actionHome;                    ///< 打开网页主页。
    QAction *actionBoard;                   ///< 切换到白板页。
    QAction *actionErase;                   ///< 进入擦除模式。
    QAction *actionPreferences;             ///< 打开偏好设置。
    QAction *actionMenu;                    ///< 应用主菜单入口。
    QAction *actionDesktop;                 ///< 显示计算机桌面。
    QAction *actionWebBigger;               ///< 放大网页。
    QAction *actionWebSmaller;              ///< 缩小网页。
    QAction *actionNewFolder;               ///< 新建文件夹。
    QAction *actionNewDocument;             ///< 新建文档。
    QAction *actionImport;                  ///< 导入文档。
    QAction *actionExport;                  ///< 导出文档。
    QAction *actionOpen;                    ///< 在白板中打开所选页面。
    QAction *actionDuplicate;               ///< 复制所选内容。
    QAction *actionDelete;                  ///< 删除所选内容。
    QAction *actionAddToWorkingDocument;    ///< 加入当前打开的文档。
    QAction *actionDocumentAdd;             ///< 把内容添加到文档。
    QAction *actionRename;                  ///< 重命名。
    QAction *actionDocumentTools;           ///< 显示/隐藏文档工具面板。
    QAction *actionWebTools;                ///< 显示/隐藏网页工具面板（可勾选）。
    QAction *actionMultiScreen;             ///< 多屏显示开关（默认选中）。
    QAction *actionWidePageSize;            ///< 使用 16:9 宽页面。
    QAction *actionRegularPageSize;         ///< 使用 4:3 常规页面。
    QAction *actionCustomPageSize;          ///< 自定义页面尺寸（默认禁用）。
    QAction *actionStopLoading;             ///< 停止加载网页。
    QAction *actionCut;                     ///< 剪切。
    QAction *actionCopy;                    ///< 复制。
    QAction *actionPaste;                   ///< 粘贴。
    QAction *actionSleep;                   ///< 让展示画面进入黑屏。
    QAction *actionVirtualKeyboard;         ///< 显示虚拟键盘（可勾选）。
    QAction *actionPlainLightBackground;    ///< 纯色浅色背景。
    QAction *actionCrossedLightBackground;  ///< 网格浅色背景。
    QAction *actionRuledLightBackground;    ///< 横线浅色背景。
    QAction *actionPlainDarkBackground;     ///< 纯色深色背景。
    QAction *actionCrossedDarkBackground;   ///< 网格深色背景。
    QAction *actionRuledDarkBackground;     ///< 横线深色背景。
    QAction *actionRecording;               ///< 录屏菜单入口。
    QAction *actionRecordingStart;          ///< 开始/停止屏幕录制。
    QAction *actionEraseItems;              ///< 擦除页面中的所有对象。
    QAction *actionEraseAnnotations;        ///< 擦除页面中的所有批注。
    QAction *actionClearPage;               ///< 清空当前页。
    QAction *actionAdd;                     ///< 通用“添加”动作。
    QAction *actionPen;                     ///< 钢笔工具（默认选中）。
    QAction *actionEraser;                  ///< 橡皮工具。
    QAction *actionMarker;                  ///< 荧光笔工具。
    QAction *actionSelector;                ///< 选择/编辑对象工具。
    QAction *actionHand;                    ///< 抓手（平移页面）工具。
    QAction *actionZoomIn;                  ///< 放大工具。
    QAction *actionZoomOut;                 ///< 缩小工具。
    QAction *actionPointer;                 ///< 激光笔工具。
    QAction *actionLine;                    ///< 画线工具。
    QAction *actionText;                    ///< 文本工具。
    QAction *actionCapture;                 ///< 截取屏幕区域。
    QAction *actionAddItemToCurrentPage;    ///< 把截取内容加入当前页。
    QAction *actionAddItemToNewPage;        ///< 把截取内容加入新页。
    QAction *actionAddItemToLibrary;        ///< 把截取内容加入素材库。
    QAction *actionPages;                   ///< 打开页面导航面板。
    QAction *actionNewPage;                 ///< 新建页面。
    QAction *actionDuplicatePage;           ///< 复制当前页。
    QAction *actionImportPage;              ///< 导入外部页面。
    QAction *actionRecordingPause;          ///< 暂停屏幕录制（默认禁用）。
    QAction *actionRecordingConfig;         ///< 配置屏幕录制。
    QAction *actionWebTrap;                 ///< 网页捕获开关。
    QAction *actionWebCustomCapture;        ///< 网页区域截图。
    QAction *actionWebShowHideOnDisplay;    ///< 在展示屏上显示/隐藏主屏内容。
    QAction *actionEraseDesktopAnnotations; ///< 擦除桌面批注。
    QAction *actionHideApplication;         ///< 隐藏应用窗口。
    QAction *actionPlay;                    ///< 与页面内对象交互（播放）模式。
    QAction *actionEraseBackground;         ///< 清除页面背景。
    QAction *actionDefaultGridSize;         ///< 恢复默认网格尺寸。
    QWidget *centralwidget;                 ///< 表单声明的中央部件；MainWindow 会换成带页面栈的部件。
    QToolBar *boardToolBar;                 ///< 白板工具栏，位于顶部第一行。
    QToolBar *webToolBar;                   ///< 网页工具栏，通过 toolbar break 独占一行。
    QToolBar *documentToolBar;              ///< 文档工具栏，同样独占一行。

    /**
     * @brief 创建全部动作与三条工具栏并挂到主窗口上。
     * @param mw 目标主窗口，动作一律以它为父对象。
     * @note 只创建界面元素，不建立业务连接；末尾会调用 retranslateUi。
     */
    void setupUi(QMainWindow *mw);
    /**
     * @brief 为动作设置标题、提示与快捷键。
     * @param mw 主窗口，用于设置窗口标题。
     * @note 切换语言后会被再次调用，以便重新取词。
     */
    void retranslateUi(QMainWindow *mw);
};
