#include "content/SvgSubsetAdaptor.h"
#include <algorithm>
#include <QElapsedTimer>
#include "core/VirtualDesktop.h"

#include <QtCore>
#include <QtXml>
#include <QGraphicsTextItem>
#include <QDomElement>
#include <QGraphicsVideoItem>

#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsProxyWidget.h"
#include "whiteboard/GraphicsPolygonItem.h"
#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsWidgetItem.h"
#include "whiteboard/GraphicsPDFItem.h"
#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsTextItemDelegate.h"
#include "whiteboard/GraphicsStroke.h"
#include "whiteboard/GraphicsStrokesGroup.h"
#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsGroupContainerItemDelegate.h"
#include "whiteboard/Item.h"

#include "whiteboard/GraphicsRuler.h"
#include "whiteboard/GraphicsCompass.h"
#include "whiteboard/GraphicsProtractor.h"
#include "whiteboard/GraphicsCurtainItem.h"
#include "whiteboard/GraphicsTriangle.h"
#include "whiteboard/GraphicsCache.h"

#include "model/DocumentProxy.h"

#include "whiteboard/BoardView.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"
#include "whiteboard/BoardPaletteManager.h"

#include "utils/FileSystemUtils.h"
#include "utils/StringUtils.h"
#include "utils/FileSystemUtils.h"

#include "core/Settings.h"
#include "core/Setting.h"
#include "core/PersistenceManager.h"
#include "core/Application.h"
#include "core/TextTools.h"

#include "content/PDFRenderer.h"


const QString SvgSubsetAdaptor::nsSvg = "http://www.w3.org/2000/svg";
const QString SvgSubsetAdaptor::nsXHtml = "http://www.w3.org/1999/xhtml";
const QString SvgSubsetAdaptor::nsXLink = "http://www.w3.org/1999/xlink";
const QString SvgSubsetAdaptor::xmlTrue = "true";
const QString SvgSubsetAdaptor::xmlFalse = "false";
const QString SvgSubsetAdaptor::sFontSizePrefix = "font-size:";
const QString SvgSubsetAdaptor::sPixelUnit = "px";
const QString SvgSubsetAdaptor::sFontWeightPrefix = "font-weight:";
const QString SvgSubsetAdaptor::sFontStylePrefix = "font-style:";

const QString tElement = "element";
const QString tGroup = "group";
const QString tStrokeGroup = "strokeGroup";
const QString tGroups = "groups";
const QString aId = "id";

QString SvgSubsetAdaptor::toSvgTransform(const QTransform& matrix)
{
    return QString("matrix(%1, %2, %3, %4, %5, %6)")
            .arg(matrix.m11(), 0 , 'g')
            .arg(matrix.m12(), 0 , 'g')
            .arg(matrix.m21(), 0 , 'g')
            .arg(matrix.m22(), 0 , 'g')
            .arg(matrix.dx(), 0 , 'g')
            .arg(matrix.dy(), 0 , 'g');
}


QTransform SvgSubsetAdaptor::fromSvgTransform(const QString& transform)
{
    QTransform matrix;
    QString ts = transform;
    ts.replace("matrix(", "");
    ts.replace(")", "");
    QStringList sl = ts.split(",");

    if (sl.size() >= 6)
    {
        matrix.setMatrix(
                    sl.at(0).toFloat(),
                    sl.at(1).toFloat(),
                    0,
                    sl.at(2).toFloat(),
                    sl.at(3).toFloat(),
                    0,
                    sl.at(4).toFloat(),
                    sl.at(5).toFloat(),
                    1);
    }

    return matrix;
}



static bool itemZIndexComp(const QGraphicsItem* item1,
                           const QGraphicsItem* item2)
{
    return item1->data(GraphicsItemData::ItemOwnZValue).toReal() < item2->data(GraphicsItemData::ItemOwnZValue).toReal();
}


void SvgSubsetAdaptor::upgradeScene(DocumentProxy* proxy, const int pageIndex)
{
    //4.2
    QDomDocument doc = loadSceneDocument(proxy, pageIndex);
    QDomElement elSvg = doc.documentElement(); // SVG tag
    QString formatVersion = elSvg.attributeNS(Settings::documentNamespaceUri, "version", "4.1"); // default to 4.1

    if (formatVersion.startsWith("4.1") || formatVersion.startsWith("4.2") || formatVersion.startsWith("4.3"))
    {
        // migrate to 4.2.1 (or latter)
        GraphicsScene *scene = loadScene(proxy, pageIndex);
        scene->setModified(true);
        persistScene(proxy, scene, pageIndex);
    }
}


QDomDocument SvgSubsetAdaptor::loadSceneDocument(DocumentProxy* proxy, const int pPageIndex)
{
    QString fileName = proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg",pPageIndex);

    QFile file(fileName);
    QDomDocument doc("page");

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Cannot open file " << fileName << " for reading ...";
            return doc;
        }

        doc.setContent(&file, true);
        file.close();
    }

    return doc;
}


void SvgSubsetAdaptor::setSceneUuid(DocumentProxy* proxy, const int pageIndex, QUuid pUuid)
{
    QString fileName = proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg",pageIndex);

    QFile file(fileName);

    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return;

    QTextStream textReadStream(&file);
    QString xmlContent = textReadStream.readAll();
    int uuidIndex = xmlContent.indexOf("uuid");
    if (-1 == uuidIndex)
    {
        qWarning() << "Cannot read UUID from file" << fileName << "to set new UUID";
        file.close();
        return;
    }
    int quoteStartIndex = xmlContent.indexOf('"', uuidIndex);
    if (-1 == quoteStartIndex)
    {
        qWarning() << "Cannot read UUID from file" << fileName << "to set new UUID";
        file.close();
        return;
    }
    int quoteEndIndex = xmlContent.indexOf('"', quoteStartIndex + 1);
    if (-1 == quoteEndIndex)
    {
        qWarning() << "Cannot read UUID from file" << fileName << "to set new UUID";
        file.close();
        return;
    }

    file.close();

    QString newXmlContent = xmlContent.left(quoteStartIndex + 1);
    newXmlContent.append(StringUtils::toCanonicalUuid(pUuid));
    newXmlContent.append(xmlContent.right(xmlContent.length() - quoteEndIndex));

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QTextStream textWriteStream(&file);
        textWriteStream << newXmlContent;
        file.close();
    }
    else
    {
        qWarning() << "Cannot open file" << fileName  << "to write UUID";
    }
}

GraphicsScene* SvgSubsetAdaptor::loadScene(DocumentProxy* proxy, const int pageIndex)
{
    QString fileName = proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", pageIndex);
    qDebug() << fileName;
    QFile file(fileName);

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Cannot open file " << fileName << " for reading ...";
            return 0;
        }

        GraphicsScene* scene = loadScene(proxy, file.readAll());

        file.close();

        return scene;
    }

    return 0;
}


QByteArray SvgSubsetAdaptor::loadSceneAsText(DocumentProxy* proxy, const int pageIndex)
{
    QString fileName = proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", pageIndex);
    qDebug() << fileName;
    QFile file(fileName);

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Cannot open file " << fileName << " for reading ...";
            return "";
        }

        return file.readAll();

        file.close();

    }
    return "";
}


QUuid SvgSubsetAdaptor::sceneUuid(DocumentProxy* proxy, const int pageIndex)
{
    QString fileName = proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", pageIndex);

    QFile file(fileName);

    QUuid uuid;

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Cannot open file " << fileName << " for reading ...";
            return QUuid();
        }

        QXmlStreamReader xml(file.readAll());

        bool foundSvg = false;

        while (!xml.atEnd() && !foundSvg)
        {
            xml.readNext();

            if (xml.isStartElement())
            {
                if (xml.name() == "svg")
                {
                    QStringView svgSceneUuid = xml.attributes().value(Settings::documentNamespaceUri, "uuid");

                    if (!svgSceneUuid.isNull())
                        uuid = QUuid(svgSceneUuid.toString());

                    foundSvg = true;
                }
            }
        }

        file.close();
    }

    return uuid;
}


GraphicsScene* SvgSubsetAdaptor::loadScene(DocumentProxy* proxy, const QByteArray& pArray)
{
    SvgSubsetReader reader(proxy, TextTools::cleanHtmlCData(QString(pArray)).toUtf8());
    return reader.loadScene(proxy);
}

SvgSubsetAdaptor::SvgSubsetReader::SvgSubsetReader(DocumentProxy* pProxy, const QByteArray& pXmlData)
    : mXmlReader(pXmlData)
    , mProxy(pProxy)
    , mDocumentPath(pProxy->persistencePath())
    , mGroupHasInfo(false)
{
    // NOOP
}


