/**
 * @file GraphicsScene.h
 * @brief 白板页面场景与 Z 序分层控制器的声明。
 */

#pragma once

#include <QtWidgets>

#include "whiteboard/CoreGraphicsScene.h"

#include "utils/Enums.h"

#include "whiteboard/Item.h"
#include "whiteboard/GraphicsCurtainItem.h"

class GraphicsPixmapItem;
class GraphicsProxyWidget;
class GraphicsSvgItem;
class GraphicsPolygonItem;
class GraphicsMediaItem;
class GraphicsWidgetItem;
class GraphicsWebWidgetItem;
class GraphicsAppleWidgetItem;
class ToolWidget;
class GraphicsPDFItem;
class GraphicsTextItem;
class GraphicsRuler;
class GraphicsProtractor;
class GraphicsCompass;
class DocumentProxy;
class GraphicsCurtainItem;
class GraphicsStroke;
class MagnifierParams;
class Magnifier;
class GraphicsCache;
class GraphicsGroupContainerItem;
class SelectionFrame;
class BoardView;

/// @brief 圆周率，画弧时做角度换算用
const double PI = 4.0 * atan(1.0);

/**
 * @brief Z 序分层控制器
 *
 * 把场景的 Z 轴划分成互不重叠的区间（背景、对象、笔迹、工具、橡皮、指针、缓存等），
 * 每层维护自己的上下限与"下一个可分配值"。图元的 Z 值一律经本类生成，
 * 保证不同类型的图元不会在纵深上互相穿插。控制器只引用场景，不拥有它。
 */
class ZLayerController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 单个 Z 层的区间数据
     *
     * bottomLimit/topLimit 是该层允许的 Z 值区间；curValue 是最近一次分配出去的值
     * （初始为 bottomLimit）；incStep 是每次分配的递增步长。
     * 上下限相等表示该层只有一个固定 Z 值（如背景层、选中层）。
     */
    struct ItemLayerTypeData
    {
        ItemLayerTypeData() : bottomLimit(0), topLimit(0), curValue(0), incStep(1)
        {
        }
        ItemLayerTypeData(qreal bot, qreal top, qreal increment = 1) : bottomLimit(bot), topLimit(top), curValue(bot), incStep(increment)
        {
        }
        qreal bottomLimit;
        qreal topLimit;
        qreal curValue;
        qreal incStep;
    };

    /**
     * @brief Z 序调整的目标位置
     *
     * up 上移一层、down 下移一层、top 升到本层最高、bottom 降到本层最低。
     */
    enum moveDestination
    {
        up,
        down,
        top,
        bottom
    };

    /// @brief 层类型到区间数据的映射
    typedef QMap<itemLayerType::Enum, ItemLayerTypeData> ScopeMap;

    /// @brief 构造控制器并注册全部 Z 层区间；scene 仅被引用，不被拥有
    ZLayerController(QGraphicsScene *scene);

    /// @brief 指定层的 Z 值下限（未注册的层返回 0）
    qreal getBottomLimit(itemLayerType::Enum key) const
    {
        return scopeMap.value(key).bottomLimit;
    }
    /// @brief 指定层的 Z 值上限（未注册的层返回 0）
    qreal getTopLimit(itemLayerType::Enum key) const
    {
        return scopeMap.value(key).topLimit;
    }
    /// @brief key 是否为已注册的层类型
    bool validLayerType(itemLayerType::Enum key) const
    {
        return scopeMap.contains(key);
    }

    /// @brief 出错时使用的哨兵 Z 值，低于所有合法层
    static qreal errorNum()
    {
        return errorNumber;
    }

    /// @brief 在指定层内分配下一个可用 Z 值；层不存在时返回 errorNum()
    qreal generateZLevel(itemLayerType::Enum key);
    /// @brief 按图元 data 里记录的层类型分配 Z 值；类型无效时返回 errorNum()
    qreal generateZLevel(QGraphicsItem *item);

    /**
     * @brief 把图元移动到目标 Z 序位置
     * @param item 待调整的图元，须已带有效层类型
     * @param dest 目标位置（上移/下移/置顶/置底）
     * @return 调整后图元的自身 Z 值；图元不在任何已注册层时返回 errorNum()
     * @note 只在同层图元之间交换"自身 Z 值"数据，最后才写回 setZValue；
     *       期间会清空并重建场景选中，以触发选择框刷新。
     */
    qreal changeZLevelTo(QGraphicsItem *item, moveDestination dest);
    /// @brief 读取图元的层类型；未注册的类型一律归为 NoLayer
    itemLayerType::Enum typeForData(QGraphicsItem *item) const;
    /// @brief 把图元标记为新的层类型（只写 data，不改动 Z 值）
    void setLayerType(QGraphicsItem *pItem, itemLayerType::Enum pNewType);
    /// @brief 外部把图元 Z 值改大之后，把对应层的分配水位抬到不低于 zValue
    void shiftStoredZValue(QGraphicsItem *item, qreal zValue);

    /// @brief z 是否尚未被场景中任何图元占用
    bool zLevelAvailable(qreal z);

