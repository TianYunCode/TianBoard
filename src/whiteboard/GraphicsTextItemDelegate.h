/**
 * @file GraphicsTextItemDelegate.h
 * @brief 声明文本图元的交互委托：字体/颜色/字号/对齐按钮与它们的编辑行为。
 */

#pragma once

#include <QtWidgets>

#include <QtSvg>
#include <QtSvgWidgets>

#include "utils/Enums.h"
#include "whiteboard/GraphicsItemDelegate.h"

class GraphicsTextItem;

/**
 * @brief 对齐按钮：一个按钮在左对齐 / 居中 / 右对齐之间循环，跨段落选中时显示“混合”态。
 *
 * 四种状态各自持有一个 QSvgRenderer，绘制时按当前状态切换共享渲染器。渲染器用
 * QPointer 持有，析构里显式 delete，因为它们是手动 new 出来的裸对象而非 Qt 子对象。
 *
 * @note 隐藏“混合”态时（mHideMixed），循环上限降到 MAX_KIND-1，setKind(k_mixed) 会被忽略。
 */
class AlignTextButton : public DelegateButton
{
    Q_OBJECT

public:
    /// @brief 可循环的状态数，不含 k_mixed；配合 mHideMixed 使用。
    static const int MAX_KIND = 3;
    /// @brief 按钮所处（或即将切到）的对齐状态。
    enum kind_t
    {
        k_left = 0, ///< 左对齐
        k_center,   ///< 居中
        k_right,    ///< 右对齐
        k_mixed     ///< 选区跨段落且各段对齐不同
    };

    /**
     * @brief 建按钮并准备好四种状态的 SVG 渲染器。
     * @param fileName 图标路径前缀，四种状态按 <前缀>_left/_center/_right/_mixed 找文件。
     * @param pDelegated 按钮所属的图元。
     * @param parent 父图元。
     * @param section 按钮贴在委托边框的哪一侧。
     */
    AlignTextButton(
        const QString &fileName, QGraphicsItem *pDelegated, QGraphicsItem *parent = 0, Qt::WindowFrameSection section = Qt::TopLeftSection);
    virtual ~AlignTextButton();

    /// @brief 切到指定状态；mHideMixed 时拒绝 k_mixed。
    void setKind(int pKind);
    /// @brief 当前状态。
    int kind()
    {
        return mKind;
    }

    /// @brief 前进到下一个状态并立即生效。
    void setNextKind();
    /// @brief 下一个会落到哪个状态，但不改变自身。
    int nextKind() const;

    /// @brief 是否参与“混合”态；隐藏后循环只在三种对齐之间转。
    void setMixedButtonVisible(bool v = true)
    {
        mHideMixed = !v;
    }
    /// @brief 当前是否允许显示混合状态。
    bool isMixedButtonVisible()
    {
        return !mHideMixed;
    }

private:
    /// @brief 按状态取对应的渲染器，未知状态返回空指针。
    QSvgRenderer *rndFromKind(int pknd)
    {
        switch (pknd)
        {
            case k_left:
                return lft;
                break;
            case k_center:
                return cntr;
                break;
            case k_right:
                return rght;
                break;
            case k_mixed:
                return mxd;
                break;
        }

        return 0;
    }

    /// @brief 当前状态正在用的渲染器。
    QSvgRenderer *curRnd()
    {
        return rndFromKind(mKind);
    }

    QPointer<QSvgRenderer> lft;  ///< 左对齐图标
    QPointer<QSvgRenderer> cntr; ///< 居中图标
    QPointer<QSvgRenderer> rght; ///< 右对齐图标
    QPointer<QSvgRenderer> mxd;  ///< 混合态图标

    int  mKind;                  ///< 当前对齐状态
    bool mHideMixed;             ///< 是否禁用混合态
};

/**
 * @brief 挂在 GraphicsTextItem 上的委托：文本的编辑能力与外观按钮。
 *
 * 在通用委托（选中框、缩放手柄、删除/复制）之上加五个按钮：字体、颜色、减小字号、
 * 增大字号、对齐；再管三件事——是否可编辑、按背景色换字色、把选区/字号改动落成可撤销的
 * 一步。
 *
 * 改字号有两条路：changeSize 直接按磅值加减 delta，scaleSize 按倍数缩放；两者都靠
 * ChangeTextSize() 把选区按“字号/字体/粗斜/下划线/颜色”切成连续片段逐段套用，因为
 * QTextDocument 不支持给一个混排选区整体设一次字号。改前先 saveTextCursorFormats()
 * 存下 HTML 与选区，失败或需要回退时 restoreTextCursorFormats() 还原。
 *
 * @note 按钮由 createControls() 建立；未建立时 titleBarWidth() 返回 0。
 * @note 线程约束：只在 GUI 线程操作，字体与颜色对话框是模态的，会挡住画板交互。
 */
class GraphicsTextItemDelegate : public GraphicsItemDelegate
{
    Q_OBJECT