GraphicsScene* SvgSubsetAdaptor::SvgSubsetReader::loadScene(DocumentProxy* proxy)
{
    qDebug() << "loadScene() : starting reading...";
    QElapsedTimer time;
    time.start();
    mScene = 0;
    GraphicsWidgetItem *currentWidget = 0;
    bool pageDpiSpecified = true;
    saveSceneAfterLoading = false;

    mFileVersion = 40100; // default to 4.1.0

    GraphicsStrokesGroup* strokesGroup = 0;
    GraphicsStroke* currentStroke = 0;

    while (!mXmlReader.atEnd())
    {
        mXmlReader.readNext();
        if (mXmlReader.isStartElement())
        {
            if (mXmlReader.name() == "svg")
            {
                if (!mScene)
                {
                    mScene = new GraphicsScene(mProxy, false);
                    mScene->setModified(false);
                }

                QStringView svgUbVersion = mXmlReader.attributes().value(Settings::documentNamespaceUri, "version");

                if (!svgUbVersion.isNull())
                {
                    QString formatVersion = svgUbVersion.toString();

                    //may look like : 4 or 4.1 or 4.2 or 4.2.1, etc

                    QStringList parts = formatVersion.split(".");

                    if (parts.length() > 0)
                    {
                        mFileVersion = parts.at(0).toInt() * 10000;
                    }

                    if (parts.length() > 1)
                    {
                        mFileVersion += parts.at(1).toInt() * 100;
                    }

                    if (parts.length() > 2)
                    {
                        mFileVersion += parts.at(2).toInt();
                    }
                }

                mNamespaceUri = Settings::documentNamespaceUri;

                QStringView svgSceneUuid = mXmlReader.attributes().value(mNamespaceUri, "uuid");

                if (!svgSceneUuid.isNull())
                    mScene->setUuid(QUuid(svgSceneUuid.toString()));

                QStringView svgViewBox = mXmlReader.attributes().value("viewBox");


                if (!svgViewBox.isNull())
                {
                    QStringList ts = svgViewBox.toString().split(QLatin1Char(' '), Qt::SkipEmptyParts);

                    QRectF sceneRect;
                    if (ts.size() >= 4)
                    {
                        sceneRect.setX(ts.at(0).toFloat());
                        sceneRect.setY(ts.at(1).toFloat());
                        sceneRect.setWidth(ts.at(2).toFloat());
                        sceneRect.setHeight(ts.at(3).toFloat());

                        mScene->setSceneRect(sceneRect);
                    }
                    else
                    {
                        qWarning() << "cannot make sense of 'viewBox' value " << svgViewBox.toString();
                    }
                }

                QStringView pageDpi = mXmlReader.attributes().value("pageDpi");

                if (!pageDpi.isNull())
                    proxy->setPageDpi(pageDpi.toInt());

                else if (proxy->pageDpi() == 0) {
                    proxy->setPageDpi(VirtualDesktop::instance()->physicalDpi());
                    pageDpiSpecified = false;
                }

                bool darkBackground = false;
                bool crossedBackground = false;
                bool ruledBackground = false;

                QStringView darkBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "dark-background");

                if (!darkBackgroundValue.isNull())
                    darkBackground = (darkBackgroundValue.toString() == xmlTrue);

                QStringView crossedBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "crossed-background");

                if (!crossedBackgroundValue.isNull())
                    crossedBackground = (crossedBackgroundValue.toString() == xmlTrue);


                if (crossedBackground) {
                    QStringView gridSizeValue = mXmlReader.attributes().value(mNamespaceUri, "grid-size");

                    if (!gridSizeValue.isNull()) {
                        int gridSize = gridSizeValue.toInt();

                        mScene->setBackgroundGridSize(gridSize);
                    }
                }

                QStringView ruledBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "ruled-background");

                if (!ruledBackgroundValue.isNull())
                    ruledBackground = (ruledBackgroundValue.toString() == xmlTrue);

                if (ruledBackground && !crossedBackground) { // if for some reason both are true, the background will be a grid
                    QStringView gridSizeValue = mXmlReader.attributes().value(mNamespaceUri, "grid-size");

                    if (!gridSizeValue.isNull()) {
                        int gridSize = gridSizeValue.toInt();

                        mScene->setBackgroundGridSize(gridSize);
                    }
                }

                PageBackground bg;
                if (crossedBackground)
                    bg = PageBackground::crossed;
                else if (ruledBackground)
                    bg = PageBackground::ruled;
                else
                    bg = PageBackground::plain;

                mScene->setBackground(darkBackground, bg);

                QStringView pageNominalSize = mXmlReader.attributes().value(mNamespaceUri, "nominal-size");
                if (!pageNominalSize.isNull())
                {
                    QStringList ts = pageNominalSize.toString().split(QLatin1Char('x'), Qt::SkipEmptyParts);

                    QSize sceneSize;
                    if (ts.size() >= 2)
                    {
                        sceneSize.setWidth(ts.at(0).toInt());
                        sceneSize.setHeight(ts.at(1).toInt());

                        mScene->setNominalSize(sceneSize);
                    }
                    else
                    {
                        qWarning() << "cannot make sense of 'nominal-size' value " << pageNominalSize.toString();
                    }

                }
            }
            else if (mXmlReader.name() == "g")
            {
                strokesGroup = new GraphicsStrokesGroup();
                graphicsItemFromSvg(strokesGroup);

                QStringView zValueAttr = mXmlReader.attributes().value(mNamespaceUri, "z-value");

                if (!zValueAttr.isNull())
                {
                    mGroupZIndex = zValueAttr.toString().toFloat();
                    mGroupHasInfo = true;
                }

                QStringView fillOnDarkBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "fill-on-dark-background");

                if (!fillOnDarkBackgroundValue.isNull())
                {
                    mGroupDarkBackgroundColor.setNamedColor(fillOnDarkBackgroundValue.toString());
                }

                QStringView fillOnLightBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "fill-on-light-background");

                if (!fillOnLightBackgroundValue.isNull())
                {
                    mGroupLightBackgroundColor.setNamedColor(fillOnLightBackgroundValue.toString());
                }

                QStringView uuidValue = mXmlReader.attributes().value(mNamespaceUri, "uuid");

                if (!uuidValue.isNull())
                    strokesGroup->setUuid(QUuid::fromString(uuidValue.toString()));
                else
                    strokesGroup->setUuid(QUuid::createUuid());

                QString uuid_stripped = strokesGroup->uuid().toString().replace("}","").replace("{","");

                if (!mStrokesList.contains(uuid_stripped))
                    mStrokesList.insert(uuid_stripped, strokesGroup);
            }
            else if (mXmlReader.name() == "polygon" || mXmlReader.name() == "line")
            {
                GraphicsPolygonItem* polygonItem = 0;

                QString parentId = mXmlReader.attributes().value(mNamespaceUri, "parent").toString();

                if (mXmlReader.name() == "polygon")
                    polygonItem = polygonItemFromPolygonSvg(mScene->isDarkBackground() ? Qt::white : Qt::black);
                else if (mXmlReader.name() == "line")
                    polygonItem = polygonItemFromLineSvg(mScene->isDarkBackground() ? Qt::white : Qt::black);

                if(parentId.isEmpty() && strokesGroup)
                    parentId = strokesGroup->uuid().toString();

                if(parentId.isEmpty())
                    parentId = QUuid::createUuid().toString();

                if (polygonItem)
                {
                    polygonItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Graphic));

                    GraphicsStrokesGroup* group;
                    if(!mStrokesList.contains(parentId)){
                        group = new GraphicsStrokesGroup();
                        mStrokesList.insert(parentId,group);
                        group->setTransform(polygonItem->transform());
                        GraphicsItem::assignZValue(group, polygonItem->zValue());
                    }
                    else
                        group = mStrokesList.value(parentId);

                    if (!currentStroke)
                        currentStroke = new GraphicsStroke();

                    if(polygonItem->transform().isIdentity())
                        polygonItem->setTransform(group->transform());

                    group->addToGroup(polygonItem);
                    polygonItem->setStrokesGroup(group);
                    polygonItem->setStroke(currentStroke);

                    polygonItem->show();
                    group->addToGroup(polygonItem);
                }
            }
            else if (mXmlReader.name() == "polyline")
            {
                QList<GraphicsPolygonItem*> polygonItems = polygonItemsFromPolylineSvg(mScene->isDarkBackground() ? Qt::white : Qt::black);

                QString parentId = mXmlReader.attributes().value(mNamespaceUri, "parent").toString();

                if(parentId.isEmpty() && strokesGroup)
                    parentId = strokesGroup->uuid().toString();

                if(parentId.isEmpty())
                    parentId = QUuid::createUuid().toString();

                foreach(GraphicsPolygonItem* polygonItem, polygonItems)
                {
                    polygonItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Graphic));

                    GraphicsStrokesGroup* group;

                    if(!mStrokesList.contains(parentId)){
                        group = new GraphicsStrokesGroup();
                        mStrokesList.insert(parentId,group);
                        group->setTransform(polygonItem->transform());
                        GraphicsItem::assignZValue(group, polygonItem->zValue());
                    }
                    else
                        group = mStrokesList.value(parentId);

                    if (!currentStroke)
                        currentStroke = new GraphicsStroke();

                    if(polygonItem->transform().isIdentity())
                        polygonItem->setTransform(group->transform());

                    group->addToGroup(polygonItem);
                    polygonItem->setStrokesGroup(group);
                    polygonItem->setStroke(currentStroke);

                    polygonItem->show();
                    group->addToGroup(polygonItem);
                }

            }
            else if (mXmlReader.name() == "image")
            {
                QStringView imageHref = mXmlReader.attributes().value(nsXLink, "href");

                if (!imageHref.isNull())
                {
                    QString href = imageHref.toString();

                    QStringView backgroundValue = mXmlReader.attributes().value(mNamespaceUri, "background");

                    bool isBackground = (!backgroundValue.isNull() && backgroundValue.toString() == xmlTrue);

                    if (href.contains("png"))
                    {

                        GraphicsPixmapItem* pixmapItem = pixmapItemFromSvg();
                        if (pixmapItem)
                        {
                            pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                            pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                            mScene->addItem(pixmapItem);

                            if (isBackground)
                                mScene->setAsBackgroundObject(pixmapItem);

                            pixmapItem->show();
                        }
                    }
                    else if (href.contains("svg"))
                    {
                        GraphicsSvgItem* svgItem = svgItemFromSvg();
                        if (svgItem)
                        {
                            svgItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                            svgItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                            mScene->addItem(svgItem);

                            if (isBackground)
                                mScene->setAsBackgroundObject(svgItem);

                            svgItem->show();
                        }
                    }
                    else
                    {
                        qWarning() << "don't know what to do with href value " << href;
                    }
                }
            }
            else if (mXmlReader.name() == "audio")
            {
                GraphicsMediaItem* audioItem = audioItemFromSvg();
                if (audioItem)
                {
                    audioItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                    audioItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                    mScene->addItem(audioItem);

                    audioItem->show();

                    audioItem->play();
                    audioItem->pause();
                }
            }
            else if (mXmlReader.name() == "video")
            {
                GraphicsMediaItem* videoItem = videoItemFromSvg();
                if (videoItem)
                {

                    videoItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                    videoItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                    mScene->addItem(videoItem);

                    videoItem->show();
                }
            }
            else if (mXmlReader.name() == "text")//This is for backward compatibility with proto text field prior to version 4.3
            {
                GraphicsTextItem* textItem = textItemFromSvg();
                if (textItem)
                {
                    textItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                    textItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                    mScene->addItem(textItem);

                    textItem->show();
                }
            }
            else if (mXmlReader.name() == "curtain")
            {
                GraphicsCurtainItem* mask = curtainItemFromSvg();
                if (mask)
                {
                    mScene->addItem(mask);
                    mScene->registerTool(mask);
                }
            }
            else if (mXmlReader.name() == "ruler")
            {

                GraphicsRuler *ruler = rulerFromSvg();
                if (ruler)
                {
                    mScene->addItem(ruler);
                    mScene->registerTool(ruler);
                }

            }
            else if (mXmlReader.name() == "compass")
            {
                GraphicsCompass *compass = compassFromSvg();
                if (compass)
                {
                    mScene->addItem(compass);
                    mScene->registerTool(compass);
                }
            }
            else if (mXmlReader.name() == "protractor")
            {
                GraphicsProtractor *protractor = protractorFromSvg();
                if (protractor)
                {
                    mScene->addItem(protractor);
                    mScene->registerTool(protractor);
                }
            }
            else if (mXmlReader.name() == "triangle")
            {
                GraphicsTriangle *triangle = triangleFromSvg();
                if (triangle)
                {
                    mScene->addItem(triangle);
                    mScene->registerTool(triangle);
                }
            }
            else if (mXmlReader.name() == "cache")
            {
                GraphicsCache* cache = cacheFromSvg();
                if(cache)
                {
                    mScene->addItem(cache);
                    mScene->registerTool(cache);
                    Application::boardController->notifyCache(true);
                }
            }
            else if (mXmlReader.name() == "foreignObject")
            {
                QString href = mXmlReader.attributes().value(nsXLink, "href").toString();
                QString src = mXmlReader.attributes().value(mNamespaceUri, "src").toString();
                QString type = mXmlReader.attributes().value(mNamespaceUri, "type").toString();
                bool isBackground = mXmlReader.attributes().value(mNamespaceUri, "background").toString() == xmlTrue;

                qreal foreignObjectWidth = mXmlReader.attributes().value("width").toString().toFloat();
                qreal foreignObjectHeight = mXmlReader.attributes().value("height").toString().toFloat();

                if (href.contains(".pdf"))
                {
                    GraphicsPDFItem* pdfItem = pdfItemFromPDF();
                    if (pdfItem)
                    {
                        VirtualDesktop* desktop = VirtualDesktop::instance();
                        qreal currentDpi = desktop->physicalDpi();
                        qreal pdfScale = qreal(proxy->pageDpi())/currentDpi;
                        // qDebug() << "pdfScale " << pdfScale;

                        // If the PDF is in the background, it occupies the whole page; so we can simply
                        // use that information to calculate its scale.
                        if (isBackground) {
                            qreal pageWidth = mScene->nominalSize().width();
                            qreal pageHeight = mScene->nominalSize().height();

                            qreal scaleX = pageWidth / pdfItem->sceneBoundingRect().width();
                            qreal scaleY = pageHeight / pdfItem->sceneBoundingRect().height();

                            pdfScale = (scaleX+scaleY)/2.;
                        }

                        pdfItem->setScale(pdfScale);
                        pdfItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                        pdfItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                        mScene->addItem(pdfItem);

                        if (isBackground)
                            mScene->setAsBackgroundObject(pdfItem);

                        pdfItem->show();

                        currentWidget = 0;
                    }
                }
                else if (src.contains(".wdgt"))
                {
                    GraphicsAppleWidgetItem* appleWidgetItem = graphicsAppleWidgetFromSvg();
                    if (appleWidgetItem)
                    {
                        appleWidgetItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                        appleWidgetItem->resize(foreignObjectWidth, foreignObjectHeight);

                        mScene->addItem(appleWidgetItem);

                        appleWidgetItem->show();

                        currentWidget = appleWidgetItem;
                    }
                }
                else if (src.contains(".wgt"))
                {
                    GraphicsWebWidgetItem* webWidgetItem = graphicsWebWidgetFromSvg();
                    if (webWidgetItem)
                    {
                        webWidgetItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                        webWidgetItem->resize(foreignObjectWidth, foreignObjectHeight);

                        mScene->addItem(webWidgetItem);

                        webWidgetItem->show();

                        currentWidget = webWidgetItem;
                    }
                }
                else if (type == "text")
                {
                    GraphicsTextItem* textItem = textItemFromSvg();
                    GraphicsTextItemDelegate *textDelegate = 0;

                    if (textItem)
                        textDelegate = dynamic_cast<GraphicsTextItemDelegate*>(textItem->Delegate());

                    if (textDelegate)
                    {
                        VirtualDesktop* desktop = VirtualDesktop::instance();
                        qreal currentDpi = desktop->physicalDpi();
                        qreal textSizeMultiplier = qreal(proxy->pageDpi())/currentDpi;
                        //textDelegate->scaleTextSize(textSizeMultiplier);
                    }

                    if (textItem)
                    {
                        textItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                        textItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
                        textItem->activateTextEditor(false);

                        mScene->addItem(textItem);

                        textItem->show();
                    }
                }
                else
                {
                    qWarning() << "Ignoring unknown foreignObject:" << href;
                }
            }
            else if (currentWidget && (mXmlReader.name() == "preference"))
            {
                QString key = mXmlReader.attributes().value("key").toString();
                QString value = mXmlReader.attributes().value("value").toString();

                currentWidget->setPreference(key, value);
            }
            else if (currentWidget && (mXmlReader.name() == "datastoreEntry"))
            {
                QString key = mXmlReader.attributes().value("key").toString();
                QString value = mXmlReader.attributes().value("value").toString();

                currentWidget->setDatastoreEntry(key, value);
            } else if (mXmlReader.name() == tGroups) {
                //considering groups section at the end of the document
                readGroupRoot();
            }
            else
            {
                // NOOP
            }
        }
        else if (mXmlReader.isEndElement())
        {
            if (mXmlReader.name() == "g")
            {
                mGroupHasInfo = false;
                mGroupDarkBackgroundColor = QColor();
                mGroupLightBackgroundColor = QColor();
                strokesGroup = NULL;
                currentStroke = NULL;
            }
        }
    }


    if (mXmlReader.hasError())
    {
        qWarning() << "error parsing file " << mXmlReader.errorString();
    }

    qDebug() << "Number of detected strokes: " << mStrokesList.count();
    QHashIterator<QString, GraphicsStrokesGroup*> iterator(mStrokesList);
    while (iterator.hasNext()) {
        iterator.next();
        mScene->addItem(iterator.value());
    }

    if (mScene) {
        mScene->setModified(saveSceneAfterLoading);
        mScene->enableUndoRedoStack();
    }

    qDebug() << "loadScene() : created scene and read file";
    qDebug() << "spent milliseconds: " << time.elapsed();
    return mScene;
}


