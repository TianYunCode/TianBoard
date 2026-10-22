/**
 * @file ThumbnailWidget.h
 * @brief 缩略图网格及配套图元类的声明：页面预览、选择、拖拽和悬浮操作按钮。
 */

#pragma once

#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>
#include <QElapsedTimer>
#include <QGraphicsSceneHoverEvent>

#include "whiteboard/BoardController.h"
#include "whiteboard/CoreGraphicsScene.h"
#include "settings/Settings.h"
#include "whiteboard/Item.h"
#include "model/DocumentProxy.h"

#include <QLabel>
#include <QHBoxLayout>

#include <memory>

#define STARTDRAGTIME 1000000
#define BUTTONSIZE    96
#define BUTTONSPACING 10

class DocumentProxy;
class ThumbnailTextItem;
class Thumbnail;

/**
 * @brief 以网格形式展示一组页面/素材缩略图的图形视图。
 *
 * 界面上是按列排布的缩略图与其下方标题，支持单击、Ctrl/Shift 多选、
 * 键盘方向键导航、橡皮筋框选以及把选中项拖出为 URL 列表。由
 * DocumentThumbnailWidget 继承后放进 DocumentsWidget 的右侧框架，
 * 场景与图元都由自己持有，设置变化（宽度/间距/尺寸）时重新排版。
 */
class ThumbnailWidget : public QGraphicsView
{
    Q_OBJECT

public:
    /// 构造视图：读取默认缩略图宽度与间距，启用拖动并绑定内部场景。
    ThumbnailWidget(QWidget *parent);
    virtual ~ThumbnailWidget();

    /// 返回按行、列排序后的选中图元。
    QList<QGraphicsItem *> selectedItems();
    /// 选中第 index 个图元，extend 为 true 时保留原有选择。
    void selectItemAt(int pIndex, bool extend = false);
    /// 取消第 index 个图元的选中状态。
    void unselectItemAt(int pIndex);

    /// 返回当前缩略图宽度。
    qreal thumbnailWidth();

    /// 设置缩略图场景的背景画刷。
    void setBackgroundBrush(const QBrush &brush);

public slots:
    /// 修改缩略图宽度并重新排版。
    void setThumbnailWidth(qreal pThumbnailWidth);
    /// 修改图元间距并重新排版。
    void setSpacing(qreal pSpacing);
    /**
     * @brief 用一批图元、路径与标题重建网格。
     * @param pGraphicsItems 直接放入缩略图场景的图形项。
     * @param pItemPaths 与图元一一对应的源文件路径，拖拽时使用。
     * @param pLabels 缩略图下方标题，数量需与路径一致。
     * @param pMimeType 拖拽时附加的自定义 MIME 类型。
     * @note 会先清空旧图元与旧标题（标题项由场景负责删除）。
     */
    virtual void setGraphicsItems(const QList<QGraphicsItem *> &pGraphicsItems, const QList<QUrl> &pItemPaths,
        const QStringList pLabels = QStringList(), const QString &pMimeType = QString(""));
    /// 按当前宽度与间距重新计算列数、缩放和图元位置。
    void refreshScene();
    /// 场景选择变化时转发 selectionChanged 信号。
    void sceneSelectionChanged();

signals:
    /// 视图尺寸变化并重新排版后发出。
    void resized();
    /// 选中项发生变化。
    void selectionChanged();
    /// 双击某个缩略图，携带图元与它在列表中的下标。
    void mouseDoubleClick(QGraphicsItem *item, int index);
    /// 单击某个缩略图，携带图元与下标（当前下标固定为 0）。
    void mouseClick(QGraphicsItem *item, int index);

protected:
    /// 记录按下位置；区分框选、Shift 连选与普通点选。
    virtual void mousePressEvent(QMouseEvent *event);
    /// 拖出橡皮筋框选，或在已有选择时发起拖拽。
    virtual void mouseMoveEvent(QMouseEvent *event);
    /// 结束框选并发出 mouseClick；接收方只看有没有发生点击，用它刷新动作可用性。
    virtual void mouseReleaseEvent(QMouseEvent *event);
    /// 尺寸变化后重新排版并发出 resized。
    virtual void resizeEvent(QResizeEvent *event);
    /// 找到命中的缩略图并发出 mouseDoubleClick。
    void mouseDoubleClickEvent(QMouseEvent *event);

