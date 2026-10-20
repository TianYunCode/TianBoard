#include <QRegExp>
#include "core/VirtualDesktop.h"
#include <QSvgGenerator>
#include <QSvgRenderer>
#include <QPixmap>
#include <QMap>

#include "core/PersistenceManager.h"

#include "model/DocumentProxy.h"

#include "whiteboard/Item.h"
#include "whiteboard/GraphicsPolygonItem.h"
#include "whiteboard/GraphicsStroke.h"
#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsWidgetItem.h"
#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsTextItemDelegate.h"
#include "whiteboard/GraphicsWidgetItem.h"
#include "whiteboard/GraphicsGroupContainerItem.h"

#include "utils/FileSystemUtils.h"

#include "content/IwbSubsetAdaptor.h"
#include "content/MetadataDcSubsetAdaptor.h"
#include "content/ThumbnailAdaptor.h"
#include "content/SvgSubsetAdaptor.h"

#include "core/Application.h"
#include "QFile"


//tag names definition. Use them everiwhere!
static QString tElement = "element";
static QString tGroup = "group";
static QString tEllipse = "ellipse";
static QString tIwb = "iwb";
static QString tMeta = "meta";
static QString tPage = "page";
static QString tPageset = "pageset";
static QString tG = "g";
static QString tSwitch = "switch";
static QString tPolygon = "polygon";
static QString tPolyline = "polyline";
static QString tRect = "rect";
static QString tSvg = "svg";
static QString tText = "text";
static QString tTextarea = "textarea";
static QString tTspan = "tspan";
static QString tBreak = "tbreak";
static QString tImage = "image";
static QString tFlash = "flash";
static QString tAudio = "a";
static QString tVideo = "video";

//attribute names definition
static QString aFill = "fill";
static QString aFillopacity = "fill-opacity";
static QString aX = "x";
static QString aY = "y";
static QString aWidth = "width";
static QString aHeight = "height";
static QString aStroke = "stroke";
static QString aStrokewidth = "stroke-width";
static QString aCx = "cx";
static QString aCy = "cy";
static QString aRx = "rx";
static QString aRy = "ry";
static QString aTransform = "transform";
static QString aViewbox = "viewbox";
static QString aFontSize = "font-size";
static QString aFontfamily = "font-family";
static QString aFontstretch = "font-stretch";
static QString aFontstyle = "font-style";
static QString aFontweight = "font-weight";
static QString aTextalign = "text-align";
static QString aPoints = "points";
static QString svgNS = "http://www.w3.org/2000/svg";
static QString iwbNS = "http://www.imsglobal.org/xsd/iwb_v1p0";
static QString aId = "id";
static QString aRef = "ref";
static QString aHref = "href";
static QString aBackground = "background";
static QString aLocked = "locked";
static QString aEditable = "editable";

//attributes part names
static QString apRotate = "rotate";
static QString apTranslate = "translate";


IwbSubsetAdaptor::IwbSubsetAdaptor() {}

bool IwbSubsetAdaptor::convertIwbFileToDocumentPackage(QString &iwbSourceFile, DocumentProxy *pDocument) {
    //TODO
    // fill document proxy metadata
    // create persistance manager to save data using proxy
    // create IwbSubsetReader and make it parse iwbSourceFolder
    QFile file(iwbSourceFile);

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file " << iwbSourceFile << " for reading ...";
        return false;
    }

    IwbSubsetReader iwbReader(pDocument, &file);
    bool result = iwbReader.parse();
    file.close();

    return result;
}
IwbSubsetAdaptor::IwbSubsetReader::IwbSubsetReader(DocumentProxy *proxy, QFile *content)
    : mProxy(proxy), mGSectionContainer(NULL) {
    int errorLine, errorColumn;
    QString errorStr;
    if (!mDOMdoc.setContent(content, true, &errorStr, &errorLine, &errorColumn)) {
        qWarning() << "Error:Parseerroratline" << errorLine << ","
                   << "column" << errorColumn << ":" << errorStr;
    } else {
        qDebug() << "well parsed to DOM";
        pwdContent = QFileInfo(content->fileName()).dir().absolutePath();
    }
    qDebug() << "tmp path is" << pwdContent;
}
bool IwbSubsetAdaptor::IwbSubsetReader::parse() {
    MetadataDcSubsetAdaptor::persist(mProxy);

    mIndent = "";
    if (!getTempFileName() || !createTempFlashPath())
        return false;

    if (mDOMdoc.isNull())
        return false;

    bool result = parseDoc();
    if (result)
        result = mProxy->pageCount() != 0;

    if (QFile::exists(mTempFilePath))
        QFile::remove(mTempFilePath);

    //    if (mTmpFlashDir.exists())
    //        FileSystemUtils::deleteDir(mTmpFlashDir.path());

    return result;
}

bool IwbSubsetAdaptor::IwbSubsetReader::parseGSection(const QDomElement &element) {
    mGSectionContainer = new GraphicsGroupContainerItem();

    QDomElement currentSvgElement = element.firstChildElement();
    while (!currentSvgElement.isNull()) {
        parseSvgElement(currentSvgElement);
        currentSvgElement = currentSvgElement.nextSiblingElement();
    }

    if (mGSectionContainer->childItems().count()) {
        mCurrentScene->addGroup(mGSectionContainer);
    } else {
        delete mGSectionContainer;
    }
    mGSectionContainer = NULL;

    return true;
}
bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgSwitchSection(const QDomElement &element) {
    QDomElement currentSvgElement = element.firstChildElement();
    while (!currentSvgElement.isNull()) {
        if (parseSvgElement(currentSvgElement))
            return true;
        // 这个分支解析失败（比如引用的媒体文件缺失）要继续试下一个候选分支；
        // 不推进游标会在同一个元素上无限重试，导入直接卡死主线程。
        currentSvgElement = currentSvgElement.nextSiblingElement();
    }

    return false;
}

bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgRect(const QDomElement &element) {
    qreal x1 = element.attribute(aX).toDouble();
    qreal y1 = element.attribute(aY).toDouble();
    //rect dimensions
    qreal width = element.attribute(aWidth).toDouble();
    qreal height = element.attribute(aHeight).toDouble();

    QString textFillColor = element.attribute(aFill);
    QString textStrokeColor = element.attribute(aStroke);
    QString textStrokeWidth = element.attribute(aStrokewidth);

    QColor fillColor = !textFillColor.isNull() ? colorFromString(textFillColor) : QColor();
    QColor strokeColor = !textStrokeColor.isNull() ? colorFromString(textStrokeColor) : QColor();
    int strokeWidth = textStrokeWidth.toInt();

    x1 -= strokeWidth / 2;
    y1 -= strokeWidth / 2;
    width += strokeWidth;
    height += strokeWidth;

    //init svg generator with temp file
    QSvgGenerator *generator = createSvgGenerator(width, height);

    //init painter to paint to svg
    QPainter painter;

    painter.begin(generator);

    //fill rect
    if (fillColor.isValid()) {
        painter.setBrush(QBrush(fillColor));
        painter.fillRect(0, 0, width, height, fillColor);
    }
    QPen pen;
    if (strokeColor.isValid()) {
        pen.setColor(strokeColor);
    }
    if (strokeWidth)
        pen.setWidth(strokeWidth);
    painter.setPen(pen);
    painter.drawRect(0, 0, width, height);

    painter.end();

    GraphicsSvgItem *svgItem = mCurrentScene->addSvg(QUrl::fromLocalFile(generator->fileName()));

    QString uuid = QUuid::createUuid().toString();
    mRefToUuidMap.insert(element.attribute(aId), uuid);
    svgItem->setUuid(QUuid(uuid));

    QTransform transform;
    QString textTransform = element.attribute(aTransform);

    svgItem->resetTransform();
    if (!textTransform.isNull()) {
        transform = transformFromString(textTransform, svgItem);
    }

    repositionSvgItem(svgItem, width, height, x1, y1, transform);
    hashSceneItem(element, svgItem);

    if (mGSectionContainer) {
        addItemToGSection(svgItem);
    }

    delete generator;

    return true;
}
bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgEllipse(const QDomElement &element) {
    //ellipse horisontal and vertical radius
    qreal rx = element.attribute(aRx).toDouble();
    qreal ry = element.attribute(aRy).toDouble();
    QSvgGenerator *generator = createSvgGenerator(rx * 2, ry * 2);

    //fill and stroke color
    QColor fillColor = colorFromString(element.attribute(aFill));
    QColor strokeColor = colorFromString(element.attribute(aStroke));
    int strokeWidth = element.attribute(aStrokewidth).toInt();

    //ellipse center coordinates
    qreal cx = element.attribute(aCx).toDouble();
    qreal cy = element.attribute(aCy).toDouble();

    //init painter to paint to svg
    QPainter painter;
    painter.begin(generator);

    QPen pen(strokeColor);
    pen.setWidth(strokeWidth);
    painter.setPen(pen);
    painter.setBrush(QBrush(fillColor));

    painter.drawEllipse(0, 0, rx * 2, ry * 2);

    painter.end();

    GraphicsSvgItem *svgItem = mCurrentScene->addSvg(QUrl::fromLocalFile(generator->fileName()));

    QString uuid = QUuid::createUuid().toString();
    mRefToUuidMap.insert(element.attribute(aId), uuid);
    svgItem->setUuid(QUuid(uuid));

    QTransform transform;
    QString textTransform = element.attribute(aTransform);

    svgItem->resetTransform();
    if (!textTransform.isNull()) {
        transform = transformFromString(textTransform, svgItem);
    }


    repositionSvgItem(svgItem, rx * 2, ry * 2, cx - 2 * rx, cy + ry, transform);
    hashSceneItem(element, svgItem);

    if (mGSectionContainer) {
        addItemToGSection(svgItem);
    }

    delete generator;

    return true;
}
bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgPolygon(const QDomElement &element) {
    QString svgPoints = element.attribute(aPoints);
    QPolygonF polygon;

    if (!svgPoints.isNull()) {
        QStringList ts = svgPoints.split(QLatin1Char(' '), Qt::SkipEmptyParts);

        foreach (const QString sPoint, ts) {
            QStringList sCoord = sPoint.split(QLatin1Char(','), Qt::SkipEmptyParts);
            if (sCoord.size() == 2) {
                QPointF point;
                point.setX(sCoord.at(0).toFloat());
                point.setY(sCoord.at(1).toFloat());
                polygon << point;
            } else if (sCoord.size() == 4) {
                //This is the case on system were the "," is used to seperate decimal
                QPointF point;
                QString x = sCoord.at(0) + "." + sCoord.at(1);
                QString y = sCoord.at(2) + "." + sCoord.at(3);
                point.setX(x.toFloat());
                point.setY(y.toFloat());
                polygon << point;
            } else {
                qDebug() << "cannot make sense of a 'point' value" << sCoord;
            }
        }
    }

    //bounding rect lef top corner coordinates
    qreal x1 = polygon.boundingRect().topLeft().x();
    qreal y1 = polygon.boundingRect().topLeft().y();
    //bounding rect dimensions
    qreal width = polygon.boundingRect().width();
    qreal height = polygon.boundingRect().height();

    QString strokeColorText = element.attribute(aStroke);
    QString fillColorText = element.attribute(aFill);
    QString strokeWidthText = element.attribute(aStrokewidth);

    QColor strokeColor = !strokeColorText.isEmpty() ? colorFromString(strokeColorText) : QColor();
    QColor fillColor = !fillColorText.isEmpty() ? colorFromString(fillColorText) : QColor();
    int strokeWidth = strokeWidthText.toDouble();

    QPen pen;
    pen.setColor(strokeColor);
    pen.setWidth(strokeWidth);

    QBrush brush;
    brush.setColor(fillColor);
    brush.setStyle(Qt::SolidPattern);


    QUuid itemUuid(element.attribute(aId).right(QUuid().toString().length()));
    QUuid itemGroupUuid(element.attribute(aId).left(QUuid().toString().length() - 1));
    if (!itemUuid.isNull() && (itemGroupUuid != itemUuid)) // reimported from a .tbz document
    {
        GraphicsPolygonItem *graphicsPolygon = mCurrentScene->polygonToPolygonItem(polygon);

        graphicsPolygon->setBrush(brush);

        QTransform transform;
        QString textTransform = element.attribute(aTransform);

        graphicsPolygon->resetTransform();
        if (!textTransform.isNull()) {
            transform = transformFromString(textTransform, graphicsPolygon);
        }
        mCurrentScene->addItem(graphicsPolygon);

        graphicsPolygon->setUuid(itemUuid);
        mRefToUuidMap.insert(element.attribute(aId), itemUuid.toString());

    } else // single IWB
    {
        QSvgGenerator *generator = createSvgGenerator(width + pen.width(), height + pen.width());
        QPainter painter;

        painter.begin(generator); //drawing to svg tmp file

        painter.translate(pen.widthF() / 2 - x1, pen.widthF() / 2 - y1);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawPolygon(polygon);

        painter.end();

        //add resulting svg file to scene
        GraphicsSvgItem *svgItem = mCurrentScene->addSvg(QUrl::fromLocalFile(generator->fileName()));
        QTransform transform;
        QString textTransform = element.attribute(aTransform);

        QUuid uuid = QUuid::createUuid();
        mRefToUuidMap.insert(element.attribute(aId), uuid.toString());
        svgItem->setUuid(uuid);

        svgItem->resetTransform();
        if (!textTransform.isNull()) {
            transform = transformFromString(textTransform, svgItem);
        }
        repositionSvgItem(svgItem, width + strokeWidth, height + strokeWidth,
                          x1 - strokeWidth / 2 + transform.m31(), y1 + strokeWidth / 2 + transform.m32(),
                          transform);
        hashSceneItem(element, svgItem);

        if (mGSectionContainer) {
            addItemToGSection(svgItem);
        }

        delete generator;
    }
    return true;
}
bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgPolyline(const QDomElement &element) {
    QString svgPoints = element.attribute(aPoints);
    QPolygonF polygon;

    if (!svgPoints.isNull()) {
        QStringList ts = svgPoints.split(QLatin1Char(' '), Qt::SkipEmptyParts);

        foreach (const QString sPoint, ts) {
            QStringList sCoord = sPoint.split(QLatin1Char(','), Qt::SkipEmptyParts);
            if (sCoord.size() == 2) {
                QPointF point;
                point.setX(sCoord.at(0).toFloat());
                point.setY(sCoord.at(1).toFloat());
                polygon << point;
            } else if (sCoord.size() == 4) {
                //This is the case on system were the "," is used to seperate decimal
                QPointF point;
                QString x = sCoord.at(0) + "." + sCoord.at(1);
                QString y = sCoord.at(2) + "." + sCoord.at(3);
                point.setX(x.toFloat());
                point.setY(y.toFloat());
                polygon << point;
            } else {
                qDebug() << "cannot make sense of a 'point' value" << sCoord;
            }
        }
    }

    //bounding rect lef top corner coordinates
    qreal x1 = polygon.boundingRect().topLeft().x();
    qreal y1 = polygon.boundingRect().topLeft().y();

    //bounding rect dimensions
    qreal width = polygon.boundingRect().width();
    qreal height = polygon.boundingRect().height();

    QString strokeColorText = element.attribute(aStroke);
    QString strokeWidthText = element.attribute(aStrokewidth);

    QColor strokeColor = !strokeColorText.isEmpty() ? colorFromString(strokeColorText) : QColor();
    int strokeWidth = strokeWidthText.toDouble();

    width += strokeWidth;
    height += strokeWidth;

    QPen pen;
    pen.setColor(strokeColor);
    pen.setWidth(strokeWidth);

    QBrush brush;
    brush.setColor(strokeColor);
    brush.setStyle(Qt::SolidPattern);

    QUuid itemUuid(element.attribute(aId).right(QUuid().toString().length()));
    QUuid itemGroupUuid(element.attribute(aId).left(QUuid().toString().length() - 1));
    if (!itemUuid.isNull() && (itemGroupUuid != itemUuid)) // reimported from a .tbz document
    {
        GraphicsPolygonItem *graphicsPolygon = new GraphicsPolygonItem(polygon);

        GraphicsStroke *stroke = new GraphicsStroke();
        graphicsPolygon->setStroke(stroke);

        graphicsPolygon->setBrush(brush);
        QTransform transform;
        QString textTransform = element.attribute(aTransform);

        graphicsPolygon->resetTransform();
        if (!textTransform.isNull()) {
            transform = transformFromString(textTransform, graphicsPolygon);
        }
        mCurrentScene->addItem(graphicsPolygon);

        graphicsPolygon->setUuid(itemUuid);
        mRefToUuidMap.insert(element.attribute(aId), itemUuid.toString());

    } else // simple IWB
    {
        QSvgGenerator *generator = createSvgGenerator(width + pen.width(), height + pen.width());
        QPainter painter;

        painter.begin(generator); //drawing to svg tmp file

        painter.translate(pen.widthF() / 2 - x1, pen.widthF() / 2 - y1);
        painter.setBrush(brush);
        painter.setPen(pen);
        painter.drawPolygon(polygon);

        painter.end();

        //add resulting svg file to scene
        GraphicsSvgItem *svgItem = mCurrentScene->addSvg(QUrl::fromLocalFile(generator->fileName()));

        QString uuid = QUuid::createUuid().toString();
        mRefToUuidMap.insert(element.attribute(aId), uuid);
        svgItem->setUuid(QUuid(uuid));

        QTransform transform;
        QString textTransform = element.attribute(aTransform);

        svgItem->resetTransform();
        if (!textTransform.isNull()) {
            transform = transformFromString(textTransform, svgItem);
        }
        repositionSvgItem(svgItem, width + strokeWidth, height + strokeWidth,
                          x1 - strokeWidth / 2 + transform.m31(), y1 + strokeWidth / 2 + transform.m32(),
                          transform);
        hashSceneItem(element, svgItem);

        if (mGSectionContainer) {
            addItemToGSection(svgItem);
        }

        delete generator;
    }


    return true;
}
void IwbSubsetAdaptor::IwbSubsetReader::parseTextAttributes(const QDomElement &element, qreal &fontSize,
                                                            QColor &fontColor, QString &fontFamily,
                                                            QString &fontStretch, bool &italic,
                                                            int &fontWeight, int &textAlign,
                                                            QTransform &fontTransform) {
    //consider inch has 72 liens
    //since svg font size is given in pixels, divide it by pixels per line
    QString fontSz = element.attribute(aFontSize);
    if (!fontSz.isNull())
        fontSize = fontSz.toDouble() * 72 / VirtualDesktop::instance()->physicalDpi();

    QString fontColorText = element.attribute(aFill);
    if (!fontColorText.isNull())
        fontColor = colorFromString(fontColorText);

    QString fontFamilyText = element.attribute(aFontfamily);
    if (!fontFamilyText.isNull())
        fontFamily = fontFamilyText;

    QString fontStretchText = element.attribute(aFontstretch);
    if (!fontStretchText.isNull())
        fontStretch = fontStretchText;

    if (!element.attribute(aFontstyle).isNull())
        italic = (element.attribute(aFontstyle) == "italic");

    QString weight = element.attribute(aFontweight);
    if (!weight.isNull()) {
        if (weight == "normal")
            fontWeight = QFont::Normal;
        else if (weight == "light")
            fontWeight = QFont::Light;
        else if (weight == "demibold")
            fontWeight = QFont::DemiBold;
        else if (weight == "bold")
            fontWeight = QFont::Bold;
        else if (weight == "black")
            fontWeight = QFont::Black;
    }
    QString align = element.attribute(aTextalign);
    if (!align.isNull()) {
        if (align == "middle" || align == "center")
            textAlign = Qt::AlignHCenter;
        else if (align == "start")
            textAlign = Qt::AlignLeft;
        else if (align == "end")
            textAlign = Qt::AlignRight;
    }

    if (!element.attribute(aTransform).isNull())
        fontTransform = transformFromString(element.attribute(aTransform));
}
void IwbSubsetAdaptor::IwbSubsetReader::readTextBlockAttr(const QDomElement &element,
                                                          QTextBlockFormat &format) {
    QString fontStretchText = element.attribute(aFontstretch);
    if (!fontStretchText.isNull())
        format.setAlignment(Qt::AlignJustify);

    QString align = element.attribute(aTextalign);
    if (!align.isNull()) {
        if (align == "middle" || align == "center")
            format.setAlignment(Qt::AlignHCenter);
        else if (align == "start")
            format.setAlignment(Qt::AlignLeft);
        else if (align == "end")
            format.setAlignment(Qt::AlignRight);
        else if (align == "justify")
            format.setAlignment(Qt::AlignJustify);
    }
}
void IwbSubsetAdaptor::IwbSubsetReader::readTextCharAttr(const QDomElement &element,
                                                         QTextCharFormat &format) {
    QString fontSz = element.attribute(aFontSize);
    if (!fontSz.isNull()) {
        qreal fontSize = fontSz.remove("pt").toDouble();
        format.setFontPointSize(fontSize);
    }
    QString fontColorText = element.attribute(aFill);
    if (!fontColorText.isNull()) {
        QColor fontColor = colorFromString(fontColorText);
        if (fontColor.isValid())
            format.setForeground(fontColor);
    }
    QString fontFamilyText = element.attribute(aFontfamily);
    if (!fontFamilyText.isNull()) {
        format.setFontFamily(fontFamilyText);
    }
    if (!element.attribute(aFontstyle).isNull()) {
        bool italic = (element.attribute(aFontstyle) == "italic");
        format.setFontItalic(italic);
    }
    QString weight = element.attribute(aFontweight);
    if (!weight.isNull()) {
        if (weight == "normal")
            format.setFontWeight(QFont::Normal);
        else if (weight == "light")
            format.setFontWeight(QFont::Light);
        else if (weight == "demibold")
            format.setFontWeight(QFont::DemiBold);
        else if (weight == "bold")
            format.setFontWeight(QFont::Bold);
        else if (weight == "black")
            format.setFontWeight(QFont::Black);
    }
}

bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgText(const QDomElement &element) {
    qreal x = element.attribute(aX).toDouble();
    qreal y = element.attribute(aY).toDouble();
    qreal width = element.attribute(aWidth).toDouble();
    qreal height = element.attribute(aHeight).toDouble();


    qreal fontSize = 12;
    QColor fontColor(qApp->palette().color(QPalette::WindowText));
    QString fontFamily = "Arial";
    QString fontStretch = "normal";
    bool italic = false;
    int fontWeight = QFont::Normal;
    int textAlign = Qt::AlignLeft;
    QTransform fontTransform;
    parseTextAttributes(element, fontSize, fontColor, fontFamily, fontStretch, italic, fontWeight, textAlign,
                        fontTransform);

    QFont startFont(fontFamily, fontSize, fontWeight, italic);
    height = QFontMetrics(startFont).height();
    width = QFontMetrics(startFont).horizontalAdvance(element.text()) + 5;

    QSvgGenerator *generator = createSvgGenerator(width, height);
    QPainter painter;
    painter.begin(generator);
    painter.setFont(startFont);

    qreal curY = 0.0;
    qreal curX = 0.0;
    qreal linespacing = QFontMetricsF(painter.font()).leading();

    //    remember if text area has transform
    //    QString transformString;
    QTransform transform = fontTransform;

    QRectF lastDrawnTextBoundingRect;
    //parse text area tags

    //recursive call any tspan in text svg element
    parseTSpan(element, painter, curX, curY, width, height, linespacing, lastDrawnTextBoundingRect, fontSize,
               fontColor, fontFamily, fontStretch, italic, fontWeight, textAlign, fontTransform);

    painter.end();

    //add resulting svg file to scene
    GraphicsSvgItem *svgItem = mCurrentScene->addSvg(QUrl::fromLocalFile(generator->fileName()));

    QString uuid = QUuid::createUuid().toString();
    mRefToUuidMap.insert(element.attribute(aId), uuid);
    svgItem->setUuid(QUuid(uuid));

    svgItem->resetTransform();
    repositionSvgItem(svgItem, width, height, x + transform.m31(), y + transform.m32(), transform);
    hashSceneItem(element, svgItem);

    if (mGSectionContainer) {
        addItemToGSection(svgItem);
    }

    delete generator;
    return true;
}