private:
    ScopeMap        scopeMap;
    static qreal    errorNumber;
    QGraphicsScene *mScene;
};

/**
 * @brief 白板页面场景
 *
 * 一页白板对应一个 GraphicsScene，职责：
 * - 图元增删（addItem/removeItem 系列），入场景时自动分配 Z 值；
 * - 笔迹输入：inputDevicePress/Move/Release 驱动 GraphicsStroke 收集采样点，
 *   松手时简化并打包成 GraphicsStrokesGroup 落回场景；
 * - 背景（明暗、格线）、页面标称尺寸与视图状态的管理；
 * - 通过 ZLayerController 做 Z 序分层，通过 RenderingContext 区分
 *   屏幕/离屏/PDF 导出/录屏四种绘制场合。
 *
 * 场景以所属 DocumentProxy 为 QObject 父对象，生命周期随文档；场景不拥有文档。
 * 所有接口只允许在 GUI 线程调用。
 */
class GraphicsScene : public CoreGraphicsScene, public Item
{
    Q_OBJECT

public:
    /**
     * @brief clearContent() 的清理范围
     *
     * clearItemsAndAnnotations 清对象与批注、clearAnnotations 只清笔迹组、
     * clearItems 只清普通对象（保留笔迹）、clearBackground 只清背景图元。
     */
    enum clearCase
    {
        clearItemsAndAnnotations = 0,
        clearAnnotations,
        clearItems,
        clearBackground
    };

    /// @brief 打开撤销重做开关（构造后一次性启用）
    void enableUndoRedoStack()
    {
        mUndoRedoStackEnabled = true;
    }
    /// @brief 设置是否把增删操作压入全局撤销栈（加载文档等场合需临时关闭）
    void setURStackEnable(bool enable)
    {
        mUndoRedoStackEnabled = enable;
    }
    /// @brief 撤销栈开关是否打开
    bool isURStackIsEnabled()
    {
        return mUndoRedoStackEnabled;
    }

    /**
     * @brief 构造页面场景
     * @param parent 所属文档代理，同时作为 QObject 父对象
     * @param enableUndoRedoStack 增删图元时是否压入全局撤销栈
     */
    GraphicsScene(DocumentProxy *parent, bool enableUndoRedoStack = true);
    /// @brief 析构：清理未落完的当前笔迹与分层控制器
    virtual ~GraphicsScene();

    /// @brief Item 接口实现，等价于 sceneDeepCopy()
    virtual Item *deepCopy() const;

    /// @brief 场景没有需要单独拷贝的图元参数，空实现
    virtual void copyItemParameters(Item *copy) const
    {
        Q_UNUSED(copy);
    }

    /**
     * @brief 深拷贝整个场景（背景、页面尺寸与全部可见图元）
     * @return 新场景，所有权交给调用方
     * @note 隐藏图元不拷贝；组会重建并保留锁定状态。
     */
    GraphicsScene *sceneDeepCopy() const;

    /// @brief 按 pCase 清空场景内容，操作可通过全局撤销栈回退
    void clearContent(clearCase pCase = clearItemsAndAnnotations);