    /// 方向键/Home/End/Ctrl+A 的选择导航。
    virtual void keyPressEvent(QKeyEvent *event);
    /// 通过 Tab 获得焦点且无选中项时自动选中第一项。
    virtual void focusInEvent(QFocusEvent *event);

    /// 返回承载缩略图的图元列表。
    const QList<QGraphicsItem *> &graphicItems() const;
    /// 返回标题图元列表。
    const QList<ThumbnailTextItem *> &labelsItems() const;
    /// 返回鼠标按下位置（视图坐标）。
    const QPoint &mousePressPos() const;

    /// 返回当前图元间距。
    qreal spacing();
    /// 设置是否允许把选中项拖出。
    void setCanDrag(bool canDrag);

private:
    /// 选中全部图元。
    void selectAll();
    /// 选中从 startIndex 开始的 count 个图元，其余取消选中。
    void selectItems(int startIndex, int count);
    /// 返回网格行数（由最后一个图元的行号推得）。
    int rowCount() const;
    /// 返回网格列数（由最后一个图元的行/列推得）。
    int columnCount() const;

    /// 排序谓词：先按行、再按列比较。
    static bool thumbnailLessThan(QGraphicsItem *item1, QGraphicsItem *item2);

    /// 从场景中移除并删除框选矩形。
    void deleteLasso();

    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 缩略图在图元层面共用的辅助行为。
 *
 * 记录图元在网格中的行列、对应标题，并维护一个比图元稍大的选中边框图元；
 * 不单独使用，而是作为 ThumbnailPixmap 的第二个基类，让像素版缩略图也能
 * 参与网格排版与选中框绘制。
 */
class Thumbnail
{
public:
    /// 创建选中边框图元（此时尚未加入场景）。
    Thumbnail();

    virtual ~Thumbnail();

    /// 复制样式选项并清掉 Selected 状态，避免 Qt 在选中时再画一层虚线框。
    QStyleOptionGraphicsItem muteStyleOption(const QStyleOptionGraphicsItem *option)
    {
        QStyleOptionGraphicsItem styleOption  = QStyleOptionGraphicsItem(*option);
        styleOption.state                    &= ~QStyle::State_Selected;

        return styleOption;
    }

    /// 图元选中/变换/移动时同步显示或隐藏选中边框。
    virtual void itemChange(QGraphicsItem *item, QGraphicsItem::GraphicsItemChange change, const QVariant &value);

    /// 返回图元所在列。
    int column();
    /// 设置图元所在列。
    void setColumn(int column);
    /// 返回图元所在行。
    int row();
    /// 设置图元所在行。
    void setRow(int row);
    /// 返回图元下方的标题项。
    ThumbnailTextItem *label();
    /// 设置图元下方的标题项。
    void setLabel(ThumbnailTextItem *label);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 缩略图下方的单行标题。
 *
 * 文本过长时在设定宽度内省略右侧；页面导航里被高亮时改用蓝色。由
 * ThumbnailWidget::setGraphicsItems 创建，随缩略图场景一起销毁。
 */
class ThumbnailTextItem : public QGraphicsTextItem
{
    Q_OBJECT
public:
    /// 按页码生成“Page N”标题。
    ThumbnailTextItem(int index);

    /// 用给定文本作为标题。
    ThumbnailTextItem(const QString &text);

    virtual ~ThumbnailTextItem();

    /// 返回固定高度、宽度为 setWidth 设定值的包围盒。
    QRectF boundingRect() const;

    /// 设置标题最大宽度并重新计算省略文本。
    void setWidth(qreal pWidth);

