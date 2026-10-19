#pragma once

#include "content/IwbExport.h"

#include <QtCore>

class QTransform;
class QDomDocument;
class QDomElement;
class QDomNode;
class QuaZipFile;

class IwbDocumentAdaptor {
    class ToIwbConverter;

public:
    IwbDocumentAdaptor();
    ~IwbDocumentAdaptor();

    bool convertPageToIwb(const QString &from, const QString &to);
    bool deleteDir(const QString& pDirPath) const;
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
        ToIwbConverter(const QString &source, const QString &destination);
        ~ToIwbConverter();
        bool isValid() const;
        QString lastErrStr() const {return errorStr;}
        bool parse();
        QList<QString> getMessages() {return mExportErrorList;}

    private:
        void addLastExportError(QString error) {mExportErrorList.append(error);}

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
        bool createPngFromSvg(QString &svgPath, QString &dstPath,  QTransform transformation, QSize size = QSize());

        bool parseSVGGGroup(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePageImage(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePageVideo(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePageAudio(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parseForeignObject(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePageText(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);

        bool parsePagePolygon(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePagePolyline(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);
        bool parsePageLine(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList);       
        void addSVGElementToResultModel(const QDomElement &element, QMultiMap<int, QDomElement> &dstList, int layer = DEFAULT_LAYER);
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

        bool ibwAddLine(int x1, int y1, int x2, int y2, QString color=QString(), int width=1, bool isBackground=false);

        QTransform transformOfPageElement(const QDomElement &pageElement);
        void setGeometryFromPageElement(const QDomElement &pageElement, QDomElement &iwbElement);
        void setCoordinatesFromPageElement(const QDomElement &pageElement, QDomElement &iwbElement);
        bool setContentFromPageElement(const QDomElement &pageElement, QDomElement &svgElement);
        void setIwbTextFromHTMLTextNode(const QDomElement htmlTextNode, QDomElement &iwbElement);
        QString packageAttrNameToIwbAttrName(QString iwbAttrName);
        QString packageAttrValueToIwbAttrName(QString iwbAttrValue);

        bool setIwbAttribute(const QString &attributeName, const QString &attributeValue, const QDomElement &pageElement, QDomElement &iwbElement,  QDomElement &svgElement);
        bool copyPageCommonAttributes(const QDomElement &pageElement, QDomElement &iwbElement,  QDomElement &svgElement);
        void setViewBox(QRect viewbox);

        QDomNode findTextNode(const QDomNode &node);
        QDomNode findNodeByTagName(const QDomNode &node, QString tagName);

        QSize getSVGDimentions(const QString &element);

        inline QRect getViewboxRect(const QString &element) const;
        inline QString rectToIWBAttr(const QRect &rect) const;
        inline QString digitFileFormat(int num) const;
        inline bool strToBool(const QString &in) const {return in == "true";}
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
        operator bool() const {return isValid();}
    };
};

