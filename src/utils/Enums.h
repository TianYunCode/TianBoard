/**
 * @file Enums.h
 * @brief 汇总画板对象类型、绘制工具、层级、图形标志等全局枚举约定。
 */

#pragma once

#include <QtWidgets>

/// 视图最大缩放级别。
#define MAX_ZOOM_LEVEL 9

/** @brief 画板对象的内容类型。 */
struct MimeType
{
    enum Enum
    {
        RasterImage = 0, ///< 位图图像。
        VectorImage,     ///< 矢量图像（SVG 等）。
        AppleWidget,     ///< 旧版 Apple 小部件格式。
        WebWidget,       ///< 网页小部件。
        Video,           ///< 视频。
        Audio,           ///< 音频。
        PDF,             ///< PDF 文档。
        NativeTool,      ///< 内置工具对象。
        Group,           ///< 组合对象。
        UNKNOWN          ///< 无法识别的类型。
    };
};

/** @brief 触控笔/鼠标当前使用的工具。 */
struct StylusTool
{
    enum Enum
    {
        Pen = 0,  ///< 钢笔。
        Eraser,   ///< 橡皮。
        Marker,   ///< 荧光笔。
        Selector, ///< 选择。
        Play,     ///< 播放/浏览。
        Hand,     ///< 平移。
        ZoomIn,   ///< 放大。
        ZoomOut,  ///< 缩小。
        Pointer,  ///< 指针。
        Line,     ///< 直线。
        Text,     ///< 文本。
        Capture   ///< 截屏。
    };
};

/** @brief 笔画粗细档位。 */
struct Width
{
    enum Enum
    {
        Fine = 0,
        Medium,
        Strong
    }; ///< 细、中、粗。
};

/** @brief 缩放步长档位。 */
struct Zoom
{
    enum Enum
    {
        Small = 0,
        Medium,
        Large
    }; ///< 小、中、大。
};

/** @brief 对象尺寸档位。 */
struct Size
{
    enum Enum
    {
        Small = 0,
        Medium,
        Large
    }; ///< 小、中、大。
};

/** @brief 旧版对象层级常量；已废弃，仅为兼容旧版本文档保留。 */
struct ItemLayerType
{
    enum Enum
    {
        FixedBackground = -2000,
        Object          = -1000,
        Graphic         = 0,
        Tool            = 1000,
        Control         = 2000
    }; ///< 各固定层的 Z 值。
};

/** @brief 当前使用的对象层级枚举。 */
struct itemLayerType
{
    enum Enum
    {
        NoLayer = 0,    ///< 未指定层级。
        BackgroundItem, ///< 背景层。
        ObjectItem,     ///< 普通对象层。
        DrawingItem,    ///< 绘制内容层。
        ToolItem,       ///< 工具对象层。
        CppTool,        ///< C++ 实现的内置工具层。
        Eraiser,        ///< 橡皮轨迹层。
        Curtain,        ///< 幕布层。
        Pointer,        ///< 指针层。
        Cache,          ///< 缓存层。
        SelectedItem,   ///< 选中对象层。
        SelectionFrame  ///< 选择框层。
    };
};

/** @brief QGraphicsItem 自定义数据键。 */
struct GraphicsItemData
{
    enum Enum
    {
        ItemLayerType  ///< 已废弃的层级键，仅为兼容旧文档保留。
        ,
        ItemLocked,    ///< 对象是否被锁定。
        ItemEditable   ///< 仅文本对象：是否可编辑。
        ,
        ItemOwnZValue, ///< 对象是否自行管理 Z 值。
        itemLayerType  ///< 当前层级键，取代已废弃的 ItemLayerType。
        ,
        ItemUuid       ///< 对象 UUID，便于快速查找。
        // 复制代理类的功能标志，使纯 QGraphicsItem 也能使用
        ,
        ItemFlippable            ///< 是否可翻转（bool）。
        ,
        ItemRotatable            ///< 是否可旋转（bool）。
        ,
        ItemCanBeSetAsBackground ///< 是否允许设为背景。
    };
};