    /// @brief 改字号的两条路子：按步长加减，或按倍数缩放。
    enum textChangeMode
    {
        changeSize = 0,
        scaleSize
    };

public:
    /**
     * @brief 建立文本委托。
     * @param pDelegated 被委托的文本图元。
     * @param parent 父对象；为空时由图元接管。
     */
    GraphicsTextItemDelegate(GraphicsTextItem *pDelegated, QObject *parent = 0);
    virtual ~GraphicsTextItemDelegate();
    /// @brief 当前是否允许输入文字（只读模式下仍可拖动与选中）。
    bool isEditable();
    /// @brief 按倍数缩放字号；factor 为 1 时什么都不做。
    void scaleTextSize(qreal multiplyer);
    /// @brief 按页面背景把文字切换成对应的那份颜色。
    void recolor();
    /// @brief 选中状态变化时清掉遗留的文本选区，避免选中框与高亮同时出现。
    virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);
    /// @brief 创建五个外观按钮并挂到委托边框上。
    virtual void createControls();
    /// @brief 标题栏区按钮占的像素宽度；控件还没建时返回 0。
    qreal titleBarWidth();

public slots:
    /// @brief 文本内容变了，通知图元标脏并在空文本时重取尺寸。
    void contentsChanged();
    /// @brief 开/关文字编辑能力。
    virtual void setEditable(bool);
    /// @brief 删除文本前先退出编辑态，再走通用删除流程。
    virtual void remove(bool canUndo);

protected:
    /// @brief 追加“可编辑”菜单项。
    virtual void decorateMenu(QMenu *menu);
    /// @brief 同步“可编辑”菜单项的勾选状态。
    virtual void updateMenuActionState();

    /// @brief 摘掉本委托特有的按钮。
    virtual void freeButtons();

    virtual bool mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual bool mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual bool mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual bool keyPressEvent(QKeyEvent *event);
    virtual bool keyReleaseEvent(QKeyEvent *event);

private:
    /// @brief 把通用委托对象还原成文本图元。
    GraphicsTextItem *delegated();

    DelegateButton *mFontButton;         ///< 字体选择按钮
    DelegateButton *mColorButton;        ///< 颜色选择按钮
    DelegateButton *mDecreaseSizeButton; ///< 减小字号
    DelegateButton *mIncreaseSizeButton; ///< 增大字号
    DelegateButton *mAlignButton;        ///< 对齐循环按钮（AlignTextButton）

    int mLastFontPixelSize;              ///< 上次用过的像素字号，-1 表示还没有记录

    static const int sMinPixelSize;      ///< 像素字号下限，再小就不给减
    static const int sMinPointSize;      ///< 磅值下限，再小就不给减

private:
    /// @brief 过滤字体对话框的字体列表，深色背景下补一份可读的样式。
    void customize(QFontDialog &fontDialog);
    /// @brief 按模式改选区字号，内部切成样式一致的连续片段逐段套用。
    void ChangeTextSize(qreal factor, textChangeMode changeMode);
    /// @brief 依据选区落在一段还是跨段，设置对齐按钮的状态。
    void updateAlighButtonState();
    /// @brief 选区是否完全落在同一个段落内（无选区算“是”）。
    bool oneBlockSelection();
    /// @brief 存下当前 HTML 与选区，供批量改样式前后对照。
    void saveTextCursorFormats();
    /// @brief 还原 saveTextCursorFormats() 存下的内容与选区。
    void restoreTextCursorFormats();

    /// @brief 按当前设置拼出新建文本用的默认字体。
    QFont    createDefaultFont();
    QAction *mEditableAction; ///< “可编辑”菜单项，勾选态跟随图元状态

    /// @brief 一次按钮拖动期间的选区快照，用于松开时合成一条撤销命令。
    struct selectionData_t
    {
        selectionData_t() : mButtonIsPressed(false)
        {
        }
        bool                    mButtonIsPressed; ///< 按钮是否仍处于按下状态
        int                     position;         ///< 光标位置
        int                     anchor;           ///< 选区锚点
        QString                 html;             ///< 改动前的整段 HTML
        QTextDocumentFragment   selection;        ///< 改动前的选区内容
        QList<QTextBlockFormat> fmts;             ///< 改动前各段落的块格式

    } mSelectionData;

private slots:
    /// @brief 弹字体对话框，选定后写回设置并应用到选区。
    void pickFont();
    /// @brief 弹颜色对话框，选定后更新两种背景下的字色。
    void pickColor();

    /// @brief 按 delta 减小字号，触到底限就不动。
    void decreaseSize();
    /// @brief 按 delta 增大字号。
    void increaseSize();

    /// @brief 对齐按钮被点：切状态并应用到选区。
    void alignButtonProcess();
    /// @brief 文档内容变化时把场景标脏。
    void onContentChanged();

private:
    const int delta; ///< 每次点字号按钮跨过的磅数
};