GraphicsGroupContainerItem* SvgSubsetAdaptor::SvgSubsetReader::readGroup()
{
    GraphicsGroupContainerItem *group = new GraphicsGroupContainerItem();
    QList<QGraphicsItem *> groupContainer;

    QString id = mXmlReader.attributes().value(aId).toString();
    id = id.mid(1,id.length()-2);
    bool shouldSkipSubElements = false;
    if(mStrokesList.contains(id))
        shouldSkipSubElements = true;

    QString lockedValue = mXmlReader.attributes().value(mNamespaceUri, "locked").toString();
    if (!lockedValue.isEmpty())
    {
        bool isLocked = lockedValue.contains(xmlTrue);
        group->setData(GraphicsItemData::ItemLocked, QVariant(isLocked));
    }

    QStringView layerValue = mXmlReader.attributes().value(mNamespaceUri, "layer");
    if (!layerValue.isNull())
    {
        bool ok;
        int layerAsInt = layerValue.toString().toInt(&ok);

        if (ok)
            group->setData(GraphicsItemData::ItemLayerType, QVariant(layerAsInt));
    }


    mXmlReader.readNext();
    while (!mXmlReader.atEnd())
    {
        if (mXmlReader.isEndElement()) {
            mXmlReader.readNext();
            break;
        }
        else if (mXmlReader.isStartElement()) {
            if (mXmlReader.name() == tGroup) {
                GraphicsGroupContainerItem *curGroup = readGroup();
                if (curGroup)
                    groupContainer.append(curGroup);
            }
            else if (mXmlReader.name() == tElement && !shouldSkipSubElements) {
                QString id = mXmlReader.attributes().value(aId).toString();
                QGraphicsItem *curItem = readElementFromGroup();

                // Explanation: the second condition discriminate the old storage version that should
                // not be interpreted anymore
                if(curItem && id.count("{") < 2)
                    groupContainer.append(curItem);
            }
            else {
                mXmlReader.skipCurrentElement();
            }
        }
        else {
            mXmlReader.readNext();
        }
    }

    foreach(QGraphicsItem* item, groupContainer)
        group->addToGroup(item);

    if (group->childItems().count())
    {
//        mScene->addItem(group);

        if (1 == group->childItems().count())
        {
            group->destroy(false);
        }
    }
    return group;
}

void SvgSubsetAdaptor::SvgSubsetReader::readGroupRoot()
{
    mXmlReader.readNext();
    while (!mXmlReader.atEnd()) {
        if (mXmlReader.isEndElement()) {
            mXmlReader.readNext();
            break;
        }
        else if (mXmlReader.isStartElement()) {
            if (mXmlReader.name() == tGroup) {

                GraphicsGroupContainerItem *curGroup = readGroup();

                if (curGroup) {
                    mScene->addGroup(curGroup);
                }
            }
            else {
                mXmlReader.skipCurrentElement();
            }
        }
        else {
            mXmlReader.readNext();
        }
    }
}

QGraphicsItem *SvgSubsetAdaptor::SvgSubsetReader::readElementFromGroup()
{
    QGraphicsItem *result = 0;
    QString id = mXmlReader.attributes().value(aId).toString();
    QString uuid = id.right(QUuid().toString().size());
    result = mScene->itemForUuid(QUuid(uuid));

    if(!result)
        result = mStrokesList.take(uuid.replace("}","").replace("{",""));

    mXmlReader.skipCurrentElement();
    mXmlReader.readNext();

    return result;
}

void SvgSubsetAdaptor::persistScene(DocumentProxy* proxy, GraphicsScene* pScene, const int pageIndex)
{
    SvgSubsetWriter writer(proxy, pScene, pageIndex);
    writer.persistScene(proxy, pageIndex);
}


SvgSubsetAdaptor::SvgSubsetWriter::SvgSubsetWriter(DocumentProxy* proxy, GraphicsScene* pScene, const int pageIndex)
    : mScene(pScene)
    , mDocumentPath(proxy->persistencePath())
    , mPageIndex(pageIndex)

{
    // NOOP
}


void SvgSubsetAdaptor::SvgSubsetWriter::writeSvgElement(DocumentProxy* proxy)
{
    mXmlWriter.writeStartElement("svg");

    mXmlWriter.writeAttribute("version", "1.1");
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "version", Settings::currentFileVersion);
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "uuid", StringUtils::toCanonicalUuid(mScene->uuid()));

    int margin = Settings::settings()->svgViewBoxMargin->get().toInt();
    QRect normalized = mScene->normalizedSceneRect().toRect();
    normalized.translate(margin * -1, margin * -1);
    normalized.setWidth(normalized.width() + (margin * 2));
    normalized.setHeight(normalized.height() + (margin * 2));
    mXmlWriter.writeAttribute("viewBox", QString("%1 %2 %3 %4").arg(normalized.x()).arg(normalized.y()).arg(normalized.width()).arg(normalized.height()));

    QSize pageNominalSize = mScene->nominalSize();
    if (pageNominalSize.isValid())
    {
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "nominal-size", QString("%1x%2").arg(pageNominalSize.width()).arg(pageNominalSize.height()));
    }

    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "dark-background", mScene->isDarkBackground() ? xmlTrue : xmlFalse);

    bool crossedBackground = mScene->pageBackground() == PageBackground::crossed;
    bool ruledBackground = mScene->pageBackground() == PageBackground::ruled;

    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "crossed-background", crossedBackground ? xmlTrue : xmlFalse);
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "ruled-background", ruledBackground ? xmlTrue : xmlFalse);

    if (crossedBackground || ruledBackground) {
        int gridSize = mScene->backgroundGridSize();

        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "grid-size", QString::number(gridSize));
    }

    VirtualDesktop* desktop = VirtualDesktop::instance();

    if (proxy->pageDpi() == 0)
        proxy->setPageDpi(desktop->physicalDpi());

    mXmlWriter.writeAttribute("pageDpi", QString::number(proxy->pageDpi()));


    mXmlWriter.writeStartElement("rect");
    mXmlWriter.writeAttribute("fill", mScene->isDarkBackground() ? "black" : "white");
    mXmlWriter.writeAttribute("x", QString::number(normalized.x()));
    mXmlWriter.writeAttribute("y", QString::number(normalized.y()));
    mXmlWriter.writeAttribute("width", QString::number(normalized.width()));
    mXmlWriter.writeAttribute("height", QString::number(normalized.height()));

    mXmlWriter.writeEndElement();
}