/** @brief 图形项的自定义 QGraphicsItem 类型 ID。 */
struct GraphicsItemType
{
    enum Enum
    {
        PolygonItemType = QGraphicsItem::UserType + 1, ///< 多边形。
        PixmapItemType,                                ///< 位图。
        SvgItemType,                                   ///< SVG 图形。
        DelegateButtonType,                            ///< 代理按钮。
        MediaItemType,                                 ///< 媒体基类。
        VideoItemType,                                 ///< 视频。
        AudioItemType,                                 ///< 音频。
        PDFItemType,                                   ///< PDF 页。
        TextItemType,                                  ///< 文本。
        CurtainItemType,                               ///< 幕布。
        RulerItemType,                                 ///< 直尺。
        CompassItemType,                               ///< 圆规。
        ProtractorItemType,                            ///< 量角器。
        StrokeItemType,                                ///< 笔画。
        TriangleItemType,                              ///< 三角板。
        MagnifierItemType,                             ///< 放大镜。
        cacheItemType,                                 ///< 缓存项。
        groupContainerType,                            ///< 组合容器。
        ToolWidgetItemType,                            ///< 工具控件。
        GraphicsWidgetItemType,                        ///< 网页小部件。
        UserTypesCount,                                ///< 自定义类型数量。
        SelectionFrameType                             ///< 选择框；必须是本枚举最后一项，用作类型计数。
    };
};

/**
 * @brief 图形项能力标志位。
 *
 * 位掩码可装进 int；GF_COMMON 与 GF_ALL 是若干标志的组合值。
 */
enum GraphicsFlag
{
    GF_NONE = 0x0000                         ///< 无能力。
    ,
    GF_FLIPPABLE_X_AXIS = 0x0001             ///< 可沿 X 轴翻转。
    ,
    GF_FLIPPABLE_Y_AXIS = 0x0002             ///< 可沿 Y 轴翻转。
    ,
    GF_FLIPPABLE_ALL_AXIS = 0x0003           ///< GF_FLIPPABLE_X_AXIS | GF_FLIPPABLE_Y_AXIS。
    ,
    GF_REVOLVABLE = 0x0004                   ///< 可旋转。
    ,
    GF_SCALABLE_X_AXIS = 0x0008              ///< 可沿 X 轴缩放。
    ,
    GF_SCALABLE_Y_AXIS = 0x0010              ///< 可沿 Y 轴缩放。
    ,
    GF_SCALABLE_ALL_AXIS = 0x0018            ///< GF_SCALABLE_X_AXIS | GF_SCALABLE_Y_AXIS。
    ,
    GF_DUPLICATION_ENABLED = 0x0020          ///< 允许复制。
    ,
    GF_MENU_SPECIFIED = 0x0040               ///< 使用对象自定义的右键菜单。
    ,
    GF_ZORDER_MANIPULATIONS_ALLOWED = 0x0080 ///< 允许调整 Z 序。
    ,
    GF_TOOLBAR_USED = 0x0100                 ///< 使用对象工具栏。
    ,
    GF_SHOW_CONTENT_SOURCE = 0x0200          ///< 显示内容来源。
    ,
    GF_RESPECT_RATIO = 0x0418                ///< 保持宽高比。
    ,
    GF_TITLE_BAR_USED = 0x0800               ///< 使用标题栏。
    ,
    GF_COMMON = 0x00F8                       /*0000 0000 1111 1000   GF_SCALABLE_ALL_AXIS
                                                                                           |GF_DUPLICATION_ENABLED
                                                                                           |GF_MENU_SPECIFIED
                                                                                           |GF_ZORDER_MANIPULATIONS_ALLOWED */
    ,
    GF_ALL = 0xFFFF                          ///< 全部能力。
};
Q_DECLARE_FLAGS(GraphicsFlags, GraphicsFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(GraphicsFlags)

/** @brief 文档页面宽高比。 */
struct DocumentSizeRatio
{
    enum Enum
    {
        Ratio4_3 = 0,
        Ratio16_9,
        Custom
    }; ///< 4:3、16:9、自定义。
};

/** @brief 撤销栈中的操作类型。 */
struct UndoType
{
    enum Enum
    {
        undotype_UNKNOWN = 0,          ///< 未知类型。
        undotype_DOCUMENT,             ///< 文档级操作。
        undotype_GRAPHICITEMTRANSFORM, ///< 图形变换。
        undotype_GRAPHICITEM,          ///< 图形对象操作。
        undotype_GRAPHICTEXTITEM,      ///< 文本对象操作。
        undotype_PAGESIZE,             ///< 页面尺寸变化。
        undotype_GRAPHICSGROUPITEM,    ///< 组合对象操作。
        undotype_GRAPHICITEMZVALUE     ///< Z 值变化。
    };
};

/** @brief 页面背景样式：纯色、网格、横线。 */
enum PageBackground
{
    plain = 0,
    crossed,
    ruled
};
