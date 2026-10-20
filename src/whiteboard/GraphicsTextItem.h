/**
 * @file GraphicsTextItem.h
 * @brief 声明白板页面上的可编辑文本图元。
 */

#pragma once

#include <QtWidgets>
#include "whiteboard/Item.h"
#include "utils/Enums.h"
#include "whiteboard/ResizableGraphicsItem.h"

class GraphicsItemDelegate;
class GraphicsScene;

/**
 * @brief 场景里的一段可编辑文字。
 *
 * 四个基类各司其职：QGraphicsTextItem 负责 Qt 的文本排版与编辑，Item 提供白板图元的公共
 * 契约（UUID、深拷贝、层级），ResizableGraphicsItem 声明尺寸读写，GraphicsItem 补齐
 * 图元通用能力。交互与外观（控制手柄、边框、颜色与字号按钮）全部委托给
 * GraphicsTextItemDelegate，本类只做文本与事件转发。
 *
 * 编辑状态机由 mMultiClickState 驱动：单击进入字符级编辑，双击选词，三击选整段，
 * 第四次回到单击。占位文本 mTypeTextHereLabel 只在内容为空且未被编辑时画出，
 * 一旦有真实内容就不再参与尺寸计算。
 *
 * 深/浅两种背景各存一份用色（colorOnDarkBackground / colorOnLightBackground），
 * 切换背景时 recolor() 只换当前显示的那份，另一种背景下的选择不会丢。
 *
 * 所有权：构造期 new 出自己的委托并交给 Item 基类持有；文本内容属于本图元的
 * QTextDocument，退出编辑时自动落盘到文档 XML。
 *
 * @note 线程约束：只能在 GUI 线程创建与操作；undoCommandAdded 依赖 QTextDocument 的
 *       信号，因此文档生命周期必须不长于图元。
 */
class GraphicsTextItem : public QGraphicsTextItem,
                         public Item,
                         public ResizableGraphicsItem,
                         public GraphicsItem {
    Q_OBJECT

public:
    /**
     * @brief 创建文本图元：建委托、置为对象层、开启可选中等标志，并接好文档信号。
     * @param parent 父图元；为空表示直接挂在场景上。
     * @note 委托的控制手柄本应按需创建，目前构造期就建好，是一笔已知欠账。
     */
    GraphicsTextItem(QGraphicsItem *parent = 0);
    virtual ~GraphicsTextItem();

    /// @brief 图元类型标识，供 qgraphicsitem_cast 与分组判定使用。
    enum { Type = GraphicsItemType::TextItemType };

    /// @brief 返回 Type。
    virtual int type() const { return Type; }

    /// @brief 深拷贝出一个内容、字体、颜色与尺寸都相同的新图元（不含选中状态）。
    virtual Item *deepCopy() const;

    /// @brief 把本图元的文本属性复制到已经存在的同类图元上，用于组内替换。
    virtual void copyItemParameters(Item *copy) const;

    /// @brief 所属场景，按 GraphicsScene 返回；不在场景中时为空指针。
    virtual GraphicsScene *scene();

    /// @brief 文本排版后的外接矩形，字号变化后随之增长。
    virtual QRectF boundingRect() const;
    /// @brief 命中区域，与 boundingRect 一致（空白处也能点中，见鼠标事件里的权宜处理）。
    virtual QPainterPath shape() const;

    /// @brief 设置文本排版宽度，换行位置随之改变。
    void setTextWidth(qreal width);
    /// @brief 直接设置排版高度，不触发重新排版。
    void setTextHeight(qreal height);
    /// @brief 当前排版高度。
    qreal textHeight() const;
    /**
     * @brief 首字符的“像素/磅”比值。
     * @return 比值；文本为空或字号为 0 时返回 0。
     * @note 同一台机器上同样的磅值可能渲染出不同像素高，导出 SVG 时必须按这个比值
     *       换算，否则换一台机器字号就变。
     */
    qreal pixelsPerPoint() const;

    /// @brief 内容变化：把场景标脏，并在文本空掉时按占位文本重新取尺寸。
    void contentsChanged();

    /// @brief 调整宽高（同时设置排版宽高并重摆控制手柄）。
    virtual void resize(qreal w, qreal h);

    /// @brief 当前排版尺寸，供缩放与序列化使用。
    virtual QSizeF size() const;

    /// @brief 新建文本沿用最后一次选择的颜色，跨图元共享；进放映态时会被重置。
    static QColor lastUsedTextColor;

    /// @brief 深色背景下用哪个字色。
    QColor colorOnDarkBackground() const { return mColorOnDarkBackground; }

    /// @brief 记下深色背景下的字色（不立即改显示）。
    void setColorOnDarkBackground(QColor pColorOnDarkBackground) {
        mColorOnDarkBackground = pColorOnDarkBackground;
    }

    /// @brief 浅色背景下用哪个字色。
    QColor colorOnLightBackground() const { return mColorOnLightBackground; }

    /// @brief 记下浅色背景下的字色（不立即改显示）。
    void setColorOnLightBackground(QColor pColorOnLightBackground) {
        mColorOnLightBackground = pColorOnLightBackground;
    }

    /// @brief 文本没有外部来源，为满足 Item 契约保留的空实现。
    virtual void clearSource() { ; }
    /// @brief 设置 UUID，同时写进图元数据，供场景按 UUID 找回图元。
    virtual void setUuid(const QUuid &pUuid);
    /// @brief 进出编辑态：true 给编辑器交互，false 退为仅鼠标可选。
    void activateTextEditor(bool activate);
    /// @brief 选中状态变化，顺带切换文本交互标志，避免未选中的文字仍可输入。
    void setSelected(bool selected);
    /// @brief 按当前页面背景套用对应字色。
    void recolor();

    /// @brief 内容为空时画出来的占位提示（构造期 tr 出来，之后不再变）。
    QString mTypeTextHereLabel;

signals:
    /// @brief 文本自己的撤销栈压入了一条命令，上报给场景以便并入白板撤销栈。
    void textUndoCommandAdded(GraphicsTextItem *textItem);

private slots:
    void undoCommandAdded();
    void documentSizeChanged(const QSizeF &newSize);

private:
    /// @brief 单击进编辑、双击选词、三击选段；放映模式下只让出焦点。
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 编辑态下把拖动交给基类做选区。
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 松开后按当前连击次数收尾，必要时把焦点交回场景。
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

    virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);

    /// @brief 画文本；未选中且内容为空时画占位提示。
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    qreal mTextHeight;

    int mMultiClickState;
    QTime mLastMousePressTime;

    QColor mColorOnDarkBackground;
    QColor mColorOnLightBackground;
    bool isActivatedTextEditor;
};