    /**
     * @brief 输入设备（触控笔/鼠标）按下
     * @param scenePos 场景坐标
     * @param pressure 压感 0~1（直线工具忽略）
     * @return 事件是否被当前工具处理
     * @note 若上一笔未正常抬起，会先按"移动"语义消化本次按下。
     */
    bool inputDevicePress(const QPointF &scenePos, const qreal &pressure = 1.0);
    /// @brief 输入设备移动：按当前工具画线/擦除/移动预览圆；@return 是否被处理
    bool inputDeviceMove(const QPointF &scenePos, const qreal &pressure = 1.0);
    /**
     * @brief 输入设备抬起：收尾当前笔迹（简化、打包成 GraphicsStrokesGroup）并压入撤销命令
     * @return 本次按压是否产生了场景改动
     */
    bool inputDeviceRelease();

    /// @brief 光标离开场景时隐藏橡皮、预览圆等工具图元
    void leaveEvent(QEvent *event);

    /// @brief 添加图元：必要时分配 Z 值，并登记到快速访问列表
    /// @note 所有权移交场景；删除走 CoreGraphicsScene 的延迟删除机制。
    void addItem(QGraphicsItem *item);
    /// @brief 移除图元（不立即 delete）；单例缓存图元会被真正删除以便下次重建
    void removeItem(QGraphicsItem *item);

    /// @brief 批量添加图元，总是重新分配 Z 值
    void addItems(const QSet<QGraphicsItem *> &item);
    /// @brief 批量移除图元（不立即 delete）
    void removeItems(const QSet<QGraphicsItem *> &item);

    /// @brief 按 URL 识别类型并添加小部件；类型未知时返回 0
    GraphicsWidgetItem *addWidget(const QUrl &pWidgetUrl, const QPointF &pPos = QPointF(0, 0));
    /// @brief 添加 Apple 风格小部件
    GraphicsAppleWidgetItem *addAppleWidget(const QUrl &pWidgetUrl, const QPointF &pPos = QPointF(0, 0));
    /// @brief 添加 Web 小部件
    GraphicsWebWidgetItem *addWebWidget(const QUrl &pWidgetUrl, const QPointF &pPos = QPointF(0, 0));
    /// @brief 把已构造的小部件图元放入场景（不可作内容的小部件转投控制端视图）
    void addGraphicsWidget(GraphicsWidgetItem *graphicsWidget, const QPointF &pPos = QPointF(0, 0));

    /// @brief 视图最近一次停留的场景中心点
    QPointF lastCenter();
    /// @brief 记录当前场景中心，切页时用于恢复浏览位置
    void setLastCenter(QPointF center);

    /**
     * @brief 添加音视频媒体图元
     * @param pMediaFileUrl 媒体文件地址（本地路径或 URL）
     * @param shouldPlayAsap 加入场景后立即开始播放
     * @return 文件不存在或媒体类型不支持时返回 NULL
     */
    GraphicsMediaItem *addMedia(const QUrl &pMediaFileUrl, bool shouldPlayAsap, const QPointF &pPos = QPointF(0, 0));
    /// @brief addMedia() 的别名，语义上表示添加视频
    GraphicsMediaItem *addVideo(const QUrl &pVideoFileUrl, bool shouldPlayAsap, const QPointF &pPos = QPointF(0, 0));
    /// @brief addMedia() 的别名，语义上表示添加音频
    GraphicsMediaItem *addAudio(const QUrl &pAudioFileUrl, bool shouldPlayAsap, const QPointF &pPos = QPointF(0, 0));
    /**
     * @brief 添加 SVG 图元，并先把内容持久化到文档的图片目录
     * @param pData 非空时直接使用内嵌数据，忽略 pSvgFileUrl 指向的内容
     * @return 写盘失败返回 nullptr（此时不会入场景、不压撤销命令）
     */
    GraphicsSvgItem *addSvg(const QUrl &pSvgFileUrl, const QPointF &pPos = QPointF(0, 0), const QByteArray pData = QByteArray());
    /// @brief 用当前设置的字体添加纯文本图元
    GraphicsTextItem *addText(const QString &pString, const QPointF &pTopLeft = QPointF(0, 0));

    /**
     * @brief 用指定字体添加纯文本图元
     * @param pointSize 小于 1 时回落到设置里的像素字号
     * @param fontFamily 空串时回落到设置里的字体族
     */
    GraphicsTextItem *addTextWithFont(const QString &pString, const QPointF &pTopLeft = QPointF(0, 0), int pointSize = -1,
        const QString &fontFamily = "", bool bold = false, bool italic = false);
    /// @brief 添加富文本图元，HTML 先经 TextTools::cleanHtml() 清洗
    GraphicsTextItem *addTextHtml(const QString &pString = QString(), const QPointF &pTopLeft = QPointF(0, 0));

