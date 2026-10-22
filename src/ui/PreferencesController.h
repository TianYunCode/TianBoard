/**
 * @file PreferencesController.h
 * @brief 声明偏好设置对话框、其控制器以及画笔属性面板。
 */

#pragma once

#include <QtWidgets>
#include <QDialog>

#include <memory>

class ColorPicker;
class Application;
class VirtualDesktop;
class Settings;
class PreferencesController;
class BrushPropertiesFrame;

/**
 * @brief 偏好设置对话框：承载设置界面，并在运行期语言切换时重新翻译自身。
 *
 * 详细说明：由 PreferencesController 创建并拥有；界面对象是其内嵌 Ui 结构，
 * 通过 pimpl 持有，对外只暴露 ui()。控制器指针由外部传入，不持有所有权。
 * 线程约束：GUI 线程专用。
 */
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    /// 手写的界面结构，取代原先由 .ui 生成的 ui_preferences.h；定义在 .cpp 里。
    struct Ui;

    /**
     * @brief 构造对话框并搭建界面。
     * @param prefController 所属控制器，不转移所有权。
     * @param parent 父窗口。
     * @param f 窗口标志。
     */
    PreferencesDialog(PreferencesController *prefController, QWidget *parent = 0, Qt::WindowFlags f = Qt::WindowFlags());
    /// 析构；定义在 .cpp 中，以便销毁此处仍不完整的 Private。
    ~PreferencesDialog();

    /// 返回界面控件集合。
    Ui &ui();

protected:
    /// 关闭即接受，不额外询问。
    void closeEvent(QCloseEvent *e);
    /// 所属控制器，由外部创建并持有，本类只借用。
    PreferencesController *mPreferencesController;

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 偏好设置控制器：创建对话框、把控件与设置项双向连接，并管理画笔属性面板。
 *
 * 详细说明：由 Application 创建并以主窗口为父对象；拥有对话框与两个
 * BrushPropertiesFrame。设置项的写入通过 Settings 完成，本类只负责界面同步。
 * 线程约束：GUI 线程专用。
 */
class PreferencesController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造控制器、创建设置对话框并建立信号连接。
     * @param parent 父窗口，同时作为对话框的父窗口。
     */
    PreferencesController(QWidget *parent);
    /// 析构；连带删除本类持有的对话框与两个画笔属性面板。
    virtual ~PreferencesController();

public slots:
    /// 从设置刷新界面后以模态方式显示对话框。
    void show();

signals:
    /**
     * @brief Cookie 相关的设置变了（策略、是否清除、白名单）。
     * @note 界面不去问浏览器那一层：谁来重装过滤器、数字从哪儿来，由装配层决定
     *       （见 core/Application 里的连接）。装完策略后装配层会回推 setCookiesMarkedForDeletion()。
     */
    void cookieSettingsChanged();

public slots:
    /// @brief 装配层回填“退出时会清掉多少个 Cookie”，只改提示行文字。
    void setCookiesMarkedForDeletion(int count);

protected:
    /// 把界面控件与设置项、运行时对象连接起来。
    void wire();
    /// 把当前设置值填入界面控件。
    void init();

    /// 设置对话框，由本类持有。
    PreferencesDialog *mPreferencesWindow;
    /// 对话框界面指针，属于对话框，本类只借用。
    PreferencesDialog::Ui *mPreferencesUI;
    /// 画笔属性面板，由本类持有。
    BrushPropertiesFrame *mPenProperties;
    /// 荧光笔属性面板，由本类持有。
    BrushPropertiesFrame *mMarkerProperties;

public slots:
    // BrushPropertiesFrame 在自己的构造函数里连它，必须对外可见
    /// 接收画笔颜色选择并写入对应设置。
    void colorSelected(const QColor &);

protected slots:
    /// 关闭对话框。
    void close();
    /// 把所有设置恢复为默认值。
    void defaultSettings();
    /// 画笔粗细变化时刷新预览。
    void penPreviewFromSizeChanged(int value);
    /// 深色背景网格不透明度变化。
    void darkBackgroundCrossOpacityValueChanged(int value);
    /// 浅色背景网格不透明度变化。
    void lightBackgroundCrossOpacityValueChanged(int value);
    /// 宽度滑块变化。
    void widthSliderChanged(int value);
    /// 不透明度滑块变化。
    void opacitySliderChanged(int value);
    /// 设置深色背景下的网格颜色。
    void setCrossColorOnDarkBackground(const QColor &color);
    /// 设置浅色背景下的网格颜色。
    void setCrossColorOnLightBackground(const QColor &color);
    /// 工具栏位置单选变化。
    void toolbarPositionChanged(bool checked);
    /// 工具栏改为竖排。
    void toolbarOrientationVertical(bool checked);
    /// 工具栏改为横排。
    void toolbarOrientationHorizontal(bool checked);
    /// 切换系统屏幕键盘。
    void systemOSKCheckBoxToggled(bool checked);

    /** @brief 刷新“退出时会清掉多少个 Cookie”的提示，随白名单与开关的变化重算。 */
    void cookieCountChanged();

private slots:
    /// 屏幕数量变化时启用或禁用多屏相关设置。
    void adjustScreens(int screen);

private:
    static qreal sSliderRatio;
    static qreal sMinPenWidth;
    static qreal sMaxPenWidth;
    /// 屏幕信息服务，由 VirtualDesktop 单例提供，不持有所有权。
    VirtualDesktop *mDesktop;
};

/**
 * @brief 画笔属性面板：在给定 QFrame 内搭建颜色选择器与粗细控件，并把选择转发给控制器。
 *
 * 详细说明：非 QObject，控件都挂在 owner 下；本类只持有界面结构与选择器列表，
 * 控制器指针仅用于连接信号。由 PreferencesController 创建并拥有。
 */
class BrushPropertiesFrame
{
public:
    /// 手写的界面结构，取代原先由 .ui 生成的 ui_brushProperties.h；定义在 .cpp 里。
    struct Ui;

    /**
     * @brief 在 owner 内搭建面板。
     * @param owner 承载控件的容器。
     * @param lightBackgroundColors 浅色背景下的可选颜色。
     * @param darkBackgroundColors 深色背景下的可选颜色。
     * @param lightBackgroundSelectedColors 浅色背景下的初始选中颜色。
     * @param darkBackgroundSelectedColors 深色背景下的初始选中颜色。
     * @param controller 接收颜色选择信号的控制器，不转移所有权。
     */
    BrushPropertiesFrame(QFrame *owner, const QList<QColor> &lightBackgroundColors, const QList<QColor> &darkBackgroundColors,
        const QList<QColor> &lightBackgroundSelectedColors, const QList<QColor> &darkBackgroundSelectedColors, PreferencesController *controller);

    /// 析构；定义在 .cpp 中，以便销毁此处仍不完整的 Private。
    virtual ~BrushPropertiesFrame();

    /// 返回界面控件集合。
    Ui &ui();

    /**
     * @brief 按当前语言重刷本面板的文案。
     * @note 运行期切语言时 PreferencesDialog 只重翻译它自己那份 Ui；这两个笔/荧光笔面板
     *       的文案住在各自的 Ui 里，必须由面板自己再走一遍 retranslateUi()，
     *       否则切完语言面板上仍是旧语言（以前正是如此）。
     */
    void retranslateUi();

    /// 返回浅色背景下的颜色选择器列表。
    QList<ColorPicker *> &lightBackgroundColorPickers();
    /// 返回深色背景下的颜色选择器列表。
    QList<ColorPicker *> &darkBackgroundColorPickers();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