bool SvgSubsetAdaptor::SvgSubsetWriter::persistScene(DocumentProxy* proxy, int pageIndex)
{
    Q_UNUSED(pageIndex);

    //Creating dom structure to store information
    QDomDocument groupDomDocument;
    QDomElement groupRoot = groupDomDocument.createElement(tGroups);
    groupDomDocument.appendChild(groupRoot);

    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    mXmlWriter.setDevice(&buffer);

    mXmlWriter.setAutoFormatting(true);

    mXmlWriter.writeStartDocument();
    mXmlWriter.writeDefaultNamespace(nsSvg);
    mXmlWriter.writeNamespace(nsXLink, "xlink");
    mXmlWriter.writeNamespace(Settings::documentNamespaceUri, "tb");
    mXmlWriter.writeNamespace(nsXHtml, "xhtml");

    writeSvgElement(proxy);

    // Get the items from the scene
    QList<QGraphicsItem*> items = mScene->items();

    std::sort(items.begin(), items.end(), itemZIndexComp);

    GraphicsStroke *openStroke = 0;

    bool groupHoldsInfo = false;

    while (!items.empty())
    {
        QGraphicsItem *item = items.takeFirst();

        // Is the item a polygon?
        GraphicsPolygonItem *polygonItem = qgraphicsitem_cast<GraphicsPolygonItem*> (item);
        if (polygonItem && polygonItem->isVisible())
        {
            GraphicsStroke* currentStroke = polygonItem->stroke();
            if (openStroke && (currentStroke != openStroke))
            {
                mXmlWriter.writeEndElement(); //g
                openStroke = 0;
                groupHoldsInfo = false;
            }

            bool firstPolygonInStroke = currentStroke  && !openStroke;

            if (firstPolygonInStroke)
            {
                mXmlWriter.writeStartElement("g");
                openStroke = currentStroke;

                GraphicsStroke* stroke = dynamic_cast<GraphicsStroke* >(currentStroke);

                if (stroke)
                {
                    QColor colorOnDarkBackground = polygonItem->colorOnDarkBackground();
                    QColor colorOnLightBackground = polygonItem->colorOnLightBackground();
                    GraphicsStrokesGroup * sg = polygonItem->strokesGroup();

                    if (colorOnDarkBackground.isValid() && colorOnLightBackground.isValid() && sg)
                    {
                        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "z-value"
                                                  , QString("%1").arg(polygonItem->strokesGroup()->zValue()));

                        mXmlWriter.writeAttribute(Settings::documentNamespaceUri
                                                  , "fill-on-dark-background", colorOnDarkBackground.name());
                        mXmlWriter.writeAttribute(Settings::documentNamespaceUri
                                                  , "fill-on-light-background", colorOnLightBackground.name());

                        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "uuid", StringUtils::toCanonicalUuid(sg->uuid()));

                        QVariant locked = sg->data(GraphicsItemData::ItemLocked);
                        if (!locked.isNull() && locked.toBool())
                            mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "locked", xmlTrue);

                        QVariant layer = sg->data(GraphicsItemData::ItemLayerType);
                        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "layer", QString("%1").arg(layer.toInt()));

                        QTransform matrix = sg->sceneTransform();
                        if (!matrix.isIdentity())
                            mXmlWriter.writeAttribute("transform", toSvgTransform(matrix));

                        qDebug() << "Attributes written";

                        groupHoldsInfo = true;
                    }
                }

                if (stroke && !stroke->hasPressure())
                {

                    strokeToSvgPolyline(stroke, groupHoldsInfo);

                    //we can dequeue all polygons belonging to that stroke
                    foreach(GraphicsPolygonItem* gi, stroke->polygons())
                    {
                        items.removeOne(gi);
                    }
                    continue;
                }
            }

            GraphicsStroke* stroke = dynamic_cast<GraphicsStroke* >(currentStroke);

            if (stroke && stroke->hasPressure())
                polygonItemToSvgPolygon(polygonItem, groupHoldsInfo);

            else if (polygonItem->isNominalLine())
                polygonItemToSvgLine(polygonItem, groupHoldsInfo);


            continue;
        }

        if (openStroke)
        {
            mXmlWriter.writeEndElement(); //g
            groupHoldsInfo = false;
            openStroke = 0;
        }

        // Is the item a picture?
        GraphicsPixmapItem *pixmapItem = qgraphicsitem_cast<GraphicsPixmapItem*> (item);
        if (pixmapItem && pixmapItem->isVisible())
        {
            pixmapItemToLinkedImage(pixmapItem);
            continue;
        }

        // Is the item a shape?
        GraphicsSvgItem *svgItem = qgraphicsitem_cast<GraphicsSvgItem*> (item);
        if (svgItem && svgItem->isVisible())
        {
            svgItemToLinkedSvg(svgItem);
            continue;
        }

        GraphicsVideoItem * videoItem = qgraphicsitem_cast<GraphicsVideoItem*> (item);

        if (videoItem && videoItem->isVisible()) {
            videoItemToLinkedVideo(videoItem);
            continue;
        }

        GraphicsAudioItem * audioItem = qgraphicsitem_cast<GraphicsAudioItem*> (item);

        if (audioItem && audioItem->isVisible()) {
            audioItemToLinkedAudio(audioItem);
            continue;
        }

        // Is the item an app?
        GraphicsAppleWidgetItem *appleWidgetItem = qgraphicsitem_cast<GraphicsAppleWidgetItem*> (item);
        if (appleWidgetItem && appleWidgetItem->isVisible())
        {
            graphicsAppleWidgetToSvg(appleWidgetItem);
            continue;
        }

        // Is the item a web widget (W3C Widgets format)?
        GraphicsWebWidgetItem *webWidgetItem = qgraphicsitem_cast<GraphicsWebWidgetItem*> (item);
        if (webWidgetItem && webWidgetItem->isVisible())
        {
            graphicsWebWidgetToSvg(webWidgetItem);
            continue;
        }

        // Is the item a PDF?
        GraphicsPDFItem *pdfItem = qgraphicsitem_cast<GraphicsPDFItem*> (item);
        if (pdfItem && pdfItem->isVisible())
        {
            pdfItemToLinkedPDF(pdfItem);
            continue;
        }

        // Is the item a text?
        GraphicsTextItem *textItem = qgraphicsitem_cast<GraphicsTextItem*> (item);
        if (textItem && textItem->isVisible())
        {
            textItemToSvg(textItem);
            continue;
        }

        // Is the item a curtain?
        GraphicsCurtainItem *curtainItem = qgraphicsitem_cast<GraphicsCurtainItem*> (item);
        if (curtainItem && curtainItem->isVisible())
        {
            curtainItemToSvg(curtainItem);
            continue;
        }

        // Is the item a ruler?
        GraphicsRuler *ruler = qgraphicsitem_cast<GraphicsRuler*> (item);
        if (ruler && ruler->isVisible())
        {
            rulerToSvg(ruler);
            continue;
        }

        // Is the item a cache?
        GraphicsCache* cache = qgraphicsitem_cast<GraphicsCache*>(item);
        if(cache && cache->isVisible())
        {
            cacheToSvg(cache);
            continue;
        }

        // Is the item a compass
        GraphicsCompass *compass = qgraphicsitem_cast<GraphicsCompass*> (item);
        if (compass && compass->isVisible())
        {
            compassToSvg(compass);
            continue;
        }

        // Is the item a protractor?
        GraphicsProtractor *protractor = qgraphicsitem_cast<GraphicsProtractor*> (item);
        if (protractor && protractor->isVisible())
        {
            protractorToSvg(protractor);
            continue;
        }

        // Is the item a triangle?
        GraphicsTriangle *triangle = qgraphicsitem_cast<GraphicsTriangle*> (item);
        if (triangle && triangle->isVisible())
        {
            triangleToSvg(triangle);
            continue;
        }

        // Is the item a group?
        GraphicsGroupContainerItem *groupItem = qgraphicsitem_cast<GraphicsGroupContainerItem*>(item);
        if (groupItem && groupItem->isVisible())
        {
            persistGroupToDom(groupItem, &groupRoot, &groupDomDocument);
            continue;
        }
    }

    if (openStroke)
    {
        mXmlWriter.writeEndElement();
        groupHoldsInfo = false;
        openStroke = 0;
    }

    //writing group data
    if (groupRoot.hasChildNodes()) {
        mXmlWriter.writeStartElement(tGroups);
        QDomElement curElement = groupRoot.firstChildElement();
        while (!curElement.isNull()) {
            if (curElement.hasAttribute(aId)) {
                mXmlWriter.writeStartElement(curElement.tagName());
                mXmlWriter.writeAttribute(aId, curElement.attribute(aId));
                if(curElement.hasAttribute("locked")){
                    mXmlWriter.writeAttribute(Settings::documentNamespaceUri,"locked",curElement.attribute("locked"));
                }
                if(curElement.hasAttribute("layer")){
                    mXmlWriter.writeAttribute(Settings::documentNamespaceUri,"layer",curElement.attribute("layer"));
                }

                QDomElement curSubElement = curElement.firstChildElement();
                while (!curSubElement.isNull()) {
                    if (curSubElement.hasAttribute(aId)) {
                        mXmlWriter.writeStartElement(curSubElement.tagName());
                        mXmlWriter.writeAttribute(aId, curSubElement.attribute(aId));
                        mXmlWriter.writeEndElement();
                        curSubElement = curSubElement.nextSiblingElement();
                    }
                }
                mXmlWriter.writeEndElement();
            }
            curElement = curElement.nextSiblingElement();
        }
        mXmlWriter.writeEndElement();
    }

    mXmlWriter.writeEndDocument();
    QString fileName = mDocumentPath + FileSystemUtils::digitFileFormat("/page%1.svg", mPageIndex);
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qCritical() << "cannot open " << fileName << " for writing ...";
        return false;
    }
    file.write(buffer.data());
    file.flush();
    file.close();

    return true;
}

void SvgSubsetAdaptor::SvgSubsetWriter::persistGroupToDom(QGraphicsItem *groupItem, QDomElement *curParent, QDomDocument *groupDomDocument)
{
    QUuid uuid = GraphicsScene::getPersonalUuid(groupItem);
    if (!uuid.isNull()) {
        QDomElement curGroupElement = groupDomDocument->createElement(tGroup);
        curGroupElement.setAttribute(aId, uuid.toString());
        GraphicsGroupContainerItem* group = dynamic_cast<GraphicsGroupContainerItem*>(groupItem);
        if(group && group->Delegate()){
            if(group->Delegate()->isLocked())
                curGroupElement.setAttribute("locked", xmlTrue);
            else
                curGroupElement.setAttribute("locked", xmlFalse);

            curGroupElement.setAttribute("layer", group->data(GraphicsItemData::ItemLayerType).toString());
        }
        curParent->appendChild(curGroupElement);
        foreach (QGraphicsItem *item, groupItem->childItems()) {
            QUuid tmpUuid = GraphicsScene::getPersonalUuid(item);
            if (!tmpUuid.isNull()) {
                if (item->type() == GraphicsGroupContainerItem::Type && item->childItems().count())
                    persistGroupToDom(item, curParent, groupDomDocument);
                else {
                    QDomElement curSubElement = groupDomDocument->createElement(tElement);
                    curSubElement.setAttribute(aId, tmpUuid.toString());
                    curGroupElement.appendChild(curSubElement);
                }
            }
        }
    }
}

void SvgSubsetAdaptor::SvgSubsetWriter::polygonItemToSvgLine(GraphicsPolygonItem* polygonItem, bool groupHoldsInfo)
{
    mXmlWriter.writeStartElement("line");

    QLineF line = polygonItem->originalLine();

    mXmlWriter.writeAttribute("x1", QString::number(line.p1().x(), 'f', 2));
    mXmlWriter.writeAttribute("y1", QString::number(line.p1().y(), 'f', 2));

    // SVG renderers (Chrome) do not like line where (x1, y1) == (x2, y2)
    qreal x2 = line.p2().x();
    if (line.p1() == line.p2())
        x2 += 0.01;

    mXmlWriter.writeAttribute("x2", QString::number(x2, 'f', 2));
    mXmlWriter.writeAttribute("y2", QString::number(line.p2().y(), 'f', 2));

    mXmlWriter.writeAttribute("stroke-width", QString::number(polygonItem->originalWidth(), 'f', -1));
    mXmlWriter.writeAttribute("stroke", polygonItem->brush().color().name());

    qreal alpha = polygonItem->brush().color().alphaF();
    if (alpha < 1.0)
        mXmlWriter.writeAttribute("stroke-opacity", QString::number(alpha, 'f', 2));
    mXmlWriter.writeAttribute("stroke-linecap", "round");

    if (!groupHoldsInfo)
    {
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "z-value", QString("%1").arg(polygonItem->zValue()));
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "fill-on-dark-background", polygonItem->colorOnDarkBackground().name());
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "fill-on-light-background", polygonItem->colorOnLightBackground().name());
    }

    mXmlWriter.writeEndElement();

}


void SvgSubsetAdaptor::SvgSubsetWriter::strokeToSvgPolyline(GraphicsStroke* stroke, bool groupHoldsInfo)
{
    QList<GraphicsPolygonItem*> pols = stroke->polygons();

    if (pols.length() > 0)
    {
        mXmlWriter.writeStartElement("polyline");
        QVector<QPointF> points;

        foreach(GraphicsPolygonItem* polygon, pols)
        {
            points << polygon->originalLine().p1();
        }

        points << pols.last()->originalLine().p2();

        // SVG renderers (Chrome) do not like line withe where x1/y1 == x2/y2
        if (points.size() == 2 && (points.at(0) == points.at(1)))
        {
            points[1] = QPointF(points[1].x() + 0.01, points[1].y());
        }

        QString svgPoints = pointsToSvgPointsAttribute(points);
        mXmlWriter.writeAttribute("points", svgPoints);

        GraphicsPolygonItem* firstPolygonItem = pols.at(0);

        mXmlWriter.writeAttribute("fill", "none");
        mXmlWriter.writeAttribute("stroke-width", QString::number(firstPolygonItem->originalWidth(), 'f', 2));
        mXmlWriter.writeAttribute("stroke", firstPolygonItem->brush().color().name());
        mXmlWriter.writeAttribute("stroke-opacity", QString("%1").arg(firstPolygonItem->brush().color().alphaF()));
        mXmlWriter.writeAttribute("stroke-linecap", "round");

        if (!groupHoldsInfo)
        {

            mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "z-value", QString("%1").arg(firstPolygonItem->zValue()));

            mXmlWriter.writeAttribute(Settings::documentNamespaceUri
                                      , "fill-on-dark-background", firstPolygonItem->colorOnDarkBackground().name());
            mXmlWriter.writeAttribute(Settings::documentNamespaceUri
                                      , "fill-on-light-background", firstPolygonItem->colorOnLightBackground().name());
        }

        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "uuid", StringUtils::toCanonicalUuid(firstPolygonItem->uuid()));
        if (firstPolygonItem->parentItem()) {
            mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "parent", StringUtils::toCanonicalUuid(GraphicsItem::getOwnUuid(firstPolygonItem->strokesGroup())));
        }

        mXmlWriter.writeEndElement();
    }
}


void SvgSubsetAdaptor::SvgSubsetWriter::strokeToSvgPolygon(GraphicsStroke* stroke, bool groupHoldsInfo)
{
    QList<GraphicsPolygonItem*> pis = stroke->polygons();

    if (pis.length() > 0)
    {
        QPolygonF united;

        foreach(GraphicsPolygonItem* pi, pis)
        {
            united = united.united(pi->polygon());
        }


        GraphicsPolygonItem *clone = static_cast<GraphicsPolygonItem*>(pis.at(0)->deepCopy());
        clone->setPolygon(united);

        polygonItemToSvgPolygon(clone, groupHoldsInfo);
    }
}

void SvgSubsetAdaptor::SvgSubsetWriter::polygonItemToSvgPolygon(GraphicsPolygonItem* polygonItem, bool groupHoldsInfo)
{

    QPolygonF polygon = polygonItem->polygon();
    int pointsCount = polygon.size();

    if (pointsCount > 0)
    {
        mXmlWriter.writeStartElement("polygon");

        QString points = pointsToSvgPointsAttribute(polygon);
        mXmlWriter.writeAttribute("points", points);
        mXmlWriter.writeAttribute("transform",toSvgTransform(polygonItem->transform()));
        mXmlWriter.writeAttribute("fill", polygonItem->brush().color().name());

        qreal alpha = polygonItem->brush().color().alphaF();
        mXmlWriter.writeAttribute("fill-opacity", QString::number(alpha, 'f', 2));

        // we trick SVG antialiasing, to avoid seeing light gaps between polygons
        if (alpha < 1.0)
        {
            qreal trickedAlpha = trickAlpha(alpha);
            mXmlWriter.writeAttribute("stroke", polygonItem->brush().color().name());
            mXmlWriter.writeAttribute("stroke-width", "1");
            mXmlWriter.writeAttribute("stroke-opacity", QString::number(trickedAlpha, 'f', 2));
        }

        // svg default fill rule is nonzero, but Qt is evenodd
        //
        //http://www.w3.org/TR/SVG11/painting.html
        //http://doc.trolltech.com/4.5/qgraphicspolygonitem.html#fillRule
        //

        if (polygonItem->fillRule() == Qt::OddEvenFill)
            mXmlWriter.writeAttribute("fill-rule", "evenodd");
        else
            mXmlWriter.writeAttribute("fill-rule", "winding");

        if (!groupHoldsInfo)
        {
            mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "z-value", QString("%1").arg(polygonItem->zValue()));
            mXmlWriter.writeAttribute(Settings::documentNamespaceUri
                                      , "fill-on-dark-background", polygonItem->colorOnDarkBackground().name());
            mXmlWriter.writeAttribute(Settings::documentNamespaceUri
                                      , "fill-on-light-background", polygonItem->colorOnLightBackground().name());
        }

        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "uuid", StringUtils::toCanonicalUuid(polygonItem->uuid()));
        GraphicsStrokesGroup* sg = polygonItem->strokesGroup();
        if (sg)
            mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "parent", StringUtils::toCanonicalUuid(sg->uuid()));

        mXmlWriter.writeEndElement();
    }
}