    /// @brief 把 items 打包成一个组加入场景（嵌套组会被拆平）；@return 新组
    GraphicsGroupContainerItem *createGroup(QList<QGraphicsItem *> items);
    /// @brief 把已构建好的组加入场景并登记撤销命令
    void addGroup(GraphicsGroupContainerItem *groupItem);

    /**
     * @brief 把图元设为页面背景：不可选、不可动、压到背景 Z 层
     * @param pAdaptTransformation 是否缩放以适配页面尺寸
     * @param expand true 时即使小于页面也放大铺满
     * @return 实际作为背景的图元（可能已被缩放调整）
     * @note 旧背景会先被移除。
     */
    QGraphicsItem *setAsBackgroundObject(QGraphicsItem *item, bool pAdaptTransformation = false, bool expand = false);
    /// @brief 取消背景标记并恢复可选中/可移动；层类型与 Z 值由调用方按对象来源重设
    void unsetBackgroundObject();

    /// @brief 当前背景图元，没有则为 0
    QGraphicsItem *backgroundObject() const
    {
        return mBackgroundObject;
    }

    /// @brief item 是否就是当前背景图元
    bool isBackgroundObject(const QGraphicsItem *item) const
    {
        return item == mBackgroundObject;
    }

    /// @brief 等比缩放到页面标称尺寸之内（expand 为真时小图也放大）；center 为真时居中
    QGraphicsItem *scaleToFitDocumentSize(QGraphicsItem *item, bool center = false, int margin = 0, bool expand = false);

    /// @brief 覆盖标称页面与全部可见内容的矩形；ratio>0 时按该宽高比扩展
    QRectF normalizedSceneRect(qreal ratio = -1.0);

    /// @brief 按 UUID 线性查找图元，找不到返回 0
    QGraphicsItem *itemForUuid(QUuid uuid);

    /// @brief 设定绘制起点，并重置笔宽、圆弧与"上次线段"状态
    void moveTo(const QPointF &pPoint);
    /// @brief 从上一点画直线段到 pEndPoint 并挂到当前笔迹；bLineStyle 为真表示直线工具（先清掉本次已画段）
    void drawLineTo(const QPointF &pEndPoint, const qreal &pWidth, bool bLineStyle);
    /// @brief 同上，起点与终点分别用 pStartWidth、endWidth（压感渐变段）
    void drawLineTo(const QPointF &pEndPoint, const qreal &pStartWidth, const qreal &endWidth, bool bLineStyle);
    /// @brief 用橡皮线段擦除：与橡皮相交的笔迹多边形被拆分替换，被整块覆盖的直接移除
    void eraseLineTo(const QPointF &pEndPoint, const qreal &pWidth);
    /// @brief 圆规画弧：以 pCenterPoint 为圆心、上一笔点为半径端点，重建当前弧多边形
    void drawArcTo(const QPointF &pCenterPoint, qreal pSpanAngle);
    /// @brief 把带宽度的采样点列画成平滑曲线段（points 为 点+宽度 对）
    void drawCurve(const QList<QPair<QPointF, qreal>> &points);
    /// @brief 把等宽渐变的采样点列画成平滑曲线段（宽度从 startWidth 过渡到 endWidth）
    void drawCurve(const QList<QPointF> &points, qreal startWidth, qreal endWidth);

    /// @brief 场景中是否没有内容图元（直尺等工具不计入）
    bool isEmpty() const;

    /// @brief 改挂到另一个文档代理；离开旧文档前会先把它标记为已修改
    void setDocument(DocumentProxy *pDocument);

    /// @brief 场景所属的文档代理
    DocumentProxy *document() const
    {
        return mDocument;
    }

    /// @brief 当前是否深色背景
    bool isDarkBackground() const
    {
        return mDarkBackground;
    }

    /// @brief 当前是否浅色背景
    bool isLightBackground() const
    {
        return !mDarkBackground;
    }

