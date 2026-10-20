/**
 * @file ToolbarButtonGroup.h
 * @brief 工具栏按钮组的声明：把一组互斥动作显示成左右相连的按钮。
 */

#pragma once


#include <QtWidgets>
#include <QWidget>
#include <QToolBar>
#include <QToolButton>
#include <QActionGroup>

/**
 * @brief 工具栏上的分段式互斥按钮组。
 *
 * 界面上是几个边框相连、只允许一个选中的按钮（例如 5 个笔色、3 种线宽），
 * 用来在当前工具栏中替换单个工具按钮。由 DesktopPropertyPalette 等属性面板
 * 创建并插入 boardToolBar，所有权随工具栏；也可单独设置某个按钮的图标/颜色。
 */
class ToolbarButtonGroup : public QWidget {
    Q_OBJECT

public:
    /// 从工具栏首项取基准按钮并生成一组按钮。
    ToolbarButtonGroup(QToolBar *toolbar, const QList<QAction *> &actions = QList<QAction *>());
    virtual ~ToolbarButtonGroup();

    /// 设置第 index 个动作对应按钮的图标。
    void setIcon(const QIcon &icon, int index);
    /// 用纯色方块作为第 index 个按钮的图标。
    void setColor(const QColor &color, int index);
    /// 返回当前选中按钮的下标，未选中时为 -1。
    int currentIndex() const;

protected:
    /// 借用工具按钮样式绘制组末尾的标签文字。
    void paintEvent(QPaintEvent *);

public slots:
    /// 选中指定下标（越界断言），并发出 currentIndexChanged。
    void setCurrentIndex(int index);
    /// 按当前工具与明暗背景重新给按钮着色。
    void colorPaletteChanged();
    /// 切换是否在图标下方显示文字。
    void displayText(QVariant display);

private slots:
    /// 某个动作被触发时同步选中下标并发出 activated。
    void selected(QAction *action);

signals:
    /// 用户点击了某个按钮，携带其下标。
    void activated(int index);
    /// 当前选中项变化，携带新下标。
    void currentIndexChanged(int index);

private:
    QToolButton *mToolButton;      ///< 工具栏原有的第一个按钮，用作样式参照。
    QString mLabel;                ///< 组尾显示的文字（取最后一个动作的文本）。
    QList<QAction *> mActions;     ///< 组内动作列表。
    QList<QToolButton *> mButtons; ///< 与动作一一对应的按钮。
    int mCurrentIndex;             ///< 当前选中下标。
    bool mDisplayLabel;            ///< 是否显示文字标签。
    QActionGroup *mActionGroup;    ///< 互斥动作组。
};
