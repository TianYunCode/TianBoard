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

class SvgSubsetAdaptor {
private:
    SvgSubsetAdaptor() { ; }
    virtual ~SvgSubsetAdaptor() { ; }

public:
    static GraphicsScene *loadScene(DocumentProxy *proxy, const int pageIndex);
    static QByteArray loadSceneAsText(DocumentProxy *proxy, const int pageIndex);
    static GraphicsScene *loadScene(DocumentProxy *proxy, const QByteArray &pArray);

    static void persistScene(DocumentProxy *proxy, GraphicsScene *pScene, const int pageIndex);
    static void upgradeScene(DocumentProxy *proxy, const int pageIndex);

    static QUuid sceneUuid(DocumentProxy *proxy, const int pageIndex);
    static void setSceneUuid(DocumentProxy *proxy, const int pageIndex, QUuid pUuid);

    static void convertPDFObjectsToImages(DocumentProxy *proxy);
    static void convertSvgImagesToImages(DocumentProxy *proxy);

    static const QString nsSvg;
    static const QString nsXLink;
    static const QString nsXHtml;
    static const QString xmlTrue;
    static const QString xmlFalse;

    static const QString sFontSizePrefix;
    static const QString sPixelUnit;
    static const QString sFontWeightPrefix;
    static const QString sFontStylePrefix;

private:
    static QDomDocument loadSceneDocument(DocumentProxy *proxy, const int pPageIndex);

    static QString toSvgTransform(const QTransform &matrix);
    static QTransform fromSvgTransform(const QString &transform);

    class SvgSubsetReader {
    public:
        SvgSubsetReader(DocumentProxy *proxy, const QByteArray &pXmlData);

        virtual ~SvgSubsetReader() {}

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

    class SvgSubsetWriter {
    public:
        SvgSubsetWriter(DocumentProxy *proxy, GraphicsScene *pScene, const int pageIndex);

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
