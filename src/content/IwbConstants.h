/**
 * @file IwbConstants.h
 * @brief 汇总 IWB 1.0 文档包读写用到的文件名、XML 标签名、属性名、命名空间与格式白名单。
 *
 * 本文件中的每个字符串都会写进 .iwb/.tbz 包内的 XML，或用于解析既有文件，属于磁盘格式
 * 契约：改动取值会让导出文件偏离 IWB 规范，或让导入识别不到旧文件里的元素。新增条目时
 * 请对照 IWB 规范补充说明，既有取值一律保持原样。
 */

#pragma once

#define PI 3.1415926535

/// 背景矩形的 z 序，取足够小的值，保证始终位于所有页面内容之下。
const int DEFAULT_BACKGROUND_LAYER = -20000002;
/// 背景网格线的 z 序，比背景矩形高一层。
const int DEFAULT_BACKGROUND_CROSS_LAYER = -20000001;

// 包内固定文件名与内置资源路径
const QString fMetadata = "metadata.rdf";
const QString fIWBContent = "content.xml";
const QString fIWBBackground = "background.png";
const QString sAudioElementImage = ":images/soundOn.svg";

// 内部状态消息：既作为“无错误”的哨兵值，也会被转换器写入 errorStr。
const QString noErrorMsg = "NoError";

// IWB 内容 XML 的标签名。tPage* 系列来自文档包（.tbz）页面，tIWB* 系列来自 IWB 内容。
const QString tDescription = "Description";
const QString tIWBRoot = "iwb";
const QString tIWBMeta = "meta";
const QString tPageSize = "size";
const QString tSvg = "svg";
const QString tIWBPage = "page";
const QString tIWBPageSet = "pageset";
const QString tId = "id";
const QString tElement = "element";
const QString tPageGroup = "group";
const QString tPageGroups = "groups";
const QString tPageG = "g";
const QString tPagePolygon = "polygon";
const QString tPagePolyline = "polyline";
const QString tPageLine = "line";
const QString tPageAudio = "audio";
const QString tPageVideo = "video";
const QString tPageImage = "image";
const QString tPageForeignObject = "foreignObject";
const QString tPageTextContent = "itemTextContent";

const QString tIWBA = "a";
const QString tIWBG = "g";
const QString tIWBSwitch = "switch";
const QString tIWBImage = "image";
const QString tIWBVideo = "video";
const QString tIWBAudio = "audio";
const QString tIWBText = "text";
const QString tIWBTextArea = "textarea";
const QString tIWBPolyLine = "polyline";
const QString tIWBPolygon = "polygon";
const QString tIWBFlash = "video";
const QString tIWBRect = "rect";
const QString tIWBLine = "line";
const QString tIWBTbreak = "tbreak";
const QString tIWBTspan = "tspan";

// IWB 内容 XML 的属性名。注意大小写敏感：aIWBViewBox 是小写 "viewbox"（IWB 规范），
// aPageViewBox 是驼峰 "viewBox"（SVG 规范），两者不可混用。
const QString aIWBVersion = "version";
const QString aOwner = "owner";
const QString aDescription = "description";
const QString aCreator = "creator";
const QString aAbout = "about";
const QString aIWBViewBox = "viewbox";
const QString aPageViewBox = "viewBox";
const QString aDarkBackground = "dark-background";
const QString aBackground = "background";
const QString aCrossedBackground = "crossed-background";
const QString aPageType = "type";
const QString aPageUuid = "uuid";
const QString aPageParent = "parent";
const QString aFill = "fill";

const QString aID = "id";
const QString aRef = "ref";
const QString aSVGHref = "xlink:href";
const QString aIWBHref = "ref";
const QString aPageHref = "href";
const QString aPageSource = "source";
const QString aSrc = "src";
const QString aSVGRequiredExtension = "requiredExtensions";

const QString aX = "x";
const QString aY = "y";
const QString aWidth = "width";
const QString aHeight = "height";
const QString aStroke = "stroke";
const QString aStrokeWidth = "stroke-width";
const QString aPoints = "points";
const QString aZLayer = "z-value";
const QString aLayer = "layer";
const QString aTransform = "transform";
const QString aLocked = "locked";
const QString aIWBName = "name";
const QString aIWBContent = "content";


// 属性取值：布尔值在 IWB 中统一用小写字符串表示，不能用 XML 的 true/false 简写。
const QString avIWBVersionNo = "1.0";
const QString avPageText = "text";
const QString avFalse = "false";
const QString avTrue = "true";

// 命名空间 URI 与前缀。IWB 页面命名空间与 IWB 元素命名空间指向同一个 URI，
// 解析时必须按 URI 而不是前缀判断，否则遇到第三方改写前缀的文件会误判。
const QString svgPageNS = "http://www.imsglobal.org/xsd/iwb_v1p0";
const QString svgIWBNS = "http://www.w3.org/2000/svg";
const QString xlinkNS = "http://www.w3.org/1999/xlink";
const QString iwbNS = "http://www.imsglobal.org/xsd/iwb_v1p0";
const QString dcNSPrefix = "dc";
const QString svgIWBNSPrefix = "svg";
const QString xlinkNSPrefix = "xlink";
const QString iwbNsPrefix = "iwb";
const QString xsiPrefix = "xsi";
const QString xsiSchemaLocationPrefix = "schemaLocation";

const QString avOwner = "";
const QString avCreator = "";
const QString avDescription = "";

// 常量符号与字面量
// 尺寸串分隔符：document size 用 "1024x768"，viewBox 用 "0 0 1024 768"。
const QString dimensionsDelimiter1 = "x";
const QString dimensionsDelimiter2 = " ";
const QString pageAlias = "page";
const QString pageFileExtension = "svg";

