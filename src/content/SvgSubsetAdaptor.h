/**
 * @file SvgSubsetAdaptor.h
 * @brief 页面数据的 SVG 子集读写器：场景与 pageNNN.svg 之间的双向转换。
 */

#pragma once

#include <QtWidgets>
#include <QtXml>
#include <QGraphicsItem>

#include "utils/GeometryUtils.h"

class GraphicsSvgItem;
class GraphicsPolygonItem;
class GraphicsPixmapItem;
class GraphicsPDFItem;
class GraphicsWidgetItem;
class GraphicsMediaItem;
class GraphicsVideoItem;
class GraphicsAudioItem;
class GraphicsAppleWidgetItem;
class GraphicsWebWidgetItem;
class GraphicsTextItem;
class GraphicsCurtainItem;
class GraphicsRuler;
class GraphicsCompass;
class GraphicsProtractor;
class GraphicsScene;
class DocumentProxy;
class GraphicsStroke;
class PersistenceManager;
class GraphicsTriangle;
class GraphicsCache;
class GraphicsGroupContainerItem;
class GraphicsStrokesGroup;

/**
 * @brief 页面持久化格式（SVG 子集）的读写门面。
 *
 * 每个页面存成文档目录下的 pageNNN.svg：SVG 负责几何与画笔，自定义命名空间下的
 * 属性负责控件状态、分组信息和 z 序。读取时 SvgSubsetReader 按文件版本做增量迁移
 * （upgradeScene 复用同一套规则），写出时 SvgSubsetWriter 从 GraphicsScene 生成 DOM。
 * 对外只暴露静态方法，场景的加载/保存与 DocumentProxy 的页面索引绑定。
 */
class SvgSubsetAdaptor {
private:
    SvgSubsetAdaptor() { ; }
    virtual ~SvgSubsetAdaptor() { ; }

public:
    /** @brief 加载指定页为新的 GraphicsScene；页面文件缺失或损坏时返回 0。 */
    static GraphicsScene *loadScene(DocumentProxy *proxy, const int pageIndex);
    /** @brief 返回页面 SVG 的原始字节，供预览或导出使用，不做升级写回。 */
    static QByteArray loadSceneAsText(DocumentProxy *proxy, const int pageIndex);
    /** @brief 从内存中的 SVG 数据构建场景，用于导入与单元测试。 */
    static GraphicsScene *loadScene(DocumentProxy *proxy, const QByteArray &pArray);

    /**
     * @brief 把场景写回 pageIndex 对应的页面文件。
     * @note 仅当场景被标记为已修改时才写出；保存后清除修改标志。
     */
    static void persistScene(DocumentProxy *proxy, GraphicsScene *pScene, const int pageIndex);
    /** @brief 读取旧版本页面并就地升级到当前文件版本，缺失页面会被跳过。 */
    static void upgradeScene(DocumentProxy *proxy, const int pageIndex);

    /** @brief 读取页面 SVG 根节点上记录的页面 UUID。 */
    static QUuid sceneUuid(DocumentProxy *proxy, const int pageIndex);
    /** @brief 写入页面 UUID；pUuid 为空时生成新值。 */
    static void setSceneUuid(DocumentProxy *proxy, const int pageIndex, QUuid pUuid);

    /** @brief 把页面中引用的 PDF 背景转成位图，便于不支持 PDF 的渲染路径使用。 */
    static void convertPDFObjectsToImages(DocumentProxy *proxy);
    /** @brief 把引用的 SVG 图片栅格化，避免导出/播放时依赖外部 SVG。 */
    static void convertSvgImagesToImages(DocumentProxy *proxy);

    /// SVG 命名空间。
    static const QString nsSvg;
    /// xlink 命名空间，媒体与图片的 href 使用它。
    static const QString nsXLink;
    /// XHTML 命名空间，文本内容的 foreignObject 使用它。
    static const QString nsXHtml;
    /// SVG 布尔值 "true"。
    static const QString xmlTrue;
    /// SVG 布尔值 "false"。
    static const QString xmlFalse;

    /// 字号属性前缀（用于区分像素/点单位）。
    static const QString sFontSizePrefix;
    /// 像素单位后缀。
    static const QString sPixelUnit;
    /// 字重属性前缀。
    static const QString sFontWeightPrefix;
    /// 字体样式属性前缀。
    static const QString sFontStylePrefix;

private:
    static QDomDocument loadSceneDocument(DocumentProxy *proxy, const int pPageIndex);

    static QString toSvgTransform(const QTransform &matrix);
    static QTransform fromSvgTransform(const QString &transform);

    /**
     * @brief 增量式 SVG 读取器：用 QXmlStreamReader 顺序构建场景图元。
     *
     * 读取过程中记录目标文件版本，遇到低版本数据时就地补齐字段（例如缺省 fillRule、
     * 旧版 z 序、字号单位），因此 loadScene 与 upgradeScene 共用同一实现。
     */
    class SvgSubsetReader {
    public:
        /** @brief 绑定文档与 XML 字节；解析在 loadScene 中真正开始。 */
        SvgSubsetReader(DocumentProxy *proxy, const QByteArray &pXmlData);

        virtual ~SvgSubsetReader() {}

        /** @brief 创建并返回场景；解析失败时返回 0，已创建的图元随之释放。 */
        GraphicsScene *loadScene(DocumentProxy *proxy);

    private:
        GraphicsPolygonItem *polygonItemFromLineSvg(const QColor &pDefaultBrushColor);

        GraphicsPolygonItem *polygonItemFromPolygonSvg(const QColor &pDefaultBrushColor);