    /// 返回标题当前宽度。
    qreal width();

    /// 把标题切换为高亮配色。
    void highlight();

    /// 按当前宽度截断文本并刷新显示。
    void computeText();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 带网格元数据的位图缩略图。
 *
 * 在 QGraphicsPixmapItem 的基础上叠加 Thumbnail 的行列与选中框行为，
 * 绘制时屏蔽 Qt 自带的选中态；形状取整个包围盒，便于框选命中。
 */
class ThumbnailPixmap : public QGraphicsPixmapItem, public Thumbnail
{
public:
    /// 用给定图片构造缩略图，启用平滑缩放与几何变化通知。
    ThumbnailPixmap(const QPixmap &pix) : QGraphicsPixmapItem(pix)
    {
        setTransformationMode(Qt::SmoothTransformation);
        setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }

    virtual ~ThumbnailPixmap()
    {
        // 刻意留空
    }

    /// 命中区域为整个包围盒。
    virtual QPainterPath shape() const
    {
        QPainterPath path;
        path.addRect(boundingRect());
        return path;
    }

    /// 以去掉选中态的样式绘制图片。
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
    {
        QStyleOptionGraphicsItem styleOption = Thumbnail::muteStyleOption(option);
        QGraphicsPixmapItem::paint(painter, &styleOption, widget);
    }

    /// 先交给 Thumbnail 维护选中框，再交给基类处理。
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value)
    {
        Thumbnail::itemChange(this, change, value);
        return QGraphicsPixmapItem::itemChange(change, value);
    }
};

/**
 * @brief 绑定到文档代理与页面下标的位图缩略图。
 *
 * 除显示图片外还记住自己属于哪个 DocumentProxy、对应第几个场景，
 * 供页面导航和删除/复制页面时定位目标。
 */
class SceneThumbnailPixmap : public ThumbnailPixmap
{
public:
    /// 用页面截图、所属文档代理和场景下标构造。
    SceneThumbnailPixmap(const QPixmap &pix, DocumentProxy *proxy, int pSceneIndex);

    virtual ~SceneThumbnailPixmap();

    /// 返回所属文档代理。
    DocumentProxy *proxy();

    /// 返回对应的场景下标。
    int sceneIndex();

    /// 预留的高亮入口；当前实现为空。
    void highlight();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 页面导航面板中的缩略图，悬浮时显示删除/复制图标。
 *
 * 悬停后进入“可编辑”状态，在缩略图上绘制关闭与复制图标；点击图标分别删除
 * 或复制对应页面。仅当页面来自有效场景（pageFromSceneIndex >= 0）时才启用
 * 悬浮与选择。由 DocumentNavigator 创建并放入导航场景。
 */
class SceneThumbnailNavigPixmap : public SceneThumbnailPixmap
{
public:
    /// 构造并（页面有效时）启用悬浮事件与选中标志。
    SceneThumbnailNavigPixmap(const QPixmap &pix, DocumentProxy *proxy, int pSceneIndex);
    ~SceneThumbnailNavigPixmap();

    /// 返回是否处于显示操作图标的可编辑状态。
    bool editable();

    /// 文档多于 1 页时才允许删除。
    bool deletable();

    /// 不是第一页时才能上移。
    bool movableUp();

    /// 不是最后一页时才能下移。
    bool movableDown();

    /// 进入可编辑状态（显示图标）。
    void showUI();

    /// 退出可编辑状态（隐藏图标）。
    void hideUI();