// 包内媒体子目录名
// IWB 包内媒体子目录名；与 iwbPackageContentFolders 保持一致。
const QString cfImages = "images";
const QString cfVideos = "video";
const QString cfAudios = "audio";
const QString cfFlash = "flash";

// 已知文件扩展名
// 扩展名不带点。feWgt 是 Web Widget 包，导出时转成 png 快照。
const QString feSvg = "svg";
const QString feWgt = "wgt";
const QString fePng = "png";

// 背景网格的间距与线宽（像素）。
const int iCrossSize = 32;
const int iCrossWidth = 1;

// 允许直接拷贝进 IWB 包的图片扩展名白名单；itIsSupportedFormat() 按子串匹配。
const QString iwbElementImage(" \
wgt, \
jpeg, \
jpg, \
bmp, \
gif, \
wmf, \
emf, \
png, \
tif, \
tiff \
");

const QString iwbElementVideo(" \
mpg, \
mpeg, \
swf, \
");

// IWB 支持的音频格式
// IWB 不认 ogg/flac，只保留规范列出的 mp3/wav。
const QString iwbElementAudio(" \
mp3, \
wav \
");

// 导出时的格式分派：可直接支持的格式原样拷贝，其余格式（svg）需要先栅格化。
const QString iwbSupportedFileFormats(iwbElementImage + iwbElementVideo + iwbElementAudio);
const QString iwbPackageFormatsToConvert("svg");

// 各元素类型允许带进 IWB 的属性白名单，供 itIsSVGElementAttribute() 过滤未知属性。
// 条目名与 IWB 规范里的 SVG 子集属性一一对应，拼写错误会静默丢属性。
const QString iwbSVGImageAttributes(" \
id, \
xlink:href, \
x, \
y, \
height, \
width, \
fill-opacity, \
requiredExtentions, \
transform \
");

const QString iwbSVGAudioAttributes(" \
id, \
xlink:href, \
x, \
y, \
height, \
width, \
fill-opacity, \
requiredExtentions, \
transform \
");

const QString iwbSVGVideoAttributes(" \
id, \
xlink:href, \
x, \
y, \
height, \
width, \
fill-opacity, \
requiredExtentions, \
transform \
");

const QString iwbSVGRectAttributes(" \
id, \
x, \
y, \
height, \
width, \
fill, \
fill-opacity, \
stroke, \
stroke-dasharray, \
stroke-linecap, \
stroke-linejoin, \
stroke-opacity, \
stroke-width, \
transform \
");

const QString iwbSVGTextAttributes(" \
id, \
x, \
y, \
fill, \
font-family, \
font-size, \
font-style, \
font-weight, \
font-stretch, \
transform \
");

const QString iwbSVGTextAreaAttributes(" \
id, \
x, \
y, \
height, \
width, \
fill, \
font-family, \
font-size, \
font-style, \
font-weight, \
font-stretch, \
text-align, \
transform \
");

const QString iwbSVGTspanAttributes(" \
id, \
fill, \
font-family, \
font-size, \
font-style, \
font-weight, \
font-stretch, \
text-align, \
");

const QString iwbSVGLineAttributes(" \
id, \
x1, \
y1, \
x2, \
y2, \
stroke, \
stroke-dasharray, \
stroke-width, \
stroke-opacity, \
stroke-linecap, \
transform \
");

const QString iwbSVGPolyLineAttributes(" \
id, \
points, \
stroke, \
stroke-width, \
stroke-dasharray, \
stroke-opacity, \
stroke-linecap, \
transform \
");

const QString iwbSVGPolygonAttributes(" \
id, \
points, \
fill, \
fill-opacity, \
stroke, \
stroke-dasharray, \
stroke-width, \
stroke-linecap, \
stroke-linejoin, \
stroke-opacity, \
stroke-width, \
transform \
");

// IWB 扩展段（<iwb:element>）允许携带的元素级属性；不在列表中的属性只写进 SVG 部分。
const QString iwbElementAttributes(" \
background, \
background-fill, \
background-posture, \
flip, \
freehand, \
highlight, \
highlight-fill, \
list-style-type, \
list-style-type-fill, \
locked, \
replicate, \
revealer, \
stroke-lineshape-start, \
stroke-lineshape-end \
");

// 页面元素上需要转换而非直接拷贝的属性：href/src 要重写为新包内路径，
// transform/uuid 要拆成 IWB 的几何属性与元素 ID。
const QString iwbPackageAttributesToConvert(" \
xlink:href, \
src, \
transform, \
uuid \
");

const QString svgElementAttributes(" \
points, \
fill, \
fill-opacity, \
stroke, \
stroke-dasharray, \
stroke-linecap, \
stroke-opacity, \
stroke-width, \
stroke_linejoin, \
requiredExtensions, \
viewbox, \
x, \
y, \
x1, \
y1, \
x2, \
y2, \
height, \
width, \
font-family, \
font-size, \
font-style, \
font-weight, \
font-stretch, \
text-align \
");

const QString iwbPackageContentFolders("audios,videos,images,widgets");

/**
 * @brief 场景图元的分层常量，数值越小越靠后。
 *
 * 与 IWB 的 z-value 语义一致：背景固定在最低层，导入对象位于笔迹之下，工具和控件在最上层。
 */
struct ItemLayerType {
    /**
     * @brief 各层取值。
     * - FixedBackground：页面背景
     * - Object：导入的图片/PDF 等对象
     * - Graphic：普通笔迹与图形
     * - Tool：尺规等工具
     * - Control：控件类图元
     */
    enum Enum { FixedBackground = -2000, Object = -1000, Graphic = 0, Tool = 1000, Control = 2000 };
};
