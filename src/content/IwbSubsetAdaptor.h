/**
 * @file IwbSubsetAdaptor.h
 * @brief 读取 IWB 1.0 内容（content.xml），转换为内部文档包。
 */

#pragma once

#include <QtXml>
#include <QString>
#include <QStack>
#include <QDomDocument>
#include <QHash>

class DocumentProxy;
class GraphicsScene;
class QSvgGenerator;
class GraphicsSvgItem;
class GraphicsPixmapItem;
class GraphicsItemDelegate;
class QTransform;
class QPainter;
class GraphicsItem;
class QGraphicsItem;
class QTextBlockFormat;
class QTextCharFormat;
class QTextCursor;
class GraphicsStrokesGroup;

/**
 * @brief IWB 导入适配器：把 IWB 页面解析成场景，再逐页写进文档包。
 *
 * 负责格式一侧的全部工作：<pageset>/<page> 的页序、<g> 分组、<switch> 分支、
 * 各类图形与媒体元素、z-value 层序、viewBox 到场景坐标的换算。写出的结果不是
 * DocumentProxy 对象，而是通过 PersistenceManager 落盘的页面 SVG 与资源目录，
 * 调用方随后用指针创建或追加文档。
 */
class IwbSubsetAdaptor
{
public:
    IwbSubsetAdaptor();
    /**
     * @brief 把 iwbSourceFile 解析成文档包，页面写入 pDocument 的持久化目录。
     * @param iwbSourceFile 源 content.xml 路径（可能被改写为临时副本）。
     * @param pDocument 目标文档代理，提供持久化目录与默认页面尺寸。
     * @return 至少成功解析一页时返回 true。
     */
    static bool convertIwbFileToDocumentPackage(QString &iwbSourceFile, DocumentProxy *pDocument);

private:
    /**
     * @brief 单次导入的解析器：持有 DOM 与“引用 id -> 图元”索引。
     *
     * IWB 用 id/ref 把 SVG 元素与 <iwb:element> 扩展属性关联起来，解析顺序不保证
     * 被引用者先出现，因此先 hash 出所有元素再逐个回填；每解析完一页立即持久化，
     * 避免整本文档同时驻留内存。
     */
    class IwbSubsetReader
    {
    public:
        /** @brief 绑定目标文档与 content.xml 文件设备，读取前不解析。 */
        IwbSubsetReader(DocumentProxy *proxy, QFile *content);
        ~IwbSubsetReader();

        /// 目标文档；提供页面数量、尺寸与持久化目录。
        DocumentProxy *mProxy;
        /// content.xml 的文本内容缓存（解析期间复用，避免反复读盘）。
        QString pwdContent;

        /** @brief 执行完整解析；至少一页成功时返回 true。 */
        bool parse();

    private:
        QString                     mTempFilePath;
        GraphicsScene              *mCurrentScene;
        QRectF                      mCurrentSceneRect;
        QString                     mIndent;
        QRectF                      mViewBox;
        QRectF                      mViewPort;
        qreal                       mVBTransFactor;
        QPointF                     mViewBoxCenter;
        QSize                       mSize;
        QPointF                     mShiftVector;
        bool                        mSvgGSectionIsOpened;
        GraphicsGroupContainerItem *mGSectionContainer;

    private:
        QDomDocument                   mDOMdoc;
        QDomNode                       mCurrentDOMElement;
        QHash<QString, GraphicsItem *> persistedItems;
        QMap<QString, QString>         mRefToUuidMap;
        QDir                           mTmpFlashDir;

        void addItemToGSection(QGraphicsItem *item);
        bool hashElements();
        void addExtentionsToHash(QDomElement *parent, QDomElement *topGroup);

        void hashSvg(QDomNode *parent, QString prefix = "");
        void hashSiblingIwbElements(QDomElement *parent, QDomElement *topGroup = 0);

        inline void parseSvgSectionAttr(const QDomElement &);
        bool        parseSvgPage(const QDomElement &parent);
        bool        parseSvgPageset(const QDomElement &parent);
        bool        parseSvgElement(const QDomElement &parent);
        bool        parseIwbMeta(const QDomElement &element);
        bool        parseSvg(const QDomElement &svgSection);

        inline bool                        parseGSection(const QDomElement &element);
        inline bool                        parseSvgSwitchSection(const QDomElement &element);
        inline bool                        parseSvgRect(const QDomElement &element);
        inline bool                        parseSvgEllipse(const QDomElement &element);
        inline bool                        parseSvgPolygon(const QDomElement &element);
        inline bool                        parseSvgPolyline(const QDomElement &element);
        inline bool                        parseSvgText(const QDomElement &element);
        inline bool                        parseSvgTextarea(const QDomElement &element);
        inline bool                        parseSvgImage(const QDomElement &element);
        inline bool                        parseSvgFlash(const QDomElement &element);
        inline bool                        parseSvgAudio(const QDomElement &element);
        inline bool                        parseSvgVideo(const QDomElement &element);
        inline GraphicsGroupContainerItem *parseIwbGroup(QDomElement &parent);
        inline bool                        parseIwbElement(QDomElement &element);
        inline void parseTSpan(const QDomElement &parent, QPainter &painter, qreal &curX, qreal &curY, qreal &width, qreal &height,
            qreal &linespacing, QRectF &lastDrawnTextBoundingRect, qreal &fontSize, QColor &fontColor, QString &fontFamily, QString &fontStretch,
            bool &italic, int &fontWeight, int &textAlign, QTransform &fontTransform);
        inline void parseTSpan(const QDomElement &element, QTextCursor &cursor, QTextBlockFormat &blockFormat, QTextCharFormat &charFormat);
        inline void hashSceneItem(const QDomElement &element, GraphicsItem *item);

        // 以下文本属性读取服务于旧版内嵌 HTML 文本路径，新解析走 QTextCursor 分支
        inline void parseTextAttributes(const QDomElement &element, qreal &fontSize, QColor &fontColor, QString &fontFamily, QString &fontStretch,
            bool &italic, int &fontWeight, int &textAlign, QTransform &fontTransform);
        inline void parseTextAttributes(const QDomElement &element, QFont &font, QColor);
        inline void readTextBlockAttr(const QDomElement &element, QTextBlockFormat &format);
        inline void readTextCharAttr(const QDomElement &element, QTextCharFormat &format);

        bool parseDoc();

        bool createNewScene();
        bool persistCurrentScene();
        bool persistScenes();

        void           repositionSvgItem(QGraphicsItem *item, qreal width, qreal height, qreal x, qreal y, QTransform &transform);
        QColor         colorFromString(const QString &clrString);
        QTransform     transformFromString(const QString trString, QGraphicsItem *item = 0);
        bool           getViewBoxDimenstions(const QString &viewBox);
        QSvgGenerator *createSvgGenerator(qreal width, qreal height);
        bool           getTempFileName();
        inline bool    strToBool(QString);
        bool           createTempFlashPath();
    };
};