void IwbSubsetAdaptor::IwbSubsetReader::parseTSpan(const QDomElement &parent, QPainter &painter, qreal &curX,
                                                   qreal &curY, qreal &width, qreal &height,
                                                   qreal &linespacing, QRectF &lastDrawnTextBoundingRect,
                                                   qreal &fontSize, QColor &fontColor, QString &fontFamily,
                                                   QString &fontStretch, bool &italic, int &fontWeight,
                                                   int &textAlign, QTransform &fontTransform) {
    QDomNode curNode = parent.firstChild();
    while (!curNode.isNull()) {
        if (curNode.toElement().tagName() == tTspan) {
            QDomElement curTSpan = curNode.toElement();
            parseTextAttributes(curTSpan, fontSize, fontColor, fontFamily, fontStretch, italic, fontWeight,
                                textAlign, fontTransform);
            painter.setFont(QFont(fontFamily, fontSize, fontWeight, italic));
            painter.setPen(fontColor);
            linespacing = QFontMetricsF(painter.font()).leading();
            parseTSpan(curTSpan, painter, curX, curY, width, height, linespacing, lastDrawnTextBoundingRect,
                       fontSize, fontColor, fontFamily, fontStretch, italic, fontWeight, textAlign,
                       fontTransform);
        } else if (curNode.nodeType() == QDomNode::CharacterDataNode ||
                   curNode.nodeType() == QDomNode::CDATASectionNode ||
                   curNode.nodeType() == QDomNode::TextNode) {
            QDomCharacterData textData = curNode.toCharacterData();
            QString text = textData.data().trimmed();
            //            width = painter.fontMetrics().horizontalAdvance(text);
            //get bounding rect to obtain desired text height
            lastDrawnTextBoundingRect = painter.boundingRect(QRectF(curX, curY, width, height - curY),
                                                             textAlign | Qt::TextWordWrap, text);
            painter.drawText(curX, curY, width, lastDrawnTextBoundingRect.height(),
                             textAlign | Qt::TextWordWrap, text);
            curX += lastDrawnTextBoundingRect.x() + lastDrawnTextBoundingRect.width();
        } else if (curNode.nodeType() == QDomNode::ElementNode && curNode.toElement().tagName() == tBreak) {
            curY += lastDrawnTextBoundingRect.height() + linespacing;
            curX = 0.0;
            lastDrawnTextBoundingRect = QRectF(0, 0, 0, 0);
        }
        curNode = curNode.nextSibling();
    }
}
void IwbSubsetAdaptor::IwbSubsetReader::parseTSpan(const QDomElement &element, QTextCursor &cursor,
                                                   QTextBlockFormat &blockFormat,
                                                   QTextCharFormat &charFormat) {
    QDomNode curNode = element.firstChild();
    while (!curNode.isNull()) {
        if (curNode.toElement().tagName() == tTspan) {
            QDomElement curTspan = curNode.toElement();
            readTextBlockAttr(curTspan, blockFormat);
            readTextCharAttr(curTspan, charFormat);
            cursor.setBlockFormat(blockFormat);
            cursor.setCharFormat(charFormat);
            parseTSpan(curTspan, cursor, blockFormat, charFormat);

        } else if (curNode.nodeType() == QDomNode::CharacterDataNode ||
                   curNode.nodeType() == QDomNode::CDATASectionNode ||
                   curNode.nodeType() == QDomNode::TextNode) {
            QDomCharacterData textData = curNode.toCharacterData();
            QString text = textData.data().trimmed();
            cursor.insertText(text, charFormat);

        } else if (curNode.nodeType() == QDomNode::ElementNode && curNode.toElement().tagName() == tBreak) {
            cursor.insertBlock();
        }
        curNode = curNode.nextSibling();
    }
}

bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgTextarea(const QDomElement &element) {
    qreal x = element.attribute(aX).toDouble();
    qreal y = element.attribute(aY).toDouble();
    qreal width = element.attribute(aWidth).toDouble();
    qreal height = element.attribute(aHeight).toDouble();

    QTextBlockFormat blockFormat;
    blockFormat.setAlignment(Qt::AlignLeft);

    QTextCharFormat textFormat;
    // default values
    textFormat.setFontPointSize(12);
    textFormat.setForeground(qApp->palette().color(QPalette::WindowText));
    textFormat.setFontFamily("Arial");
    textFormat.setFontItalic(false);
    textFormat.setFontWeight(QFont::Normal);

    // readed values
    readTextBlockAttr(element, blockFormat);
    readTextCharAttr(element, textFormat);

    QTextDocument doc;
    doc.setPlainText("");
    QTextCursor tCursor(&doc);
    tCursor.setBlockFormat(blockFormat);
    tCursor.setCharFormat(textFormat);

    parseTSpan(element, tCursor, blockFormat, textFormat);

    GraphicsTextItem *svgItem = mCurrentScene->addTextHtml(doc.toHtml());
    svgItem->resize(width, height);

    QString uuid = QUuid::createUuid().toString();
    mRefToUuidMap.insert(element.attribute(aId), uuid);
    svgItem->setUuid(QUuid(uuid));

    QTransform transform;
    QString textTransform = element.attribute(aTransform);

    svgItem->resetTransform();
    if (!textTransform.isNull()) {
        transform = transformFromString(textTransform, svgItem);
    }

    //by default all the textAreas are not editable
    GraphicsTextItemDelegate *curDelegate = dynamic_cast<GraphicsTextItemDelegate *>(svgItem->Delegate());
    if (curDelegate) {
        curDelegate->setEditable(false);
    }

    repositionSvgItem(svgItem, width, height, x + transform.m31(), y + transform.m32(), transform);
    hashSceneItem(element, svgItem);

    if (mGSectionContainer) {
        addItemToGSection(svgItem);
    }

    return true;
}
bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgImage(const QDomElement &element) {
    qreal x = element.attribute(aX).toDouble();
    qreal y = element.attribute(aY).toDouble();
    qreal width = element.attribute(aWidth).toDouble();
    qreal height = element.attribute(aHeight).toDouble();

    QString itemRefPath = element.attribute(aHref);

    QPixmap pix;
    if (!itemRefPath.isNull()) {
        QString imagePath = pwdContent + "/" + itemRefPath;
        if (!QFile::exists(imagePath)) {
            qDebug() << "can't load file" << pwdContent + "/" + itemRefPath << "maybe file corrupted";
            return false;
        } else {
            //            qDebug() << "size of file" << itemRefPath << QFileInfo(itemRefPath).size();
        }
        pix.load(imagePath);
        if (pix.isNull()) {
            qDebug() << "can't create pixmap for file" << pwdContent + "/" + itemRefPath
                     << "maybe format does not supported";
        }
    }

    GraphicsPixmapItem *pixItem = mCurrentScene->addPixmap(pix, NULL);

    QString uuid = QUuid::createUuid().toString();
    mRefToUuidMap.insert(element.attribute(aId), uuid);
    pixItem->setUuid(QUuid(uuid));

    QTransform transform;
    QString textTransform = element.attribute(aTransform);

    pixItem->resetTransform();
    if (!textTransform.isNull()) {
        transform = transformFromString(textTransform, pixItem);
    }
    repositionSvgItem(pixItem, width, height, x + transform.m31(), y + transform.m32(), transform);
    hashSceneItem(element, pixItem);

    if (mGSectionContainer) {
        addItemToGSection(pixItem);
    }

    return true;
}
bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgFlash(const QDomElement &element) {
    QString itemRefPath = element.attribute(aHref);

    qreal x = element.attribute(aX).toDouble();
    qreal y = element.attribute(aY).toDouble();
    qreal width = element.attribute(aWidth).toDouble();
    qreal height = element.attribute(aHeight).toDouble();

    QUrl urlPath;
    QString flashPath;
    if (!itemRefPath.isNull()) {
        flashPath = pwdContent + "/" + itemRefPath;
        if (!QFile::exists(flashPath)) {
            qDebug() << "can't load file" << pwdContent + "/" + itemRefPath << "maybe file corrupted";
            return false;
        }
        urlPath = QUrl::fromLocalFile(flashPath);
    }
    QDir tmpFlashDir(mTmpFlashDir);
    if (!tmpFlashDir.exists()) {
        qDebug() << "Can't create temporary directory to put flash";
        return false;
    }

    QString flashUrl = GraphicsWebWidgetItem::createNPAPIWrapperInDir(
        flashPath, tmpFlashDir, "application/x-shockwave-flash",
        QSize(mCurrentSceneRect.width(), mCurrentSceneRect.height()));
    GraphicsWidgetItem *flashItem = mCurrentScene->addWebWidget(QUrl::fromLocalFile(flashUrl));
    flashItem->setSourceUrl(urlPath);

    QString uuid = QUuid::createUuid().toString();
    mRefToUuidMap.insert(element.attribute(aId), uuid);
    flashItem->setUuid(QUuid(uuid));

    QTransform transform;
    QString textTransform = element.attribute(aTransform);

    flashItem->resetTransform();
    if (!textTransform.isNull()) {
        transform = transformFromString(textTransform, flashItem);
    }
    repositionSvgItem(flashItem, width, height, x + transform.m31(), y + transform.m32(), transform);
    hashSceneItem(element, flashItem);

    if (mGSectionContainer) {
        addItemToGSection(flashItem);
    }

    return true;
}

bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgAudio(const QDomElement &element) {
    QDomElement parentOfAudio = element.firstChild().toElement();

    qreal x = parentOfAudio.attribute(aX).toDouble();
    qreal y = parentOfAudio.attribute(aY).toDouble();

    QString itemRefPath = element.attribute(aHref);

    QUrl concreteUrl;
    if (!itemRefPath.isNull()) {
        QString audioPath = pwdContent + "/" + itemRefPath;
        if (!QFile::exists(audioPath)) {
            qDebug() << "can't load file" << pwdContent + "/" + itemRefPath << "maybe file corrupted";
            return false;
        }
        concreteUrl = QUrl::fromLocalFile(audioPath);
    }

    QString uuid = QUuid::createUuid().toString();
    mRefToUuidMap.insert(element.attribute(aId), uuid);

    QString destFile;
    bool b = PersistenceManager::persistenceManager()->addFileToDocument(
        mCurrentScene->document(), concreteUrl.toLocalFile(), PersistenceManager::audioDirectory, QUuid(uuid),
        destFile);
    if (!b) {
        return false;
    }
    concreteUrl = QUrl::fromLocalFile(destFile);

    GraphicsMediaItem *audioItem = mCurrentScene->addAudio(concreteUrl, false);

    QTransform transform;
    QString textTransform = parentOfAudio.attribute(aTransform);

    audioItem->resetTransform();
    if (!textTransform.isNull()) {
        transform = transformFromString(textTransform, audioItem);
    }
    repositionSvgItem(audioItem, audioItem->boundingRect().width(), audioItem->boundingRect().height(),
                      x + transform.m31(), y + transform.m32(), transform);
    hashSceneItem(element, audioItem);

    if (mGSectionContainer) {
        addItemToGSection(audioItem);
    }

    return true;
}
bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgVideo(const QDomElement &element) {
    QString itemRefPath = element.attribute(aHref);
    if (itemRefPath.startsWith(tFlash + "/") && itemRefPath.endsWith(".swf")) {
        if (parseSvgFlash(element))
            return true;
        else
            return false;
    }
    qreal x = element.attribute(aX).toDouble();
    qreal y = element.attribute(aY).toDouble();

    QUrl concreteUrl;
    if (!itemRefPath.isNull()) {
        QString videoPath = pwdContent + "/" + itemRefPath;
        if (!QFile::exists(videoPath)) {
            qDebug() << "can't load file" << pwdContent + "/" + itemRefPath << "maybe file corrupted";
            return false;
        }
        concreteUrl = QUrl::fromLocalFile(videoPath);
    }

    QString uuid = QUuid::createUuid().toString();
    mRefToUuidMap.insert(element.attribute(aId), uuid);

    QString destFile;
    bool b = PersistenceManager::persistenceManager()->addFileToDocument(
        mCurrentScene->document(), concreteUrl.toLocalFile(), PersistenceManager::videoDirectory, QUuid(uuid),
        destFile);
    if (!b) {
        return false;
    }
    concreteUrl = QUrl::fromLocalFile(destFile);

    GraphicsMediaItem *videoItem = mCurrentScene->addVideo(concreteUrl, false);

    QTransform transform;
    QString textTransform = element.attribute(aTransform);

    videoItem->resetTransform();
    if (!textTransform.isNull()) {
        transform = transformFromString(textTransform, videoItem);
    }
    repositionSvgItem(videoItem, videoItem->boundingRect().width(), videoItem->boundingRect().height(),
                      x + transform.m31(), y + transform.m32(), transform);
    hashSceneItem(element, videoItem);

    if (mGSectionContainer) {
        addItemToGSection(videoItem);
    }

    return true;
}

void IwbSubsetAdaptor::IwbSubsetReader::parseSvgSectionAttr(const QDomElement &svgSection) {
    getViewBoxDimenstions(svgSection.attribute(aViewbox));
    mSize = QSize(svgSection.attribute(aWidth).toInt(), svgSection.attribute(aHeight).toInt());
}

void IwbSubsetAdaptor::IwbSubsetReader::addItemToGSection(QGraphicsItem *item) {
    mGSectionContainer->addToGroup(item);
}

void IwbSubsetAdaptor::IwbSubsetReader::hashSceneItem(const QDomElement &element, GraphicsItem *item) {
    //    adding element pointer to hash to refer if needed
    QString key = element.attribute(aId);
    if (!key.isNull()) {
        persistedItems.insert(key, item);
    }
}

bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgElement(const QDomElement &parent) {
    QString tagName = parent.tagName();
    if (parent.namespaceURI() != svgNS) {
        qWarning() << "Incorrect namespace, error at content file, line number" << parent.lineNumber();
        //return false;
    }

    if (tagName == tG && !parseGSection(parent))
        return false;
    else if (tagName == tSwitch && !parseSvgSwitchSection(parent))
        return false;
    else if (tagName == tRect && !parseSvgRect(parent))
        return false;
    else if (tagName == tEllipse && !parseSvgEllipse(parent))
        return false;
    else if (tagName == tPolygon && !parseSvgPolygon(parent))
        return false;
    else if (tagName == tPolyline && !parseSvgPolyline(parent))
        return false;
    else if (tagName == tText && !parseSvgText(parent))
        return false;
    else if (tagName == tTextarea && !parseSvgTextarea(parent))
        return false;
    else if (tagName == tImage && !parseSvgImage(parent))
        return false;
    else if (tagName == tFlash && !parseSvgFlash(parent))
        return false;
    else if (tagName == tAudio && !parseSvgAudio(parent))
        return false;
    else if (tagName == tVideo && !parseSvgVideo(parent))
        return false;

    return true;
}

bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgPage(const QDomElement &parent) {
    createNewScene();
    QDomElement currentSvgElement = parent.firstChildElement();
    while (!currentSvgElement.isNull()) {
        if (!parseSvgElement(currentSvgElement))
            return false;

        currentSvgElement = currentSvgElement.nextSiblingElement();
    }

    return true;
}
bool IwbSubsetAdaptor::IwbSubsetReader::parseSvgPageset(const QDomElement &parent) {
    QDomElement currentPage = parent.firstChildElement(tPage);
    while (!currentPage.isNull()) {
        if (!parseSvgPage(currentPage))
            return false;
        currentPage = currentPage.nextSiblingElement(tPage);
    }
    return true;
}

bool IwbSubsetAdaptor::IwbSubsetReader::parseIwbMeta(const QDomElement &element) {
    if (element.namespaceURI() != iwbNS) {
        qWarning() << "incorrect meta namespace, incorrect document";
        //return false;
    }

    return true;
}
bool IwbSubsetAdaptor::IwbSubsetReader::parseSvg(const QDomElement &svgSection) {
    if (svgSection.namespaceURI() != svgNS) {
        qWarning() << "incorrect svg namespace, incorrect document";
        // return false;
    }

    parseSvgSectionAttr(svgSection);
    QDomElement currentSvg = svgSection.firstChildElement();

    if (currentSvg.tagName() != tPageset) {
        parseSvgPage(svgSection);
    } else if (currentSvg.tagName() == tPageset) {
        parseSvgPageset(currentSvg);
    }

    return true;
}