    /// @brief 页面底纹类型（空白/格线/横线）
    PageBackground pageBackground() const
    {
        return mPageBackground;
    }

    /// @brief 底纹格线间距（像素）
    int backgroundGridSize() const
    {
        return mBackgroundGridSize;
    }

    /// @brief 是否开启网格吸附（全应用共用工具栏上的那个开关）。
    bool isSnapping() const;

    /**
     * @brief 把一个场景坐标吸到最近的网格交点。
     * @return 需要叠加的位移；没开吸附或没有可吸附的就是 (0,0)。
     */
    QPointF snapToGrid(const QPointF &point) const;

    /**
     * @brief 在一组图元角点里挑吸得力最强的那个位移。
     * @param snapIndex 带回选中的是第几个角点；没角点参与时为 -1。
     */
    QPointF snapCorners(const QList<QPointF> &corners, int *snapIndex = nullptr) const;

    /// @brief 把一个场景矩形按吸得力最强的那个角吸到网格。
    QPointF snapToGrid(const QRectF &rect) const;

    /**
     * @brief 计算图元用于吸附的外框。
     *
     * 有画笔的图形按笔宽把框缩回去（否则吸上之后线宽的一半会压过网格线）；
     * 只有一段的直线笔迹则退回它那两个端点构成的框。
     */
    static QRectF itemRectForSnapping(const QGraphicsItem *item);

    /// @brief 是否设置了背景图元
    bool hasBackground()
    {
        return (mBackgroundObject != 0);
    }

    /// @brief 在 center 处添加直尺（工具层，不属于文档内容）
    void addRuler(QPointF center);
    /// @brief 在 center 处添加量角器（工具层）
    void addProtractor(QPointF center);
    /// @brief 在 center 处添加圆规（工具层）
    void addCompass(QPointF center);
    /// @brief 在指定场景位置放一个坐标轴图元。
    void addAxes(QPointF center);
    /// @brief 在 center 处添加三角板（工具层）
    void addTriangle(QPointF center);
    /// @brief 打开放大镜：控制端与显示端各建一个窗口；已有放大镜时忽略
    void addMagnifier(MagnifierParams params);

    /// @brief 添加幕布（遮罩），初始覆盖页面中央一半区域
    void addMask(const QPointF &center = QPointF());
    /// @brief 把单例缓存图元加入场景并选中
    void addCache();
    /**
     * @brief 通知总控器"这页出现了缓存图元"，让笔栏切到缓存态。
     * @note 交互路径（addCache）与加载路径（从盘上读回一张带缓存的页）都要说这一句，
     * 通知对象只有画板自己认识，因此谁都不必去拿全局控制器。
     */
    void notifyCacheEnabled();

    /// @brief 顶层图元的快速访问列表（组成员不在其中）
    QList<QGraphicsItem *> getFastAccessItems()
    {
        return mFastAccessItems;
    }

    /**
     * @brief 视图状态快照：缩放系数、滚动条位置与场景中心
     *
     * 每页各存一份，切页时保存/恢复，使页面记住自己的浏览位置。
     */
    class SceneViewState
    {
    public:
        /// @brief 默认状态：缩放 1、滚动 0、中心在原点
        SceneViewState()
        {
            zoomFactor         = 1;
            horizontalPosition = 0;
            verticalPostition  = 0;
            mLastSceneCenter   = QPointF();
        }

        /// @brief 用指定的缩放系数与滚动条位置构造
        SceneViewState(qreal pZoomFactor, int pHorizontalPosition, int pVerticalPostition, QPointF sceneCenter = QPointF())
        {
            zoomFactor         = pZoomFactor;
            horizontalPosition = pHorizontalPosition;
            verticalPostition  = pVerticalPostition;
            mLastSceneCenter   = sceneCenter;
        }

        /// @brief 最近一次记录的场景中心点
        QPointF lastSceneCenter()
        {
            return mLastSceneCenter;
        }

        /// @brief 记录当前场景中心点
        void setLastSceneCenter(QPointF center)
        {
            mLastSceneCenter = center;
        }

        /// @brief 最近一次记录的场景中心点
        QPointF mLastSceneCenter;