GraphicsPolygonItem* SvgSubsetAdaptor::SvgSubsetReader::polygonItemFromPolygonSvg(const QColor& pDefaultColor)
{
    GraphicsPolygonItem* polygonItem = new GraphicsPolygonItem();

    graphicsItemFromSvg(polygonItem);

    QStringView svgPoints = mXmlReader.attributes().value("points");

    QPolygonF polygon;

    if (!svgPoints.isNull())
    {
        QStringList ts = svgPoints.toString().split(QLatin1Char(' '), Qt::SkipEmptyParts);

        foreach(const QString sPoint, ts)
        {
            QStringList sCoord = sPoint.split(QLatin1Char(','), Qt::SkipEmptyParts);

            if (sCoord.size() == 2)
            {
                QPointF point;
                point.setX(sCoord.at(0).toFloat());
                point.setY(sCoord.at(1).toFloat());
                polygon << point;
            }
            else if (sCoord.size() == 4){
                //This is the case on system were the "," is used to seperate decimal
                QPointF point;
                QString x = sCoord.at(0) + "." + sCoord.at(1);
                QString y = sCoord.at(2) + "." + sCoord.at(3);
                point.setX(x.toFloat());
                point.setY(y.toFloat());
                polygon << point;
            }
            else
            {
                qWarning() << "cannot make sense of a 'point' value" << sCoord;
            }
        }
    }
    else
    {
        qWarning() << "cannot make sense of 'points' value " << svgPoints.toString();
    }

    polygonItem->setPolygon(polygon);

    QStringView svgFill = mXmlReader.attributes().value("fill");

    QColor brushColor = pDefaultColor;

    if (!svgFill.isNull())
        brushColor.setNamedColor(svgFill.toString());

    QStringView svgFillOpacity = mXmlReader.attributes().value("fill-opacity");
    qreal opacity = 1.0;

    if (!svgFillOpacity.isNull())
    {
        opacity = svgFillOpacity.toString().toFloat();
        brushColor.setAlphaF(opacity);
    }

    polygonItem->setColor(brushColor);

    QStringView fillOnDarkBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "fill-on-dark-background");

    if (!fillOnDarkBackgroundValue.isNull())
    {
        QColor color;
        color.setNamedColor(fillOnDarkBackgroundValue.toString());
        if (!color.isValid())
            color = Qt::white;

        color.setAlphaF(opacity);
        polygonItem->setColorOnDarkBackground(color);
    }
    else
    {
        QColor color = mGroupDarkBackgroundColor;
        color.setAlphaF(opacity);
        polygonItem->setColorOnDarkBackground(color);
    }

    QStringView fillOnLightBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "fill-on-light-background");

    if (!fillOnLightBackgroundValue.isNull())
    {
        QColor color;
        color.setNamedColor(fillOnLightBackgroundValue.toString());
        if (!color.isValid())
            color = Qt::black;
        color.setAlphaF(opacity);
        polygonItem->setColorOnLightBackground(color);
    }
    else
    {
        QColor color = mGroupLightBackgroundColor;
        color.setAlphaF(opacity);
        polygonItem->setColorOnLightBackground(color);
    }

    // Before TianBoard v1.4, fill rule was only saved if it was "Even-odd". Therefore if no fill rule
    // is specified, we assume that it should be Winding fill.

    QStringView fillRule = mXmlReader.attributes().value("fill-rule");

    if (!fillRule.isNull() && fillRule.toString() == "evenodd")
        polygonItem->setFillRule(Qt::OddEvenFill);
    else
        polygonItem->setFillRule(Qt::WindingFill);

    return polygonItem;

}

GraphicsPolygonItem* SvgSubsetAdaptor::SvgSubsetReader::polygonItemFromLineSvg(const QColor& pDefaultColor)
{
    QStringView svgX1 = mXmlReader.attributes().value("x1");
    QStringView svgY1 = mXmlReader.attributes().value("y1");
    QStringView svgX2 = mXmlReader.attributes().value("x2");
    QStringView svgY2 = mXmlReader.attributes().value("y2");

    QLineF line;

    if (!svgX1.isNull() && !svgY1.isNull() && !svgX2.isNull() && !svgY2.isNull())
    {
        qreal x1 = svgX1.toString().toFloat();
        qreal y1 = svgY1.toString().toFloat();
        qreal x2 = svgX2.toString().toFloat();
        qreal y2 = svgY2.toString().toFloat();

        line.setLine(x1, y1, x2, y2);
    }
    else
    {
        qWarning() << "cannot make sense of 'line' value";
        return 0;
    }

    QStringView strokeWidth = mXmlReader.attributes().value("stroke-width");

    qreal lineWidth = 1.;

    if (!strokeWidth.isNull())
    {
        lineWidth = strokeWidth.toString().toFloat();
    }

    GraphicsPolygonItem* polygonItem = new GraphicsPolygonItem(line, lineWidth);
    graphicsItemFromSvg(polygonItem);

    QStringView svgStroke = mXmlReader.attributes().value("stroke");

    QColor brushColor = pDefaultColor;

    if (!svgStroke.isNull())
    {
        brushColor.setNamedColor(svgStroke.toString());

    }

    QStringView svgStrokeOpacity = mXmlReader.attributes().value("stroke-opacity");
    qreal opacity = 1.0;

    if (!svgStrokeOpacity.isNull())
    {
        opacity = svgStrokeOpacity.toString().toFloat();
        brushColor.setAlphaF(opacity);
    }

    polygonItem->setColor(brushColor);

    QStringView fillOnDarkBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "fill-on-dark-background");

    if (!fillOnDarkBackgroundValue.isNull())
    {
        QColor color;
        color.setNamedColor(fillOnDarkBackgroundValue.toString());
        if (!color.isValid())
            color = Qt::white;

        color.setAlphaF(opacity);
        polygonItem->setColorOnDarkBackground(color);
    }
    else
    {
        QColor color = mGroupDarkBackgroundColor;
        color.setAlphaF(opacity);
        polygonItem->setColorOnDarkBackground(color);
    }

    QStringView fillOnLightBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "fill-on-light-background");

    if (!fillOnLightBackgroundValue.isNull())
    {
        QColor color;
        color.setNamedColor(fillOnLightBackgroundValue.toString());
        if (!color.isValid())
            color = Qt::black;
        color.setAlphaF(opacity);
        polygonItem->setColorOnLightBackground(color);
    }
    else
    {
        QColor color = mGroupLightBackgroundColor;
        color.setAlphaF(opacity);
        polygonItem->setColorOnLightBackground(color);
    }

    return polygonItem;
}

QList<GraphicsPolygonItem*> SvgSubsetAdaptor::SvgSubsetReader::polygonItemsFromPolylineSvg(const QColor& pDefaultColor)
{
    QStringView strokeWidth = mXmlReader.attributes().value("stroke-width");

    qreal lineWidth = 1.;

    if (!strokeWidth.isNull())
    {
        lineWidth = strokeWidth.toString().toFloat();
    }

    QColor brushColor = pDefaultColor;

    QStringView svgStroke = mXmlReader.attributes().value("stroke");
    if (!svgStroke.isNull())
    {
        brushColor.setNamedColor(svgStroke.toString());
    }

    qreal opacity = 1.0;

    QStringView svgStrokeOpacity = mXmlReader.attributes().value("stroke-opacity");
    if (!svgStrokeOpacity.isNull())
    {
        opacity = svgStrokeOpacity.toString().toFloat();
        brushColor.setAlphaF(opacity);
    }

    QStringView zValueAttr = mXmlReader.attributes().value(mNamespaceUri, "z-value");

    qreal zValue = mGroupZIndex;
    if (!zValueAttr.isNull())
    {
        zValue = zValueAttr.toString().toFloat();
    }

    QColor colorOnDarkBackground = mGroupDarkBackgroundColor;

    QStringView fillOnDarkBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "fill-on-dark-background");
    if (!fillOnDarkBackgroundValue.isNull())
    {
        colorOnDarkBackground.setNamedColor(fillOnDarkBackgroundValue.toString());
    }

    if (!colorOnDarkBackground.isValid())
        colorOnDarkBackground = Qt::white;

    colorOnDarkBackground.setAlphaF(opacity);

    QColor colorOnLightBackground = mGroupLightBackgroundColor;

    QStringView fillOnLightBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "fill-on-light-background");
    if (!fillOnLightBackgroundValue.isNull())
    {
        QColor colorOnLightBackground;
        colorOnLightBackground.setNamedColor(fillOnLightBackgroundValue.toString());
    }

    if (!colorOnLightBackground.isValid())
        colorOnLightBackground = Qt::black;

    colorOnLightBackground.setAlphaF(opacity);

    QStringView svgPoints = mXmlReader.attributes().value("points");

    QList<GraphicsPolygonItem*> polygonItems;

    if (!svgPoints.isNull())
    {
        QStringList ts = svgPoints.toString().split(QLatin1Char(' '),
                                                    Qt::SkipEmptyParts);

        QList<QPointF> points;

        foreach(const QString sPoint, ts)
        {
            QStringList sCoord = sPoint.split(QLatin1Char(','), Qt::SkipEmptyParts);

            if (sCoord.size() == 2)
            {
                QPointF point;
                point.setX(sCoord.at(0).toFloat());
                point.setY(sCoord.at(1).toFloat());
                points << point;
            }
            else if (sCoord.size() == 4){
                //This is the case on system were the "," is used to seperate decimal
                QPointF point;
                QString x = sCoord.at(0) + "." + sCoord.at(1);
                QString y = sCoord.at(2) + "." + sCoord.at(3);
                point.setX(x.toFloat());
                point.setY(y.toFloat());
                points << point;
            }
            else
            {
                qWarning() << "cannot make sense of a 'point' value" << sCoord;
            }
        }

        for (int i = 0; i < points.size() - 1; i++)
        {
            GraphicsPolygonItem* polygonItem = new GraphicsPolygonItem(QLineF(points.at(i), points.at(i + 1)), lineWidth);
            polygonItem->setColor(brushColor);
            GraphicsItem::assignZValue(polygonItem, zValue);
            polygonItem->setColorOnDarkBackground(colorOnDarkBackground);
            polygonItem->setColorOnLightBackground(colorOnLightBackground);

            polygonItems <<polygonItem;
        }
    }
    else
    {
        qWarning() << "cannot make sense of 'points' value " << svgPoints.toString();
    }

    return polygonItems;
}




void SvgSubsetAdaptor::SvgSubsetWriter::pixmapItemToLinkedImage(GraphicsPixmapItem* pixmapItem)
{
    mXmlWriter.writeStartElement("image");

    QString fileName = PersistenceManager::imageDirectory + "/" + pixmapItem->uuid().toString() + ".png";

    QString path = mDocumentPath + "/" + fileName;

    mXmlWriter.writeAttribute(nsXLink, "href", fileName);

    graphicsItemToSvg(pixmapItem);

    mXmlWriter.writeEndElement();
}


GraphicsPixmapItem* SvgSubsetAdaptor::SvgSubsetReader::pixmapItemFromSvg()
{

    GraphicsPixmapItem* pixmapItem = new GraphicsPixmapItem();

    QStringView imageHref = mXmlReader.attributes().value(nsXLink, "href");

    if (!imageHref.isNull())
    {
        QString href = imageHref.toString();
        QPixmap pix(mDocumentPath + "/" + FileSystemUtils::normalizeFilePath(href));
        pixmapItem->setPixmap(pix);
    }
    else
    {
        qWarning() << "cannot make sens of image href value";
        return 0;
    }

    graphicsItemFromSvg(pixmapItem);

    return pixmapItem;

}