GraphicsGroupContainerItem *IwbSubsetAdaptor::IwbSubsetReader::parseIwbGroup(QDomElement &parent) {
    //TODO. Create groups from elements parsed by parseIwbElement() function
    if (parent.namespaceURI() != iwbNS) {
        qWarning() << "incorrect iwb group namespace, incorrect document";
        //  return false;
    }

    GraphicsGroupContainerItem *group = new GraphicsGroupContainerItem();
    QMultiMap<QString, GraphicsPolygonItem *> strokesGroupsContainer;
    QList<QGraphicsItem *> groupContainer;
    QString currentStrokeIdentifier;

    QDomElement currentStrokeElement = parent.firstChildElement();
    while (!currentStrokeElement.isNull()) {
        if (tGroup == currentStrokeElement.tagName())
            group->addToGroup(parseIwbGroup(currentStrokeElement));
        else {
            QString ref = currentStrokeElement.attribute(aRef);
            QString uuid = mRefToUuidMap[ref];
            if (!uuid.isEmpty()) {
                if (ref.size() > QUuid().toString().length()) // create stroke group
                {
                    currentStrokeIdentifier = ref.left(QUuid().toString().length() - 1);
                    GraphicsPolygonItem *strokeByUuid = qgraphicsitem_cast<GraphicsPolygonItem *>(
                        mCurrentScene->itemForUuid(QUuid(ref.right(QUuid().toString().length()))));

                    if (strokeByUuid)
                        strokesGroupsContainer.insert(currentStrokeIdentifier, strokeByUuid);
                } else // single elements in group
                    groupContainer.append(mCurrentScene->itemForUuid(QUuid(uuid)));
            }
        }
        currentStrokeElement = currentStrokeElement.nextSiblingElement();
    }


    foreach (QString key,
             QSet<QString>(strokesGroupsContainer.keys().begin(), strokesGroupsContainer.keys().end())) {
        GraphicsStrokesGroup *pStrokesGroup = new GraphicsStrokesGroup();
        GraphicsStroke *currentStroke = new GraphicsStroke();
        foreach (GraphicsPolygonItem *poly, strokesGroupsContainer.values(key)) {
            if (poly) {
                mCurrentScene->removeItem(poly);
                mCurrentScene->removeItemFromDeletion(poly);
                poly->setStrokesGroup(pStrokesGroup);
                poly->setStroke(currentStroke);
                pStrokesGroup->addToGroup(poly);
            }
        }
        if (currentStroke->polygons().empty())
            delete currentStroke;

        if (pStrokesGroup->childItems().count()) {
            mCurrentScene->addItem(pStrokesGroup);
            groupContainer.append(qgraphicsitem_cast<QGraphicsItem *>(pStrokesGroup));
        } else {
            // 空笔迹组用完就删；删完不能再进 groupContainer（悬空指针）。
            delete pStrokesGroup;
        }
    }

    foreach (QGraphicsItem *item, groupContainer)
        group->addToGroup(item);

    if (group->childItems().count()) {
        mCurrentScene->addItem(group);

        if (1 == group->childItems().count()) {
            group->destroy(false);
        }
    }

    return group;
}

bool IwbSubsetAdaptor::IwbSubsetReader::strToBool(QString str) {
    return str == "true";
}

bool IwbSubsetAdaptor::IwbSubsetReader::parseIwbElement(QDomElement &element) {
    if (element.namespaceURI() != iwbNS) {
        qWarning() << "incorrect iwb element namespace, incorrect document";
        //  return false;
    }

    bool locked = false;
    bool isEditableItem = false;
    bool isEditable = false; //Text items to convert to GraphicsTextItem only

    QString IDRef = element.attribute(aRef);
    if (!IDRef.isNull()) {
        element.hasAttribute(aBackground) ? strToBool(element.attribute(aBackground)) : false;
        locked = element.hasAttribute(aBackground) ? strToBool(element.attribute(aBackground)) : false;
        isEditableItem = element.hasAttribute(aEditable);
        if (isEditableItem)
            isEditable = strToBool(element.attribute(aEditable));

        GraphicsItem *referedItem(0);
        QHash<QString, GraphicsItem *>::iterator iReferedItem;
        iReferedItem = persistedItems.find(IDRef);
        if (iReferedItem != persistedItems.end()) {
            referedItem = *iReferedItem;
            referedItem->Delegate()->lock(locked);
        }
        // ref 指向的 id 没被解析出来时 referedItem 仍为 0（例如引用了被跳过的媒体），
        // 这里必须判空再取 Delegate()。
        if (isEditableItem && referedItem) {
            GraphicsTextItemDelegate *textDelegate =
                dynamic_cast<GraphicsTextItemDelegate *>(referedItem->Delegate());
            if (textDelegate) {
                textDelegate->setEditable(isEditable);
            }
        }
    }

    return true;
}
bool IwbSubsetAdaptor::IwbSubsetReader::parseDoc() {
    QDomElement currentTopElement = mDOMdoc.documentElement().firstChildElement();
    while (!currentTopElement.isNull()) {
        QString tagName = currentTopElement.tagName();
        if (tagName == tMeta && !parseIwbMeta(currentTopElement))
            return false;
        else if (tagName == tSvg && !parseSvg(currentTopElement))
            return false;
        else if (tagName == tGroup && !parseIwbGroup(currentTopElement))
            return false;
        else if (tagName == tElement && !parseIwbElement(currentTopElement))
            return false;

        currentTopElement = currentTopElement.nextSiblingElement();
    }
    if (!persistScenes())
        return false;

    return true;
}

void IwbSubsetAdaptor::IwbSubsetReader::repositionSvgItem(QGraphicsItem *item, qreal width, qreal height,
                                                          qreal x, qreal y, QTransform &transform) {
    //First using viebox coordinates, then translate them to scene coordinates

    QRectF itemBounds = item->boundingRect();

    qreal xScale = width / itemBounds.width();
    qreal yScale = height / itemBounds.height();

    qreal fullScaleX = mVBTransFactor * xScale;
    qreal fullScaleY = mVBTransFactor * yScale;

    QPointF oldVector((x - transform.dx()), (y - transform.dy()));
    QTransform rTransform;
    QPointF newVector = rTransform.map(oldVector);

    QTransform tr = item->sceneTransform();
    item->setTransform(rTransform.scale(fullScaleX, fullScaleY), true);
    tr = item->sceneTransform();
    QPoint pos;
    if (GraphicsTextItem::Type == item->type())
        pos = QPoint((int)((x + mShiftVector.x() + (newVector - oldVector).x())),
                     (int)((y + mShiftVector.y() + (newVector - oldVector).y()) * mVBTransFactor));
    else
        pos = QPoint((int)((x + mShiftVector.x() + (newVector - oldVector).x()) * mVBTransFactor),
                     (int)((y + mShiftVector.y() + (newVector - oldVector).y()) * mVBTransFactor));


    item->setPos(pos);
}

bool IwbSubsetAdaptor::IwbSubsetReader::createNewScene() {
    mCurrentScene =
        PersistenceManager::persistenceManager()->createDocumentSceneAt(mProxy, mProxy->pageCount(), false);
    mCurrentScene->setSceneRect(mViewBox);
    if ((mCurrentScene->sceneRect().topLeft().x() >= 0) || (mCurrentScene->sceneRect().topLeft().y() >= 0)) {
        mShiftVector = -mViewBox.center();
    }
    mCurrentSceneRect = mViewBox;
    mVBTransFactor = qMin(mCurrentScene->normalizedSceneRect().width() / mViewPort.width(),
                          mCurrentScene->normalizedSceneRect().height() / mViewPort.height());
    return true;
}

bool IwbSubsetAdaptor::IwbSubsetReader::persistCurrentScene() {
    if (mCurrentScene != 0 && mCurrentScene->isModified()) {
        ThumbnailAdaptor::persistScene(mProxy, mCurrentScene, mProxy->pageCount() - 1);
        SvgSubsetAdaptor::persistScene(mProxy, mCurrentScene, mProxy->pageCount() - 1);

        mCurrentScene->setModified(false);
        mCurrentScene = 0;
    }
    return true;
}
bool IwbSubsetAdaptor::IwbSubsetReader::persistScenes() {
    if (!mProxy->pageCount()) {
        qDebug() << "No pages created";
        return false;
    }
    for (int i = 0; i < mProxy->pageCount(); i++) {
        mCurrentScene = PersistenceManager::persistenceManager()->getDocumentScene(mProxy, i);
        if (!mCurrentScene) {
            qDebug() << "can't allocate scene, loading failed";
            return false;
        }

        SvgSubsetAdaptor::persistScene(mProxy, mCurrentScene, i);
        GraphicsScene *tmpScene = SvgSubsetAdaptor::loadScene(mProxy, i);
        tmpScene->setModified(true);
        ThumbnailAdaptor::persistScene(mProxy, tmpScene, i);
        mCurrentScene->setModified(false);
    }

    return true;
}