        /// @brief 视图缩放系数
        qreal zoomFactor;
        /// @brief 水平滚动条位置
        int horizontalPosition;
        /// @brief 垂直滚动条位置（成员名沿用了历史拼写 Postition）
        int verticalPostition;
    };

    /// @brief 取回本页保存的视图状态
    SceneViewState viewState() const
    {
        return mViewState;
    }

    /// @brief 保存本页的视图状态
    void setViewState(const SceneViewState &pViewState)
    {
        mViewState = pViewState;
    }

    /// @brief 把渲染质量广播给场景中所有实现了 Item 接口的图元
    virtual void setRenderingQuality(Item::RenderingQuality pRenderingQuality);

    /// @brief 场景引用的图片/音视频/小部件资源在文档包内的相对路径（保存打包用）
    QList<QUrl> relativeDependencies() const;

    /// @brief 页面标称尺寸；从未设置过时回落到文档的默认尺寸
    QSize nominalSize();

    /// @brief 页面实际尺寸：有 PDF 背景时取其场景包围盒，否则为 nominalSize()
    QSize sceneSize();

    /// @brief 设置页面标称尺寸，并同步为文档默认尺寸
    void setNominalSize(const QSize &pSize);

    /// @brief 设置页面标称尺寸（宽高形式）
    void setNominalSize(int pWidth, int pHeight);

    /// @brief 调整图元 Z 序；addUndo 为真时压入撤销命令；@return 新的自身 Z 值
    qreal changeZLevelTo(QGraphicsItem *item, ZLayerController::moveDestination dest, bool addUndo = false);

    /**
     * @brief 渲染目标上下文
     *
     * Screen 正常上屏；NonScreen 离屏渲染（缩略图等，包含 PDF 背景）；
     * PdfExport 导出 PDF（剔除 PDF 背景图元避免重复绘制）；
     * Recording 录屏（只画内容层，剔除工具与选中框）。
     */
    enum RenderingContext
    {
        Screen = 0,
        NonScreen,
        PdfExport,
        Recording
    };

    /// @brief 设置当前渲染上下文，drawItems()/drawBackground() 据此过滤
    void setRenderingContext(RenderingContext pRenderingContext)
    {
        mRenderingContext = pRenderingContext;
    }

    /// @brief 当前渲染上下文
    RenderingContext renderingContext() const
    {
        return mRenderingContext;
    }

    /// @brief 已注册的工具图元集合（直尺、橡皮预览等，不算文档内容）
    QSet<QGraphicsItem *> tools()
    {
        return mTools;
    }

    /// @brief 把图元登记为工具（不计入内容计数，导出/录制时被剔除）
    void registerTool(QGraphicsItem *item)
    {
        mTools << item;
    }

    /// @brief 上一次绘制/擦除的终点，即下一段的起点
    const QPointF &previousPoint()
    {
        return mPreviousPoint;
    }

    /// @brief 把图元临时抬到选中层，保证选中时盖在其他内容之上
    void setSelectedZLevel(QGraphicsItem *item);
    /// @brief 恢复图元存储的自身 Z 值（取消选中时调用）
    void setOwnZlevel(QGraphicsItem *item);

    /// @brief 读取图元 data 里的 UUID；未设置时返回空 QUuid
    static QUuid getPersonalUuid(QGraphicsItem *item);

    /// @brief 由多边形创建笔迹多边形图元（已按当前工具着色，未入场景）
    GraphicsPolygonItem *polygonToPolygonItem(const QPolygonF pPolygon);
    /// @brief 清空多选框圈住的图元列表（框本身留在场景）
    void clearSelectionFrame();
    /// @brief 返回控制端视图（教师屏）；未找到返回 0
    BoardView *controlView();
    /// @brief 外部改动图元 Z 值后，同步分层控制器的分配水位
    void notifyZChanged(QGraphicsItem *item, qreal zValue);
    /// @brief 取消除 graphicsItem 外所有图元的选中（文本图元同时退出编辑态）
    void deselectAllItemsExcept(QGraphicsItem *graphicsItem);