        QList<GraphicsPolygonItem *> polygonItemsFromPolylineSvg(const QColor &pDefaultColor);

        GraphicsPixmapItem *pixmapItemFromSvg();

        GraphicsSvgItem *svgItemFromSvg();

        GraphicsPDFItem *pdfItemFromPDF();

        GraphicsMediaItem *videoItemFromSvg();

        GraphicsMediaItem *audioItemFromSvg();

        GraphicsAppleWidgetItem *graphicsAppleWidgetFromSvg();

        GraphicsWebWidgetItem *graphicsWebWidgetFromSvg();

        GraphicsTextItem *textItemFromSvg();

        GraphicsCurtainItem *curtainItemFromSvg();

        GraphicsRuler *rulerFromSvg();

        GraphicsCompass *compassFromSvg();

        GraphicsProtractor *protractorFromSvg();

        GraphicsTriangle *triangleFromSvg();

        GraphicsCache *cacheFromSvg();

        void readGroupRoot();
        QGraphicsItem *readElementFromGroup();
        GraphicsGroupContainerItem *readGroup();

        void graphicsItemFromSvg(QGraphicsItem *gItem);

        QXmlStreamReader mXmlReader;
        int mFileVersion;
        DocumentProxy *mProxy;
        QString mDocumentPath;

        QColor mGroupDarkBackgroundColor;
        QColor mGroupLightBackgroundColor;
        qreal mGroupZIndex;
        bool mGroupHasInfo;

        bool saveSceneAfterLoading;

        QString mNamespaceUri;
        GraphicsScene *mScene;

        QHash<QString, GraphicsStrokesGroup *> mStrokesList;
    };

    /**
     * @brief SVG 写出器：遍历场景，把图元映射成 SVG 元素与自定义属性。
     *
     * 资源类图元（图片、PDF、音视频、控件）先复制/落盘到对象目录，再在 SVG 中写
     * 引用；笔迹按“是否闭合”分别写成 polyline 或 polygon。写出后 XML 由
     * writeSvgElement() 统一交给 QXmlStreamWriter。
     */
    class SvgSubsetWriter {
    public:
        /** @brief 记录场景、文档路径与页号，构造时不写文件。 */
        SvgSubsetWriter(DocumentProxy *proxy, GraphicsScene *pScene, const int pageIndex);

        /** @brief 把场景写到 pageIndex 对应的页面文件；成功返回 true。 */
        bool persistScene(DocumentProxy *proxy, int pageIndex);

        virtual ~SvgSubsetWriter() {}

    private:
        void persistGroupToDom(QGraphicsItem *groupItem, QDomElement *curParent,
                               QDomDocument *curDomDocument);
        void persistStrokeToDom(QGraphicsItem *strokeItem, QDomElement *curParent,
                                QDomDocument *curDomDocument);
        void polygonItemToSvgPolygon(GraphicsPolygonItem *polygonItem, bool groupHoldsInfo);
        void polygonItemToSvgLine(GraphicsPolygonItem *polygonItem, bool groupHoldsInfo);
        void strokeToSvgPolyline(GraphicsStroke *stroke, bool groupHoldsInfo);
        void strokeToSvgPolygon(GraphicsStroke *stroke, bool groupHoldsInfo);

        inline QString pointsToSvgPointsAttribute(QVector<QPointF> points) {
            GeometryUtils::crashPointList(points);

            int pointsCount = points.size();
            QString svgPoints;

            int length = 0;
            QString sBuf;

            for (int j = 0; j < pointsCount; j++) {
                sBuf = "%1,%2 ";
                const QPointF &point = points.at(j);

                QString temp1 = "%1", temp2 = "%2";

                temp1 = temp1.arg(point.x());
                temp2 = temp2.arg(point.y());

                QLocale loc(QLocale::C);
                sBuf = sBuf.arg(loc.toFloat(temp1)).arg(loc.toFloat(temp2));

                svgPoints.insert(length, sBuf);
                length += sBuf.length();
            }
            return svgPoints;
        }

        inline qreal trickAlpha(qreal alpha) {
            qreal trickAlpha = alpha;
            if (trickAlpha >= 0.2 && trickAlpha < 0.6) {
                trickAlpha /= 5;
            } else if (trickAlpha < 0.8)
                trickAlpha /= 3;

            return trickAlpha;
        }

        void pixmapItemToLinkedImage(GraphicsPixmapItem *pixmapItem);
        void svgItemToLinkedSvg(GraphicsSvgItem *svgItem);
        void pdfItemToLinkedPDF(GraphicsPDFItem *pdfItem);
        void videoItemToLinkedVideo(GraphicsVideoItem *videoItem);
        void audioItemToLinkedAudio(GraphicsAudioItem *audioItem);
        void graphicsItemToSvg(QGraphicsItem *item);
        void graphicsAppleWidgetToSvg(GraphicsAppleWidgetItem *item);
        void graphicsWebWidgetToSvg(GraphicsWebWidgetItem *item);
        void graphicsWidgetToSvg(GraphicsWidgetItem *item);
        void textItemToSvg(GraphicsTextItem *item);
        void curtainItemToSvg(GraphicsCurtainItem *item);
        void rulerToSvg(GraphicsRuler *item);
        void compassToSvg(GraphicsCompass *item);
        void protractorToSvg(GraphicsProtractor *item);
        void cacheToSvg(GraphicsCache *item);
        void triangleToSvg(GraphicsTriangle *item);
        void writeSvgElement(DocumentProxy *proxy);

    private:
        GraphicsScene *mScene;
        QXmlStreamWriter mXmlWriter;
        QString mDocumentPath;
        int mPageIndex;
    };
};
