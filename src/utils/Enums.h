#pragma once

#include <QtWidgets>

#define MAX_ZOOM_LEVEL 9

struct MimeType {
    enum Enum {
        RasterImage = 0,
        VectorImage,
        AppleWidget,
        WebWidget,
        Video,
        Audio,
        PDF,
        NativeTool,
        Group,
        UNKNOWN
    };
};

struct StylusTool {
    enum Enum {
        Pen = 0,
        Eraser,
        Marker,
        Selector,
        Play,
        Hand,
        ZoomIn,
        ZoomOut,
        Pointer,
        Line,
        Text,
        Capture
    };
};

struct Width {
    enum Enum { Fine = 0, Medium, Strong };
};

struct Zoom {
    enum Enum { Small = 0, Medium, Large };
};

struct Size {
    enum Enum { Small = 0, Medium, Large };
};

// Deprecated. Keep it for backward campability with old versions
struct ItemLayerType {
    enum Enum { FixedBackground = -2000, Object = -1000, Graphic = 0, Tool = 1000, Control = 2000 };
};

struct itemLayerType {
    enum Enum {
        NoLayer = 0,
        BackgroundItem,
        ObjectItem,
        DrawingItem,
        ToolItem,
        CppTool,
        Eraiser,
        Curtain,
        Pointer,
        Cache,
        SelectedItem,
        SelectionFrame
    };
};

struct GraphicsItemData {
    enum Enum {
        ItemLayerType //Deprecated. Keep it for backward campability with old versions. Use itemLayerType instead
        ,
        ItemLocked,
        ItemEditable //for text only
        ,
        ItemOwnZValue,
        itemLayerType //use instead of deprecated ItemLayerType
        ,
        ItemUuid //storing uuid in QGraphicsItem for fast finding operations
        //Duplicating delegate's functions to make possible working with pure QGraphicsItem
        ,
        ItemFlippable // (bool)
        ,
        ItemRotatable // (bool)
        ,
        ItemCanBeSetAsBackground
    };
};

struct GraphicsItemType {
    enum Enum {
        PolygonItemType = QGraphicsItem::UserType + 1,
        PixmapItemType,
        SvgItemType,
        DelegateButtonType,
        MediaItemType,
        VideoItemType,
        AudioItemType,
        PDFItemType,
        TextItemType,
        CurtainItemType,
        RulerItemType,
        CompassItemType,
        ProtractorItemType,
        StrokeItemType,
        TriangleItemType,
        MagnifierItemType,
        cacheItemType,
        groupContainerType,
        ToolWidgetItemType,
        GraphicsWidgetItemType,
        UserTypesCount,
        SelectionFrameType // this line must be the last line in this enum because it is types counter.
    };
};

// Might be fit in int value under most OS
enum GraphicsFlag {
    GF_NONE = 0x0000 //0000 0000 0000 0000
    ,
    GF_FLIPPABLE_X_AXIS = 0x0001 //0000 0000 0000 0001
    ,
    GF_FLIPPABLE_Y_AXIS = 0x0002 //0000 0000 0000 0010
    ,
    GF_FLIPPABLE_ALL_AXIS = 0x0003 //0000 0000 0000 0011 GF_FLIPPABLE_X_AXIS | GF_FLIPPABLE_Y_AXIS
    ,
    GF_REVOLVABLE = 0x0004 //0000 0000 0000 0100
    ,
    GF_SCALABLE_X_AXIS = 0x0008 //0000 0000 0000 1000
    ,
    GF_SCALABLE_Y_AXIS = 0x0010 //0000 0000 0001 0000
    ,
    GF_SCALABLE_ALL_AXIS = 0x0018 //0000 0000 0001 1000 GF_SCALABLE_X_AXIS | GF_SCALABLE_Y_AXIS
    ,
    GF_DUPLICATION_ENABLED = 0x0020 //0000 0000 0010 0000
    ,
    GF_MENU_SPECIFIED = 0x0040 //0000 0000 0100 0000
    ,
    GF_ZORDER_MANIPULATIONS_ALLOWED = 0x0080 //0000 0000 1000 0000
    ,
    GF_TOOLBAR_USED = 0x0100 //0000 0001 0000 0000
    ,
    GF_SHOW_CONTENT_SOURCE = 0x0200 //0000 0010 0000 0000
    ,
    GF_RESPECT_RATIO = 0x0418 //0000 0100 0001 1000
    ,
    GF_TITLE_BAR_USED = 0x0800 //0000 1000 0000 0000
    ,
    GF_COMMON = 0x00F8 /*0000 0000 1111 1000   GF_SCALABLE_ALL_AXIS
                                                                     |GF_DUPLICATION_ENABLED
                                                                     |GF_MENU_SPECIFIED
                                                                     |GF_ZORDER_MANIPULATIONS_ALLOWED */
    ,
    GF_ALL = 0xFFFF //1111 1111 1111 1111
};
Q_DECLARE_FLAGS(GraphicsFlags, GraphicsFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(GraphicsFlags)

struct DocumentSizeRatio {
    enum Enum { Ratio4_3 = 0, Ratio16_9, Custom };
};

struct UndoType {
    enum Enum {
        undotype_UNKNOWN = 0,
        undotype_DOCUMENT,
        undotype_GRAPHICITEMTRANSFORM,
        undotype_GRAPHICITEM,
        undotype_GRAPHICTEXTITEM,
        undotype_PAGESIZE,
        undotype_GRAPHICSGROUPITEM,
        undotype_GRAPHICITEMZVALUE
    };
};

enum PageBackground { plain = 0, crossed, ruled };