    /// @brief 除工具外全部图元的联合包围盒
    QRectF annotationsBoundingRect() const;

public slots:
    /// @brief 按当前选中项刷新界面：单选显示图元控件，多选显示选择框
    void updateSelectionFrame();
    /// @brief 适配 QUndoStack::indexChanged(int) 的包装，撤销/重做后刷新选择框
    void updateSelectionFrameWrapper(int);
    /// @brief 开始一条新笔迹（由绘制控制器在程序化落笔前调用）
    void initStroke();
    /// @brief 隐藏橡皮、马克笔/钢笔预览圆等工具图元
    void hideTool();

    /// @brief 设置明暗背景与页面底纹；明暗变化时重着色全部图元并刷新视图缓存
    void setBackground(bool pIsDark, PageBackground pBackground);
    /// @brief 记录当前缩放系数，底纹在缩放过小时淡出或不画
    void setBackgroundZoomFactor(qreal zoom);
    /// @brief 设置底纹格线间距（像素，须为正）并刷新视图缓存
    void setBackgroundGridSize(int pSize);
    /// @brief bModeDesktop 为真表示桌面注解模式，背景绘制交回基类
    void setDrawingMode(bool bModeDesktop);
    /// @brief 取消全部选中，隐藏选择框并让文本图元退出编辑态
    void deselectAllItems();

    /**
     * @brief 添加图片图元，并把 PNG 持久化到文档的图片目录
     * @param replaceFor 被本图替换的旧图元（进撤销命令），没有则传 0
     * @param pPos 图元中心的目标位置
     * @param scaleFactor 缩放系数
     * @param pUseAnimation 是否用 1 秒淡入动画
     * @param useProxyForDocumentPath true 用本场景所属文档的目录，false 用当前选中文档
     */
    GraphicsPixmapItem *addPixmap(const QPixmap &pPixmap, QGraphicsItem *replaceFor, const QPointF &pPos = QPointF(0, 0), qreal scaleFactor = 1.0,
        bool pUseAnimation = false, bool useProxyForDocumentPath = false);

    /// @brief 文本图元请求压入一条文本撤销命令
    void textUndoCommandAdded(GraphicsTextItem *textItem);

    /// @brief 切换工具：选择/文本/播放工具需清选中、隐藏预览圆并丢弃空笔迹
    void setToolCursor(int tool);

    /// @brief 选中变化时把首个选中项的 Z 值显示到状态栏（调试用途）
    void selectionChangedProcessing();
    /// @brief 按放大镜当前位置重新抓取画面
    void moveMagnifier();
    /// @brief 把控制端与显示端放大镜移动到 newPos（控制端容器坐标）
    void moveMagnifier(QPoint newPos, bool forceGrab = false);
    /// @brief 关闭并销毁两端放大镜窗口
    void closeMagnifier();
    /// @brief 放大镜倍率 +0.5（上限 8）
    void zoomInMagnifier();
    /// @brief 放大镜倍率 -0.5（下限 1）
    void zoomOutMagnifier();
    /// @brief 切换放大镜的绘制模式
    void changeMagnifierMode(int mode);
    /// @brief 调整放大镜窗口尺寸（占场景百分比，仅接受 18~50）
    void resizedMagnifier(qreal newPercent);

protected:
    /// @brief 由等宽线段生成多边形图元（未入场景）
    GraphicsPolygonItem *lineToPolygonItem(const QLineF &pLine, const qreal &pWidth);
    /// @brief 由变宽线段生成多边形图元（未入场景）
    GraphicsPolygonItem *lineToPolygonItem(const QLineF &pLine, const qreal &pStartWidth, const qreal &pEndWidth);

    /// @brief 由圆弧（起始半径 + 跨越角）生成多边形图元（未入场景）
    GraphicsPolygonItem *arcToPolygonItem(const QLineF &pStartRadius, qreal pSpanAngle, qreal pWidth);
    /// @brief 由带宽度的采样点列生成平滑曲线多边形（未入场景）
    GraphicsPolygonItem *curveToPolygonItem(const QList<QPair<QPointF, qreal>> &points);
    /// @brief 由等宽渐变的采样点列生成平滑曲线多边形（未入场景）
    GraphicsPolygonItem *curveToPolygonItem(const QList<QPointF> &points, qreal startWidth, qreal endWidth);
    /// @brief 多边形入场景并挂到当前笔迹；半透明时先减去与本笔已画多边形的重叠，避免叠加变色
    void addPolygonItemToCurrentStroke(GraphicsPolygonItem *polygonItem);