void SvgSubsetAdaptor::SvgSubsetWriter::svgItemToLinkedSvg(GraphicsSvgItem* svgItem)
{

    mXmlWriter.writeStartElement("image");

    QString fileName = PersistenceManager::imageDirectory + "/" + svgItem->uuid().toString() + ".svg";


    mXmlWriter.writeAttribute(nsXLink, "href", fileName);

    graphicsItemToSvg(svgItem);

    mXmlWriter.writeEndElement();
}


GraphicsSvgItem* SvgSubsetAdaptor::SvgSubsetReader::svgItemFromSvg()
{
    GraphicsSvgItem* svgItem = 0;

    QStringView imageHref = mXmlReader.attributes().value(nsXLink, "href");

    if (!imageHref.isNull())
    {
        QString href = imageHref.toString();

        svgItem = new GraphicsSvgItem(mDocumentPath + "/" + FileSystemUtils::normalizeFilePath(href));
    }
    else
    {
        qWarning() << "cannot make sens of image href value";
        return 0;
    }

    graphicsItemFromSvg(svgItem);

    return svgItem;

}


void SvgSubsetAdaptor::SvgSubsetWriter::pdfItemToLinkedPDF(GraphicsPDFItem* pdfItem)
{
    mXmlWriter.writeStartElement("foreignObject");
    mXmlWriter.writeAttribute("requiredExtensions", "http://ns.adobe.com/pdf/1.3/");

    QString fileName = PersistenceManager::objectDirectory + "/" + pdfItem->fileUuid().toString() + ".pdf";

    QString path = mDocumentPath + "/" + fileName;

    if (!QFile::exists(path))
    {
        QDir dir;
        dir.mkdir(mDocumentPath + "/" + PersistenceManager::objectDirectory);

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
        {
            qWarning() << "cannot open file for writing embeded pdf content " << path;
            return;
        }

        file.write(pdfItem->fileData());
        file.close();
    }

    mXmlWriter.writeAttribute(nsXLink, "href", fileName + "#page=" + QString::number(pdfItem->pageNumber()));

    graphicsItemToSvg(pdfItem);

    mXmlWriter.writeEndElement();
}


GraphicsPDFItem* SvgSubsetAdaptor::SvgSubsetReader::pdfItemFromPDF()
{
    GraphicsPDFItem* pdfItem = 0;

    QString href = mXmlReader.attributes().value(nsXLink, "href").toString();
    QStringList parts = href.split("#page=");
    if (parts.count() != 2)
    {
        qWarning() << "invalid pdf href value" << href;
        return 0;
    }

    QString pdfPath = parts[0];
    QUuid uuid(QFileInfo(pdfPath).baseName());
    int pageNumber = parts[1].toInt();

    pdfItem = new GraphicsPDFItem(PDFRenderer::rendererForUuid(uuid, mDocumentPath + "/" + FileSystemUtils::normalizeFilePath(pdfPath)), pageNumber);

    graphicsItemFromSvg(pdfItem);

    return pdfItem;
}

void SvgSubsetAdaptor::SvgSubsetWriter::audioItemToLinkedAudio(GraphicsAudioItem *audioItem)
{
    mXmlWriter.writeStartElement("audio");

    graphicsItemToSvg(audioItem);

    if (audioItem->playerState() == QMediaPlayer::PausedState &&
       (audioItem->mediaDuration() - audioItem->mediaPosition()) > 0)
    {
        qint64 pos = audioItem->mediaPosition();
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "position", QString("%1").arg(pos));
    }

    QString audioFileHref = "audios/" + audioItem->mediaFileUrl().fileName();

    mXmlWriter.writeAttribute(nsXLink, "href", audioFileHref);
    mXmlWriter.writeEndElement();
}


void SvgSubsetAdaptor::SvgSubsetWriter::videoItemToLinkedVideo(GraphicsVideoItem* videoItem)
{
    /* w3c sample
     *
     *  <video xlink:href="noonoo.avi" volume=".8" type="video/x-msvideo"
     *               width="320" height="240" x="50" y="50" repeatCount="indefinite"/>
     *
     */

    mXmlWriter.writeStartElement("video");

    graphicsItemToSvg(videoItem);

    if (videoItem->playerState() == QMediaPlayer::PausedState &&
       (videoItem->mediaDuration() - videoItem->mediaPosition()) > 0)
    {
        qint64 pos = videoItem->mediaPosition();
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "position", QString("%1").arg(pos));
    }

    QString videoFileHref = "videos/" + videoItem->mediaFileUrl().fileName();

    mXmlWriter.writeAttribute(nsXLink, "href", videoFileHref);
    mXmlWriter.writeEndElement();
}

GraphicsMediaItem* SvgSubsetAdaptor::SvgSubsetReader::audioItemFromSvg()
{

    QStringView audioHref = mXmlReader.attributes().value(nsXLink, "href");

    if (audioHref.isNull())
    {
        qWarning() << "cannot make sens of video href value";
        return 0;
    }

    QString href = mDocumentPath + "/" + audioHref.toString();

// 页面里记的是绝对路径，换机器或共享文档后必然失效，
// 所以导入时折算成相对文档根目录的路径。
    if(!audioHref.toString().startsWith("audios/")){
        int indexOfAudioDirectory = href.lastIndexOf("audios");
        href = mDocumentPath + "/" + href.right(href.length() - indexOfAudioDirectory);
    }

    GraphicsMediaItem* audioItem = GraphicsMediaItem::createMediaItem(QUrl::fromLocalFile(href));
    if(audioItem)
        audioItem->connect(Application::boardController, &BoardController::activeSceneChanged, audioItem, &GraphicsMediaItem::activeSceneChanged);

    graphicsItemFromSvg(audioItem);
    QStringView posValue = mXmlReader.attributes().value(mNamespaceUri, "position");

    qint64 p = 0;
    if (!posValue.isNull())
        p = posValue.toString().toLongLong();

    audioItem->setInitialPos(p);
    return audioItem;
}

GraphicsMediaItem* SvgSubsetAdaptor::SvgSubsetReader::videoItemFromSvg()
{

    QStringView videoHref = mXmlReader.attributes().value(nsXLink, "href");

    if (videoHref.isNull())
    {
        qWarning() << "cannot make sens of video href value";
        return 0;
    }

    QString href = mDocumentPath + "/" + videoHref.toString();

// 页面里记的是绝对路径，换机器或共享文档后必然失效，
// 所以导入时折算成相对文档根目录的路径。
    if(!videoHref.toString().startsWith("videos/")){
        int indexOfAudioDirectory = href.lastIndexOf("videos");
        href = mDocumentPath + "/" + href.right(href.length() - indexOfAudioDirectory);
    }

    GraphicsMediaItem* videoItem = GraphicsMediaItem::createMediaItem(QUrl::fromLocalFile(href));
    if(videoItem){
        QObject::connect(Application::boardController, &BoardController::activeSceneChanged, videoItem, &GraphicsMediaItem::activeSceneChanged);
    }

    graphicsItemFromSvg(videoItem);
    QStringView posValue = mXmlReader.attributes().value(mNamespaceUri, "position");

    qint64 p = 0;
    if (!posValue.isNull())
    {
        p = posValue.toString().toLongLong();
    }

    videoItem->setInitialPos(p);
    return videoItem;
}

void SvgSubsetAdaptor::SvgSubsetReader::graphicsItemFromSvg(QGraphicsItem* gItem)
{

    QStringView svgTransform = mXmlReader.attributes().value("transform");

    QTransform itemMatrix;

    if (!svgTransform.isNull())
    {
        itemMatrix = fromSvgTransform(svgTransform.toString());
        gItem->setTransform(itemMatrix);
    }

    QStringView svgX = mXmlReader.attributes().value("x");
    QStringView svgY = mXmlReader.attributes().value("y");

    if (mFileVersion >= 40202)
    {
        gItem->setPos(0,0);
    }
    else if (mFileVersion >= 40201)
    {
        if (!svgX.isNull() && !svgY.isNull())
        {
            gItem->setPos(svgX.toString().toFloat() * itemMatrix.m11(), svgY.toString().toFloat() * itemMatrix.m22());
        }
    }
    else
    {
        if (!svgX.isNull() && !svgY.isNull())
        {
#ifdef Q_OS_WIN
            gItem->setPos(svgX.toString().toFloat(), svgY.toString().toFloat());
#endif
        }
    }

    ResizableGraphicsItem *rgi = dynamic_cast<ResizableGraphicsItem*>(gItem);

    if (rgi)
    {
        QStringView svgWidth = mXmlReader.attributes().value("width");
        QStringView svgHeight = mXmlReader.attributes().value("height");

        if (!svgWidth.isNull() && !svgHeight.isNull())
        {
            rgi->resize(svgWidth.toString().toFloat(), svgHeight.toString().toFloat());
        }
    }

    QStringView zValueAttr = mXmlReader.attributes().value(mNamespaceUri, "z-value");

    if (!zValueAttr.isNull()){
        // FIX
        // In the firsts zvalue implemenations values outside the boudaries have been used.
        // No boundaries specified on documentation but to small values are not correctly handled.
        qreal zValue = zValueAttr.toString().toFloat();
        while(zValue < -999999) zValue /= 10.;
        GraphicsItem::assignZValue(gItem, zValue);
    }

    Item* boardItem = dynamic_cast<Item*>(gItem);

    if (boardItem)
    {
        QStringView uuidValue = mXmlReader.attributes().value(mNamespaceUri, "uuid");

        if (!uuidValue.isNull())
            boardItem->setUuid(QUuid(uuidValue.toString()));
        else
            boardItem->setUuid(QUuid::createUuid());

        QStringView sourceValue = mXmlReader.attributes().value(mNamespaceUri, "source");

        if (!sourceValue.isNull())
            boardItem->setSourceUrl(QUrl(sourceValue.toString()));
    }

    QStringView lockedValue = mXmlReader.attributes().value(mNamespaceUri, "locked");

    if (!lockedValue.isNull())
    {
        bool isLocked = (lockedValue.toString() == xmlTrue || lockedValue.toString() == "1");
        gItem->setData(GraphicsItemData::ItemLocked, QVariant(isLocked));
    }

    QStringView editableValue = mXmlReader.attributes().value(mNamespaceUri, "editable");

    if (!editableValue.isNull())
    {
        bool isEditable = (editableValue.toString() == xmlTrue || editableValue.toString() == "1");
        gItem->setData(GraphicsItemData::ItemEditable, QVariant(isEditable));
    }

    //deprecated as of 4.4.a.12
    QStringView layerValue = mXmlReader.attributes().value(mNamespaceUri, "layer");
    if (!layerValue.isNull())
    {
        bool ok;
        int layerAsInt = layerValue.toString().toInt(&ok);

        if (ok)
            gItem->setData(GraphicsItemData::ItemLayerType, QVariant(layerAsInt));
    }
}

void SvgSubsetAdaptor::SvgSubsetWriter::graphicsItemToSvg(QGraphicsItem* item)
{
    mXmlWriter.writeAttribute("x", "0");
    mXmlWriter.writeAttribute("y", "0");

    mXmlWriter.writeAttribute("width", QString("%1").arg(item->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->boundingRect().height()));

    mXmlWriter.writeAttribute("transform", toSvgTransform(item->sceneTransform()));

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "z-value", zs);

    bool isBackground = mScene->isBackgroundObject(item);

    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "background", isBackground ? xmlTrue : xmlFalse);

    Item* boardItem = dynamic_cast<Item*>(item);

    if (boardItem)
    {
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "uuid", StringUtils::toCanonicalUuid(boardItem->uuid()));

        QUrl sourceUrl = boardItem->sourceUrl();

        if (!sourceUrl.isEmpty())
            mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "source", sourceUrl.path());

    }

    QVariant layer = item->data(GraphicsItemData::ItemLayerType);
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "layer", QString("%1").arg(layer.toInt()));

    QVariant locked = item->data(GraphicsItemData::ItemLocked);

    if (!locked.isNull() && locked.toBool())
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "locked", xmlTrue);

    QVariant editable = item->data(GraphicsItemData::ItemEditable);
    if (!editable.isNull()) {
        if (editable.toBool())
            mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "editable", xmlTrue);
        else
            mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "editable", xmlFalse);
    }
}




void SvgSubsetAdaptor::SvgSubsetWriter::graphicsAppleWidgetToSvg(GraphicsAppleWidgetItem* item)
{
    graphicsWidgetToSvg(item);
}

void SvgSubsetAdaptor::SvgSubsetWriter::graphicsWebWidgetToSvg(GraphicsWebWidgetItem* item)
{
    graphicsWidgetToSvg(item);
}

