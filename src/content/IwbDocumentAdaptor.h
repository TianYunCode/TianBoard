/**
 * @file IwbDocumentAdaptor.h
 * @brief 把 .tbz 文档目录转换为 IWB 1.0 包。
 */

#pragma once

#include "content/IwbExport.h"

#include <QtCore>

class QTransform;
class QDomDocument;
class QDomElement;
class QDomNode;
class QuaZipFile;

/**
 * @brief IWB 导出入口：解压/定位源目录，驱动 ToIwbConverter，再压缩成 .iwb。
 *
 * 输入可以是已解压的文档目录，也可以是 .tbz；输出总是 zip 形式的 .iwb
 * （content.xml + images/video/audio 子目录）。转换过程中的临时目录登记在
 * tmpDirs 中，析构时统一清理。与 PersistenceManager 无直接依赖：它只读取
 * 磁盘上的文档目录结构，不理解 DocumentProxy。
 */
class IwbDocumentAdaptor {
    /**
     * @brief 实际的 .tbz -> IWB 内容转换器。
     *
     * 读取 metadata.rdf 生成 <iwb:meta>，逐页解析 pageNNN.svg，把受支持的元素
     * 映射为 IWB 的 SVG 子集与 <iwb:element> 扩展段，媒体文件按类型拷入包内
     * 目录。解析失败时把错误写入 errorStr / mExportErrorList。
     */
    class ToIwbConverter;

public:
    IwbDocumentAdaptor();
    ~IwbDocumentAdaptor();

    /**
     * @brief 把文档目录或 .tbz 转换为 .iwb。
     * @param from 源目录或 .tbz 路径。
     * @param to 目标 .iwb 路径。
     * @return 转换并压缩成功时返回 true；源不可读、解析失败或写包失败返回 false。
     * @note 失败信息通过 getConversionMessages() 取回；临时目录在成功路径上立即清理。
     */
    bool convertPageToIwb(const QString &from, const QString &to);
    /** @brief 递归删除目录；路径为空、"." 或 ".." 时拒绝执行。 */
    bool deleteDir(const QString &pDirPath) const;
    /** @brief 返回转换过程中累积的错误/提示信息。 */
    QList<QString> getConversionMessages();

private:
    QString uncompressZip(const QString &zipFile);
    bool compressZip(const QString &source, const QString &destination);
    bool compressDir(const QString &dirName, const QString &parentDir, QuaZipFile *outZip);
    bool compressFile(const QString &fileName, const QString &parentDir, QuaZipFile *outZip);

    QString createNewTmpDir();
    bool freeDir(const QString &dir);
    void freeTmpDirs();

private:
    QStringList tmpDirs;
    QList<QString> mConversionMessages;

private:
    class ToIwbConverter {
        static const int DEFAULT_LAYER = -100000;

    public:
        /** @brief 记录源目录与目标目录，并初始化 DOM 与 XML 写入器。 */
        ToIwbConverter(const QString &source, const QString &destination);
        ~ToIwbConverter();
        /** @brief 源目录存在且尚无错误时返回 true。 */
        bool isValid() const;
        /** @brief 返回最近一次错误描述，无错误时为 noErrorMsg。 */
        QString lastErrStr() const { return errorStr; }
        /** @brief 生成 content.xml：先写 meta，再写页面，最后写扩展段。 */
        bool parse();
        /** @brief 返回转换中累积的错误列表，供上层以警告形式展示。 */
        QList<QString> getMessages() { return mExportErrorList; }

    private:
        void addLastExportError(QString error) { mExportErrorList.append(error); }

        void fillNamespaces();

        bool parseMetadata();
        bool parseContent();
        QDomElement parsePageset(const QStringList &pageFileNames);
        QDomElement parsePage(const QString &pageFileName);
        QDomElement parseSvgPageSection(const QDomElement &element);
        void writeQDomElementToXML(const QDomNode &node);
        bool writeExtendedIwbSection();
        QDomElement parseGroupsPageSection(const QDomElement &groupRoot);

        bool createBackground(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        QString createBackgroundImage(const QDomElement &element, QSize size);
        bool createPngFromSvg(QString &svgPath, QString &dstPath, QTransform transformation,
                              QSize size = QSize());

        bool parseSVGGGroup(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePageImage(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePageVideo(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePageAudio(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parseForeignObject(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePageText(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);

        bool parsePagePolygon(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePagePolyline(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePageLine(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        void addSVGElementToResultModel(const QDomElement &element, QMultiMap<int, QDomElement> &dstList,
                                        int layer = DEFAULT_LAYER);
        void addIWBElementToResultModel(const QDomElement &element);

        qreal getAngleFromTransform(const QTransform &tr);
        QString getDstContentFolderName(const QString &elementType);
        QString getSrcContentFolderName(QString href);
        QString getFileNameFromPath(QString sPath);
        QString getExtentionFromFileName(const QString &filename);
        QString convertExtention(const QString &ext);
        QString pageElementTypeName(const QDomElement &element);

        int getElementLayer(const QDomElement &element);

        bool itIsSupportedFormat(const QString &format) const;
        bool itIsFormatToConvert(const QString &format) const;
        bool itIsSVGElementAttribute(const QString ItemType, const QString &AttrName);
        bool itIsIWBAttribute(const QString &attribute) const;
        bool isPageAttributeToConvert(const QString &attribute) const;

        bool ibwAddLine(int x1, int y1, int x2, int y2, QString color = QString(), int width = 1,
                        bool isBackground = false);

        QTransform transformOfPageElement(const QDomElement &pageElement);
        void setGeometryFromPageElement(const QDomElement &pageElement, QDomElement &iwbElement);
        void setCoordinatesFromPageElement(const QDomElement &pageElement, QDomElement &iwbElement);
        bool setContentFromPageElement(const QDomElement &pageElement, QDomElement &svgElement);
        void setIwbTextFromHTMLTextNode(const QDomElement htmlTextNode, QDomElement &iwbElement);
        QString packageAttrNameToIwbAttrName(QString iwbAttrName);
        QString packageAttrValueToIwbAttrName(QString iwbAttrValue);

        bool setIwbAttribute(const QString &attributeName, const QString &attributeValue,
                             const QDomElement &pageElement, QDomElement &iwbElement,
                             QDomElement &svgElement);
        bool copyPageCommonAttributes(const QDomElement &pageElement, QDomElement &iwbElement,
                                      QDomElement &svgElement);
        void setViewBox(QRect viewbox);

        QDomNode findTextNode(const QDomNode &node);
        QDomNode findNodeByTagName(const QDomNode &node, QString tagName);

        QSize getSVGDimentions(const QString &element);

        inline QRect getViewboxRect(const QString &element) const;
        inline QString rectToIWBAttr(const QRect &rect) const;
        inline QString digitFileFormat(int num) const;
        inline bool strToBool(const QString &in) const { return in == "true"; }
        QString contentIWBFileName() const;

    private:
        QList<QString> mExportErrorList;
        QMap<QString, QString> iwbSVGItemsAttributes;
        QDomDocument *mDataModel;
        QXmlStreamWriter *mIWBContentWriter;
        QSize mSVGSize;
        QRect mViewbox;
        QString sourcePath;
        QString destinationPath;
        QDomDocument *mDocumentToWrite;
        QMultiMap<int, QDomElement> mSvgElements;
        QList<QDomElement> mExtendedElements;
        mutable QString errorStr;

    public:
        /** @brief 等价于 isValid()，便于在条件语句中直接判断。 */
        operator bool() const { return isValid(); }
    };
};