    /// @brief 按当前工具（钢笔/马克笔）与明暗背景初始化多边形颜色及层类型
    void initPolygonItem(GraphicsPolygonItem *);

    /// @brief 把橡皮预览圆移到 pEndPoint 并按当前橡皮宽度设置大小
    void drawEraser(const QPointF &pEndPoint, bool pressed = true);
    /// @brief 显示橡皮圈：按下时实线、悬停时虚线
    void redrawEraser(bool pressed);
    /// @brief 隐藏橡皮圈
    void hideEraser();
    /// @brief 绘制/移动红色激光指示点
    void drawPointer(const QPointF &pEndPoint, bool isFirstDraw = false);
    /// @brief 未落笔时显示马克笔的笔宽预览圆
    void drawMarkerCircle(const QPointF &pEndPoint);
    /// @brief 未落笔时显示钢笔的笔宽预览圆（同时把视口光标换成空白，靠圆点指示位置）
    void drawPenCircle(const QPointF &pEndPoint);
    /// @brief 隐藏马克笔预览圆
    void hideMarkerCircle();
    /// @brief 隐藏钢笔预览圆
    void hidePenCircle();
    /// @brief 销毁两端放大镜窗口并恢复被覆盖的光标
    void DisposeMagnifierQWidgets();

    /// @brief 处理 Ctrl+A 全选与 Delete 删除（文本编辑态下的图元不删）
    virtual void keyReleaseEvent(QKeyEvent *keyEvent);

    /// @brief 明暗背景切换后，把笔迹组与文本图元换成对应背景下的颜色
    void recolorAllItems();

    /// @brief 按渲染上下文过滤待绘图元：导出剔除工具与 PDF 背景，录制只画内容层
    virtual void drawItems(QPainter *painter, int numItems, QGraphicsItem *items[], const QStyleOptionGraphicsItem options[], QWidget *widget = 0);

    /// @brief 沿父链返回图元所在的顶层图元
    QGraphicsItem *rootItem(QGraphicsItem *item) const;

    /// @brief 画底色与格线/横线底纹；桌面注解模式下直接交回基类
    virtual void drawBackground(QPainter *painter, const QRectF &rect);

private:
    void setDocumentUpdated();
    void createEraiser();
    void createPointer();
    void createMarkerCircle();
    void createPenCircle();
    void updateEraserColor();
    void updateMarkerCircleColor();
    void updatePenCircleColor();
    bool hasTextItemWithFocus(GraphicsGroupContainerItem *item);
    void simplifyCurrentStroke();

    QGraphicsEllipseItem *mEraser;
    QGraphicsEllipseItem *mPointer;
    QGraphicsEllipseItem *mMarkerCircle;
    QGraphicsEllipseItem *mPenCircle;

    QSet<QGraphicsItem *> mAddedItems;
    QSet<QGraphicsItem *> mRemovedItems;

    DocumentProxy *mDocument;

    bool           mDarkBackground;
    PageBackground mPageBackground;
    int            mBackgroundGridSize;

    bool  mIsDesktopMode;
    qreal mZoomFactor;

    QGraphicsItem *mBackgroundObject;

    QPointF mPreviousPoint;
    qreal   mPreviousWidth;
    qreal   mDistanceFromLastStrokePoint;

    QList<GraphicsPolygonItem *> mPreviousPolygonItems;

    SceneViewState mViewState;

    bool mInputDeviceIsPressed;

    QSet<QGraphicsItem *> mTools;

    GraphicsPolygonItem *mArcPolygonItem;

    QSize mNominalSize;

    RenderingContext mRenderingContext;

    GraphicsStroke *mCurrentStroke;

    int mItemCount;

    QList<QGraphicsItem *> mFastAccessItems;

    bool mHasCache;
    bool mUndoRedoStackEnabled;

    Magnifier *magnifierControlViewWidget;
    Magnifier *magnifierDisplayViewWidget;

    ZLayerController    *mZLayerController;
    GraphicsPolygonItem *mpLastPolygon;
    GraphicsPolygonItem *mTempPolygon;

    bool                 mDrawWithCompass;
    GraphicsPolygonItem *mCurrentPolygon;
    SelectionFrame      *mSelectionFrame;
};