void SvgSubsetAdaptor::SvgSubsetWriter::graphicsWidgetToSvg(GraphicsWidgetItem* item)
{
    QUrl widgetRootUrl = item->widgetUrl();
    QString uuid = StringUtils::toCanonicalUuid(item->uuid());
    QString widgetDirectoryPath = PersistenceManager::widgetDirectory;
    if (widgetRootUrl.toString().startsWith("file://"))
    {
        QString widgetRootDir = widgetRootUrl.toLocalFile();
        QFileInfo fi(widgetRootDir);
        QString extension = fi.suffix();

        QString widgetTargetDir = widgetDirectoryPath + "/" + item->uuid().toString() + "." + extension;

        QString path = mDocumentPath + "/" + widgetTargetDir;
        QDir dir(path);

        if (!dir.exists(path))
        {
            QDir dir;
            dir.mkpath(path);
            FileSystemUtils::copyDir(widgetRootDir, path);
        }

        widgetRootUrl = widgetTargetDir;
    }

    QString widgetPath = "widgets/" + widgetRootUrl.fileName();

    mXmlWriter.writeStartElement("foreignObject");
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "src", widgetPath);

    graphicsItemToSvg(item);

    if (item->isFrozen())
    {
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "frozen", xmlTrue);
    }

    mXmlWriter.writeStartElement(nsXHtml, "iframe");

    mXmlWriter.writeAttribute("style", "border: none");
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->boundingRect().height()));

    QString startFileUrl;
    if (item->mainHtmlFileName().startsWith("http://"))
        startFileUrl = item->mainHtmlFileName();
    else
        startFileUrl = widgetRootUrl.toString() + "/" + item->mainHtmlFileName();

    startFileUrl = QUrl::fromPercentEncoding(startFileUrl.toUtf8());

    mXmlWriter.writeAttribute("src", startFileUrl);
    mXmlWriter.writeEndElement(); //iFrame

    //persists widget state
    QMap<QString, QString> preferences = item->preferences();

    foreach(QString key, preferences.keys())
    {
        QString value = preferences.value(key);

        mXmlWriter.writeStartElement(Settings::documentNamespaceUri, "preference");

        mXmlWriter.writeAttribute("key", key);
        mXmlWriter.writeAttribute("value", value);

        mXmlWriter.writeEndElement(); //tb::preference
    }

    //persists datasore state
    QMap<QString, QString> datastore = item->datastoreEntries();

    foreach(QString key, datastore.keys())
    {
        QString value = datastore.value(key);

        mXmlWriter.writeStartElement(Settings::documentNamespaceUri, "datastoreEntry");

        mXmlWriter.writeAttribute("key", key);
        mXmlWriter.writeAttribute("value", value);

        mXmlWriter.writeEndElement(); //tb::datastoreEntry
    }

    mXmlWriter.writeEndElement();
}


GraphicsAppleWidgetItem* SvgSubsetAdaptor::SvgSubsetReader::graphicsAppleWidgetFromSvg()
{

    QStringView widgetUrl = mXmlReader.attributes().value(mNamespaceUri, "src");

    if (widgetUrl.isNull())
    {
        qWarning() << "cannot make sens of widget src value";
        return 0;
    }

    QString href = widgetUrl.toString();

    QUrl url(href);

    if (url.isRelative())
    {
        href = mDocumentPath + "/" + FileSystemUtils::normalizeFilePath(widgetUrl.toString());
    }

    GraphicsAppleWidgetItem* widgetItem = new GraphicsAppleWidgetItem(QUrl::fromLocalFile(href));

    graphicsItemFromSvg(widgetItem);

    return widgetItem;
}

GraphicsWebWidgetItem* SvgSubsetAdaptor::SvgSubsetReader::graphicsWebWidgetFromSvg()
{
    QStringView widgetUrl = mXmlReader.attributes().value(mNamespaceUri, "src");

    if (widgetUrl.isNull())
    {
        qWarning() << "cannot make sens of widget src value";
        return 0;
    }

    QString href = widgetUrl.toString();
    QUrl url(href);

    if (url.isRelative())
    {
        href = mDocumentPath + "/" + FileSystemUtils::normalizeFilePath(widgetUrl.toString());
    }

    GraphicsWebWidgetItem* widgetItem = new GraphicsWebWidgetItem(QUrl::fromLocalFile(href));

    QStringView uuid = mXmlReader.attributes().value(mNamespaceUri, "uuid");
    QString pixPath = mDocumentPath + "/" + PersistenceManager::widgetDirectory + "/" + uuid.toString() + ".png";

    QPixmap snapshot(pixPath);
    if (!snapshot.isNull())
        widgetItem->setSnapshot(snapshot);

    QStringView frozen = mXmlReader.attributes().value(mNamespaceUri, "frozen");

    if (!frozen.isNull() && frozen.toString() == xmlTrue && !snapshot.isNull())
    {
        widgetItem->freeze();
    }

    graphicsItemFromSvg(widgetItem);

    return widgetItem;
}

void SvgSubsetAdaptor::SvgSubsetWriter::textItemToSvg(GraphicsTextItem* item)
{
    /**
     * sample
     *
        <foreignObject x="0" y="0" width="489.297" height="76.3303" transform="matrix(0.508456, -0.861088, 0.861088, 0.508456, -181.721, 243.124)" tb:z-value="-9999998.000000" tb:background="false" tb:fill-on-dark-background="#81ff5c" tb:fill-on-light-background="#008000">
            <xhtml:body>
                <xhtml:div>
                    <xhtml:font face="Arial" style="font-size:48px;" color="#81ff5c">this a text<br/>on 2 lines</xhtml:font>
                </xhtml:div>
            </xhtml:body>
        </foreignObject>
     */

    mXmlWriter.writeStartElement("foreignObject");
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "type", "text");

    graphicsItemToSvg(item);

    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "width", QString("%1").arg(item->textWidth()));
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "height", QString("%1").arg(item->textHeight()));
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "pixels-per-point", QString("%1").arg(item->pixelsPerPoint()));

    QColor colorDarkBg = item->colorOnDarkBackground();
    QColor colorLightBg = item->colorOnLightBackground();

    mXmlWriter.writeAttribute(Settings::documentNamespaceUri
                              , "fill-on-dark-background", colorDarkBg.name());
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri
                              , "fill-on-light-background", colorLightBg.name());

    //for new documents from version 4.5.0
    mXmlWriter.writeStartElement("itemTextContent");

    // Note: don't use mXmlWriter.writeCDATA(htmlString); because it doesn't escape characters sequences correctly.
    // Texts copied from other programs like Open-Office can truncate the svg file.
    //mXmlWriter.writeCharacters(item->toHtml());
    mXmlWriter.writeCharacters(TextTools::cleanHtmlCData(item->toHtml()));
    mXmlWriter.writeEndElement(); //itemTextContent

    mXmlWriter.writeEndElement(); //foreignObject
}

GraphicsTextItem* SvgSubsetAdaptor::SvgSubsetReader::textItemFromSvg()
{
    qreal width = mXmlReader.attributes().value("width").toString().toFloat();
    qreal height = mXmlReader.attributes().value("height").toString().toFloat();

    qreal originalPixelsPerPoint = mXmlReader.attributes().value(mNamespaceUri, "pixels-per-point").toString().toDouble();

    GraphicsTextItem* textItem = new GraphicsTextItem();

    graphicsItemFromSvg(textItem);

    QStringView fillOnDarkBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "fill-on-dark-background");
    QStringView fillOnLightBackgroundValue = mXmlReader.attributes().value(mNamespaceUri, "fill-on-light-background");

    if (!fillOnDarkBackgroundValue.isNull()) {
        QColor color;
        color.setNamedColor(fillOnDarkBackgroundValue.toString());
        if (!color.isValid())
            color = Qt::white;
        textItem->setColorOnDarkBackground(color);
    }

    if (!fillOnLightBackgroundValue.isNull()) {
        QColor color;
        color.setNamedColor(fillOnLightBackgroundValue.toString());
        if (!color.isValid())
            color = Qt::black;
        textItem->setColorOnLightBackground(color);
    }

    QString text;

    while (!(mXmlReader.isEndElement() && (mXmlReader.name() == "font" || mXmlReader.name() == "foreignObject")))
    {
        if (mXmlReader.hasError())
        {
            delete textItem;
            textItem = 0;
            return 0;
        }

        mXmlReader.readNext();
        if (mXmlReader.isStartElement())
        {
            //for new documents from version 4.5.0
            if (mFileVersion >= 40500) {
                if (mXmlReader.name() == "itemTextContent") {
                    text = mXmlReader.readElementText();
                    textItem->setHtml(text);

                    // Fonts sizes are not displayed the same across platforms: e.g a text item with the same
                    // font size (in Pts) is displayed smaller on Linux than Windows. This messes up layouts
                    // when importing documents created on another computer, so if a font is being displayed
                    // at a different size (relative to the rest of the document) than it was when created,
                    // we adjust its size.
                    if (originalPixelsPerPoint != 0) {
                        qreal pixelsPerPoint = textItem->pixelsPerPoint();

                        qDebug() << "Pixels per point: original/current" << originalPixelsPerPoint
                                 << "/" << pixelsPerPoint;
                        qreal ratio = originalPixelsPerPoint/pixelsPerPoint;

                        if (ratio != 1) {
                            qDebug() << "Scaling text by " << ratio;
                            GraphicsTextItemDelegate* textDelegate = dynamic_cast<GraphicsTextItemDelegate*>(textItem->Delegate());
                            if (textDelegate)
                                textDelegate->scaleTextSize(ratio);
                        }
                    }
                    else
                        // mark scene as modified so the text item will be saved with a pixelsPerPoint value
                        saveSceneAfterLoading = true;


                    textItem->resize(width, height);
                    if (textItem->toPlainText().isEmpty()) {
                        delete textItem;
                        textItem = 0;
                    }
                    return textItem;
                }

                //tracking for backward capability with older versions
            } else if (mXmlReader.name() == "font")  {
                QFont font = textItem->font();

                QStringView fontFamily = mXmlReader.attributes().value("face");

                if (!fontFamily.isNull()) {
                    font.setFamily(fontFamily.toString());
                }
                QStringView fontStyle = mXmlReader.attributes().value("style");
                if (!fontStyle.isNull()) {
                    foreach (QString styleToken, fontStyle.toString().split(";")) {
                        styleToken = styleToken.trimmed();
                        if (styleToken.startsWith(sFontSizePrefix) && styleToken.endsWith(sPixelUnit)) {
                            int fontSize = styleToken.mid(
                                        sFontSizePrefix.length(),
                                        styleToken.length() - sFontSizePrefix.length() - sPixelUnit.length()).toInt();
                            font.setPixelSize(fontSize);
                        } else if (styleToken.startsWith(sFontWeightPrefix)) {
                            QString fontWeight = styleToken.mid(
                                        sFontWeightPrefix.length(),
                                        styleToken.length() - sFontWeightPrefix.length());
                            font.setBold(fontWeight.contains("bold"));
                        } else if (styleToken.startsWith(sFontStylePrefix)) {
                            QString fontStyle = styleToken.mid(
                                        sFontStylePrefix.length(),
                                        styleToken.length() - sFontStylePrefix.length());
                            font.setItalic(fontStyle.contains("italic"));
                        }
                    }
                }

                QTextCursor curCursor = textItem->textCursor();
                QTextCharFormat format;

                format.setFont(font);
                curCursor.mergeCharFormat(format);
                textItem->setTextCursor(curCursor);
                textItem->setFont(font);

                QStringView fill = mXmlReader.attributes().value("color");
                if (!fill.isNull()) {
                    QColor textColor;
                    textColor.setNamedColor(fill.toString());
                    textItem->setDefaultTextColor(textColor);
                }

                while (!(mXmlReader.isEndElement() && mXmlReader.name() == "font")) {
                    if (mXmlReader.hasError()) {
                        break;
                    }
                    QXmlStreamReader::TokenType tt = mXmlReader.readNext();
                    if (tt == QXmlStreamReader::Characters) {
                        text += mXmlReader.text().toString();
                    }

                    if (mXmlReader.isStartElement() && mXmlReader.name() == "br") {
                        text += "\n";
                    }
                }
            }
        }
    }

    if (text.isEmpty()) {
        delete textItem;
        textItem = 0;
    } else {
        textItem->setPlainText(text);
        textItem->resize(width, height);
    }

    textItem->resize(width, height);

    return textItem;
}