    /// 设置可编辑标志。
    void setEditable(bool editable);

protected:
    /// 鼠标进入时显示操作图标并重绘。
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    /// 鼠标离开时隐藏操作图标并重绘。
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    /// 先画缩略图，再在可编辑时叠加删除/复制图标。
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    /// 命中图标时执行删除/复制，否则忽略事件。
    void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    /// 询问用户后删除本页。
    void deletePage();
    /// 复制本页。
    void duplicatePage();
    /// 把本页上移一位（当前没有入口调用）。
    void moveUpPage();
    /// 把本页下移一位（当前没有入口调用）。
    void moveDownPage();

    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 缩略图与标题的组合，负责在网格单元中摆放两者。
 *
 * 不是图形项，只是 DocumentNavigator 排版时使用的轻量容器：持有两个图元的
 * 裸指针并记录边框宽度。图元本身由场景管理。
 */
class ImgTextThumbnailElement
{
private:
    SceneThumbnailNavigPixmap *thumbnail;
    ThumbnailTextItem         *caption;
    int                        border;

public:
    /// 用缩略图与标题构造，边框默认为 0。
    ImgTextThumbnailElement(SceneThumbnailNavigPixmap *thumb, ThumbnailTextItem *text) : border(0)
    {
        this->thumbnail = thumb;
        this->caption   = text;
    }

    /// 返回缩略图图元。
    SceneThumbnailNavigPixmap *getThumbnail() const
    {
        return this->thumbnail;
    }
    /// 替换缩略图图元。
    void setThumbnail(SceneThumbnailNavigPixmap *newGItem)
    {
        this->thumbnail = newGItem;
    }

    /// 返回标题图元。
    ThumbnailTextItem *getCaption() const
    {
        return this->caption;
    }
    /// 替换标题图元。
    void setCaption(ThumbnailTextItem *newcaption)
    {
        this->caption = newcaption;
    }

    /// 把两个图元缩放并摆到第 row 行、第 col 列的单元格中。
    void Place(int row, int col, qreal width, qreal height);

    /// 返回边框宽度。
    int getBorder() const
    {
        return this->border;
    }
    /// 设置边框宽度。
    void setBorder(int newBorder)
    {
        this->border = newBorder;
    }
};

/**
 * @brief 缩略图悬浮操作图标的绘制工具集。
 *
 * 图标从 SVG 预渲染成 96x96 的位图，按位置（第几个图标）横向排列；
 * 通过 x 坐标判断点击了哪个图标，通过 y 坐标判断是否落在图标条上。
 */
namespace ThumbnailUI
{
    const int ICONSIZE    = 96; ///< 单个图标的边长。
    const int ICONSPACING = 10; ///< 图标之间的间距。

    /**
     * @brief 一个悬浮操作图标（关闭/复制/上移/下移）。
     */
    class ThumbnailUIIcon : public QPixmap
    {
    public:
        /// 从 SVG 文件渲染图标，pos 决定它在图标条中的横向序号。
        ThumbnailUIIcon(const QString &filename, int pos);

        ~ThumbnailUIIcon();

        /// 返回图标在图标条中的序号。
        int pos() const;

        /// 判断横坐标 x 是否落在本图标范围内。
        bool triggered(qreal x) const;

    private:
        struct Private;
        std::unique_ptr<Private> d;
    };

    namespace _private
    {
        static QMap<QString, ThumbnailUIIcon *> catalog; ///< 按名称缓存已渲染的图标。
        /// 向缓存中注册全部内置图标（close/duplicate/moveUp/moveDown 及其禁用版）。
        void initCatalog();
    } // namespace _private

    /**
     * @brief 渲染并登记一个图标。
     * @param thumbnailIcon 图标名（对应 :images/<name>.svg）。
     * @param pos 图标在图标条中的位置。
     * @return 新建的图标对象，登记失败时返回的指针仍需调用方判断。
     */
    ThumbnailUIIcon *addIcon(const QString &thumbnailIcon, int pos);
    /// 按名称取图标；未登记时打印错误并返回空指针。
    ThumbnailUIIcon *getIcon(const QString &thumbnailIcon);
    /// 把图标画到 painter 对应位置（y=0）。
    void draw(QPainter *painter, const ThumbnailUIIcon &thumbnailIcon);
    /// 判断纵坐标 y 是否落在图标条的高度范围内。
    bool triggered(qreal y);
} // namespace ThumbnailUI