QColor IwbSubsetAdaptor::IwbSubsetReader::colorFromString(const QString &clrString) {
    //init regexp with pattern
    //pattern corresponds to strings like 'rgb(1,2,3) or rgb(10%,20%,30%)'
    QRegExp regexp("rgb\\(([0-9]+%{0,1}),([0-9]+%{0,1}),([0-9]+%{0,1})\\)");
    if (regexp.exactMatch(clrString)) {
        if (regexp.capturedTexts().count() == 4 &&
            regexp.capturedTexts().at(0).length() == clrString.length()) {
            int r = regexp.capturedTexts().at(1).toInt();
            if (regexp.capturedTexts().at(1).indexOf("%") != -1)
                r = r * 255 / 100;
            int g = regexp.capturedTexts().at(2).toInt();
            if (regexp.capturedTexts().at(2).indexOf("%") != -1)
                g = g * 255 / 100;
            int b = regexp.capturedTexts().at(3).toInt();
            if (regexp.capturedTexts().at(3).indexOf("%") != -1)
                b = b * 255 / 100;
            return QColor(r, g, b);
        } else
            return QColor();
    } else
        return QColor(clrString);
}

QTransform IwbSubsetAdaptor::IwbSubsetReader::transformFromString(const QString trString,
                                                                  QGraphicsItem *item) {
    qreal dxr = 0.0;
    qreal dyr = 0.0;
    qreal dx = 0.0;
    qreal dy = 0.0;
    qreal angle = 0.0;
    QTransform tr;

    foreach (QString trStr, trString.split(" ", Qt::SkipEmptyParts)) {
        //check pattern for strings like 'rotate(10)'
        QRegExp regexp("rotate\\( *([-+]{0,1}[0-9]*\\.{0,1}[0-9]*) *\\)");
        if (regexp.exactMatch(trStr)) {
            angle = regexp.capturedTexts().at(1).toDouble();
            if (item) {
                item->setTransformOriginPoint(QPointF(0, 0));
                item->setRotation(angle);
            }
            continue;
        };

        //check pattern for strings like 'rotate(10,20,20)' or 'rotate(10.1,10.2,34.2)'
        regexp.setPattern("rotate\\( *([-+]{0,1}[0-9]*\\.{0,1}[0-9]*) *, *([-+]{0,1}[0-9]*\\.{0,1}[0-9]*) *, "
                          "*([-+]{0,1}[0-9]*\\.{0,1}[0-9]*) *\\)");
        if (regexp.exactMatch(trStr)) {
            angle = regexp.capturedTexts().at(1).toDouble();
            dxr = regexp.capturedTexts().at(2).toDouble();
            dyr = regexp.capturedTexts().at(3).toDouble();
            if (item) {
                item->setTransformOriginPoint(QPointF(dxr, dyr) - item->pos());
                item->setRotation(angle);
            }
            continue;
        }

        //check pattern for strings like 'translate(11.0, 12.34)'
        regexp.setPattern(
            "translate\\( *([-+]{0,1}[0-9]*\\.{0,1}[0-9]*) *,*([-+]{0,1}[0-9]*\\.{0,1}[0-9]*)*\\)");
        if (regexp.exactMatch(trStr)) {
            dx = regexp.capturedTexts().at(1).toDouble();
            dy = regexp.capturedTexts().at(2).toDouble();
            tr.translate(dx, dy);
            continue;
        }
    }
    return tr;
}

bool IwbSubsetAdaptor::IwbSubsetReader::getViewBoxDimenstions(const QString &viewBox) {
    QStringList capturedTexts = viewBox.split(" ", Qt::SkipEmptyParts);
    if (capturedTexts.count()) {
        if (4 == capturedTexts.count()) {
            mViewBox = QRectF(capturedTexts.at(0).toDouble(), capturedTexts.at(1).toDouble(),
                              capturedTexts.at(2).toDouble(), capturedTexts.at(3).toDouble());
            mViewPort = mViewBox;
            mViewPort.translate(-mViewPort.center());
            mViewBoxCenter.setX(mViewBox.width() / 2);
            mViewBoxCenter.setY(mViewBox.height() / 2);

            return true;
        }
    }

    mViewBox = QRectF(0, 0, 1000, 1000);
    mViewBoxCenter = QPointF(500, 500);
    return false;
}

QSvgGenerator *IwbSubsetAdaptor::IwbSubsetReader::createSvgGenerator(qreal width, qreal height) {
    QSvgGenerator *generator = new QSvgGenerator();
    //    qWarning() << QString("Making generator with file %1, size (%2, %3) and viewbox (%4 %5 %6 %7)").arg(mTempFilePath)
    //        .arg(width).arg(height).arg(0.0).arg(0.0).arg(width).arg(width);
    generator->setResolution(VirtualDesktop::instance()->physicalDpi());
    generator->setFileName(mTempFilePath);
    generator->setSize(QSize(width, height));
    generator->setViewBox(QRectF(0, 0, width, height));

    return generator;
}

bool IwbSubsetAdaptor::IwbSubsetReader::getTempFileName() {
    int tmpNumber = 0;
    QDir rootDir;
    while (true) {
        mTempFilePath = QString("%1/iwbSvg%2.%3")
                            .arg(rootDir.tempPath())
                            .arg(QDateTime::currentDateTime().toString("dd_MM_yyyy_HH-mm"))
                            .arg(tmpNumber);
        if (!QFile::exists(mTempFilePath))
            return true;
        tmpNumber++;
        if (tmpNumber == 100000) {
            qWarning() << "Import failed. Failed to create temporary file for svg objects";
            return false;
        }
    }
}
bool IwbSubsetAdaptor::IwbSubsetReader::createTempFlashPath() {
    int tmpNumber = 0;
    QDir systemTmp = QDir::temp();

    while (true) {
        QString dirName = QString("IwbFlashTmp%1.%2")
                              .arg(QDateTime::currentDateTime().toString("dd_MM_yyyy_HH-mm"))
                              .arg(tmpNumber++);
        if (!systemTmp.exists(dirName)) {
            if (systemTmp.mkdir(dirName)) {
                mTmpFlashDir = QDir(systemTmp.absolutePath() + "/" + dirName);
                return true;
            } else {
                qDebug() << "Can't create temporary dir maybe due to permissions";
                return false;
            }
        } else if (tmpNumber == 1000) {
            qWarning() << "Import failed. Failed to create temporary file for svg objects";
            return false;
        }
    }

    return true;
}
IwbSubsetAdaptor::IwbSubsetReader::~IwbSubsetReader() {
    //    QList<int> pages;
    //    for (int i = 0; i < mProxy->pageCount(); i++) {
    //        pages << i;
    //    }
    //    PersistenceManager::persistenceManager()->deleteDocumentScenes(mProxy, pages);
}