void SvgSubsetAdaptor::SvgSubsetWriter::curtainItemToSvg(GraphicsCurtainItem* curtainItem)
{
    /**
     *
     * sample
     *
      <tb:curtain x="250" y="150" width="122" height="67"...>
      </tb:curtain>
     */

    mXmlWriter.writeStartElement(Settings::documentNamespaceUri, "curtain");
    mXmlWriter.writeAttribute("x", QString("%1").arg(curtainItem->boundingRect().center().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(curtainItem->boundingRect().center().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(curtainItem->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(curtainItem->boundingRect().height()));
    mXmlWriter.writeAttribute("transform", toSvgTransform(curtainItem->sceneTransform()));

    //graphicsItemToSvg(curtainItem);
    QString zs;
    zs.setNum(curtainItem->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "z-value", zs);

    Item* boardItem = dynamic_cast<Item*>(curtainItem);

    if (boardItem)
    {
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "uuid", StringUtils::toCanonicalUuid(boardItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}


GraphicsCurtainItem* SvgSubsetAdaptor::SvgSubsetReader::curtainItemFromSvg()
{
    GraphicsCurtainItem* curtainItem = new GraphicsCurtainItem();

    graphicsItemFromSvg(curtainItem);

    QStringView svgX = mXmlReader.attributes().value("x");
    QStringView svgY = mXmlReader.attributes().value("y");
    QStringView svgWidth = mXmlReader.attributes().value("width");
    QStringView svgHeight = mXmlReader.attributes().value("height");


    QRect rect;
    rect.setX(svgX.toString().toFloat()-svgWidth.toString().toFloat()/2);
    rect.setY(svgY.toString().toFloat()-svgHeight.toString().toFloat()/2);
    rect.setWidth(svgWidth.toString().toFloat());
    rect.setHeight(svgHeight.toString().toFloat());

    curtainItem->setRect(rect);

    curtainItem->setVisible(true);

    return curtainItem;
}


void SvgSubsetAdaptor::SvgSubsetWriter::rulerToSvg(GraphicsRuler* item)
{

    /**
     *
     * sample
     *
      <tb:ruler x="250" y="150" width="122" height="67"...>
      </tb:ruler>
     */

    mXmlWriter.writeStartElement(Settings::documentNamespaceUri, "ruler");
    mXmlWriter.writeAttribute("x", QString("%1").arg(item->boundingRect().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(item->boundingRect().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->boundingRect().height()));
    mXmlWriter.writeAttribute("transform", toSvgTransform(item->sceneTransform()));

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "z-value", zs);

    Item* boardItem = dynamic_cast<Item*>(item);

    if (boardItem)
    {
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "uuid", StringUtils::toCanonicalUuid(boardItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}


GraphicsRuler* SvgSubsetAdaptor::SvgSubsetReader::rulerFromSvg()
{
    GraphicsRuler* ruler = new GraphicsRuler();

    graphicsItemFromSvg(ruler);

    ruler->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    QStringView svgWidth = mXmlReader.attributes().value("width");
    QStringView svgHeight = mXmlReader.attributes().value("height");
    QStringView svgX = mXmlReader.attributes().value("x");
    QStringView svgY = mXmlReader.attributes().value("y");

    if (!svgWidth.isNull() && !svgHeight.isNull() && !svgX.isNull() && !svgY.isNull())
    {
        ruler->setRect(svgX.toString().toFloat(), svgY.toString().toFloat(),  svgWidth.toString().toFloat(), svgHeight.toString().toFloat());
    }

    ruler->setVisible(true);

    return ruler;
}


void SvgSubsetAdaptor::SvgSubsetWriter::compassToSvg(GraphicsCompass* item)
{

    /**
     *
     * sample
     *
      <tb:compass x="250" y="150" width="122" height="67"...>
      </tb:compass>
     */

    mXmlWriter.writeStartElement(Settings::documentNamespaceUri, "compass");
    mXmlWriter.writeAttribute("x", QString("%1").arg(item->boundingRect().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(item->boundingRect().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->boundingRect().height()));
    mXmlWriter.writeAttribute("transform", toSvgTransform(item->sceneTransform()));

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "z-value", zs);

    Item* boardItem = dynamic_cast<Item*>(item);

    if (boardItem)
    {
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "uuid", StringUtils::toCanonicalUuid(boardItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}


GraphicsCompass* SvgSubsetAdaptor::SvgSubsetReader::compassFromSvg()
{
    GraphicsCompass* compass = new GraphicsCompass();

    graphicsItemFromSvg(compass);

    compass->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    QStringView svgX = mXmlReader.attributes().value("x");
    QStringView svgY = mXmlReader.attributes().value("y");
    QStringView svgWidth = mXmlReader.attributes().value("width");
    QStringView svgHeight = mXmlReader.attributes().value("height");

    if (!svgX.isNull() && !svgY.isNull() && !svgWidth.isNull() && !svgHeight.isNull())
    {
        compass->setRect(svgX.toString().toFloat(), svgY.toString().toFloat()
                         , svgWidth.toString().toFloat(), svgHeight.toString().toFloat());
    }

    compass->setVisible(true);

    return compass;
}


void SvgSubsetAdaptor::SvgSubsetWriter::protractorToSvg(GraphicsProtractor* item)
{

    /**
     *
     * sample
     *
      <tb:protractor x="250" y="150" width="122" height="67"...>
      </tb:protractor>
     */

    mXmlWriter.writeStartElement(Settings::documentNamespaceUri, "protractor");

    mXmlWriter.writeAttribute("x", QString("%1").arg(item->rect().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(item->rect().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->rect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->rect().height()));
    mXmlWriter.writeAttribute("transform", toSvgTransform(item->sceneTransform()));

    QString angle;
    angle.setNum(item->angle(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "angle", angle);
    angle.setNum(item->markerAngle(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "marker-angle", angle);

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "z-value", zs);

    Item* boardItem = dynamic_cast<Item*>(item);

    if (boardItem)
    {
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "uuid", StringUtils::toCanonicalUuid(boardItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}


GraphicsProtractor* SvgSubsetAdaptor::SvgSubsetReader::protractorFromSvg()
{
    GraphicsProtractor* protractor = new GraphicsProtractor();

    protractor->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    graphicsItemFromSvg(protractor);

    QStringView angle = mXmlReader.attributes().value(mNamespaceUri, "angle");
    if (!angle.isNull())
    {
        protractor->setAngle(angle.toString().toFloat());
    }

    QStringView markerAngle = mXmlReader.attributes().value(mNamespaceUri, "marker-angle");
    if (!markerAngle.isNull())
    {
        protractor->setMarkerAngle(markerAngle.toString().toFloat());
    }

    QStringView svgX = mXmlReader.attributes().value("x");
    QStringView svgY = mXmlReader.attributes().value("y");
    QStringView svgWidth = mXmlReader.attributes().value("width");
    QStringView svgHeight = mXmlReader.attributes().value("height");

    if (!svgX.isNull() && !svgY.isNull() && !svgWidth.isNull() && !svgHeight.isNull())
    {
        protractor->setRect(svgX.toString().toFloat(), svgY.toString().toFloat()
                            , svgWidth.toString().toFloat(), svgHeight.toString().toFloat());
    }

    protractor->setVisible(true);

    return protractor;
}

void SvgSubsetAdaptor::SvgSubsetWriter::triangleToSvg(GraphicsTriangle *item)
{

    /**
     *
     * sample
     *
      <tb:triangle x="250" y="150" width="122" height="67"...>
      </tb:triangle>
     */

    mXmlWriter.writeStartElement(Settings::documentNamespaceUri, "triangle");
    mXmlWriter.writeAttribute("x", QString("%1").arg(item->boundingRect().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(item->boundingRect().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->boundingRect().height()));
    mXmlWriter.writeAttribute("transform", toSvgTransform(item->sceneTransform()));
    mXmlWriter.writeAttribute("orientation", GraphicsTriangle::orientationToStr(item->getOrientation()));

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "z-value", zs);

    Item* boardItem = dynamic_cast<Item*>(item);

    if (boardItem)
    {
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "uuid", StringUtils::toCanonicalUuid(boardItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}

GraphicsTriangle* SvgSubsetAdaptor::SvgSubsetReader::triangleFromSvg()
{
    GraphicsTriangle* triangle = new GraphicsTriangle();

    triangle->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    graphicsItemFromSvg(triangle);

    QStringView svgX = mXmlReader.attributes().value("x");
    QStringView svgY = mXmlReader.attributes().value("y");
    QStringView svgWidth = mXmlReader.attributes().value("width");
    QStringView svgHeight = mXmlReader.attributes().value("height");

    QStringView orientationStringRef = mXmlReader.attributes().value("orientation");
    GraphicsTriangle::Orientation orientation = GraphicsTriangle::orientationFromStr(orientationStringRef);
    triangle->setOrientation(orientation);

    if (!svgX.isNull() && !svgY.isNull() && !svgWidth.isNull() && !svgHeight.isNull())
    {
        triangle->setRect(svgX.toString().toFloat(), svgY.toString().toFloat(), svgWidth.toString().toFloat(), svgHeight.toString().toFloat(), orientation);
    }

    triangle->setVisible(true);
    return triangle;
}

GraphicsCache* SvgSubsetAdaptor::SvgSubsetReader::cacheFromSvg()
{
    GraphicsCache* pCache = GraphicsCache::instance(mScene);
    pCache->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    graphicsItemFromSvg(pCache);

    QStringView colorR = mXmlReader.attributes().value("colorR");
    QStringView colorG = mXmlReader.attributes().value("colorG");
    QStringView colorB = mXmlReader.attributes().value("colorB");
    QStringView colorA = mXmlReader.attributes().value("colorA");
    QStringView shape = mXmlReader.attributes().value("shape");
    QStringView shapeSize = mXmlReader.attributes().value("shapeSize");

    QColor color(colorR.toString().toInt(), colorG.toString().toInt(), colorB.toString().toInt(), colorA.toString().toInt());

    pCache->setMaskColor(color);
    pCache->setShapeWidth(shapeSize.toString().toInt());
    pCache->setMaskShape(static_cast<eMaskShape>(shape.toString().toInt()));

    pCache->setVisible(true);

    return pCache;
}

void SvgSubsetAdaptor::SvgSubsetWriter::cacheToSvg(GraphicsCache* item)
{
    mXmlWriter.writeStartElement(Settings::documentNamespaceUri, "cache");

    mXmlWriter.writeAttribute("x", QString("%1").arg(item->rect().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(item->rect().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->rect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->rect().height()));
    mXmlWriter.writeAttribute("colorR", QString("%1").arg(item->maskColor().red()));
    mXmlWriter.writeAttribute("colorG", QString("%1").arg(item->maskColor().green()));
    mXmlWriter.writeAttribute("colorB", QString("%1").arg(item->maskColor().blue()));
    mXmlWriter.writeAttribute("colorA", QString("%1").arg(item->maskColor().alpha()));
    mXmlWriter.writeAttribute("shape", QString("%1").arg(item->maskshape()));
    mXmlWriter.writeAttribute("shapeSize", QString("%1").arg(item->shapeWidth()));

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "z-value", zs);

    Item* boardItem = dynamic_cast<Item*>(item);

    if (boardItem)
    {
        mXmlWriter.writeAttribute(Settings::documentNamespaceUri, "uuid", StringUtils::toCanonicalUuid(boardItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}

void SvgSubsetAdaptor::convertPDFObjectsToImages(DocumentProxy* proxy)
{
    for (int i = 0; i < proxy->pageCount(); i++)
    {
        GraphicsScene* scene = loadScene(proxy, i);

        if (scene)
        {
            bool foundPDFItem = false;

            foreach(QGraphicsItem* item, scene->items())
            {
                GraphicsPDFItem *pdfItem = dynamic_cast<GraphicsPDFItem*>(item);

                if (pdfItem)
                {
                    foundPDFItem = true;
                    GraphicsPixmapItem* pixmapItem = pdfItem->toPixmapItem();

                    scene->removeItem(pdfItem);
                    scene->addItem(pixmapItem);

                }
            }

            if (foundPDFItem)
            {
                scene->setModified(true);
                persistScene(proxy, scene, i);
            }
        }

    }
}


void SvgSubsetAdaptor::convertSvgImagesToImages(DocumentProxy* proxy)
{
    for (int i = 0; i < proxy->pageCount(); i++)
    {
        GraphicsScene* scene = loadScene(proxy, i);

        if (scene)
        {
            bool foundSvgItem = false;

            foreach(QGraphicsItem* item, scene->items())
            {
                GraphicsSvgItem *svgItem = dynamic_cast<GraphicsSvgItem*>(item);

                if (svgItem)
                {
                    foundSvgItem = true;
                    GraphicsPixmapItem* pixmapItem = svgItem->toPixmapItem();

                    scene->removeItem(svgItem);
                    scene->addItem(pixmapItem);
                }
            }

            if (foundSvgItem)
            {
                scene->setModified(true);
                persistScene(proxy, scene, i);
            }
        }
    }
}
