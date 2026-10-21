#include "model/ForeignObjectsHandler.h"
#include <QRegularExpression>
#include <QRegExp>

#include <QtWidgets>
#include <QtXml>

const QString tVideo          = "video";
const QString tAudio          = "audio";
const QString tImage          = "image";
const QString tForeignObject  = "foreignObject";
const QString tTeacherGuide   = "teacherGuide";
const QString tMedia          = "media";
const QString tGroups         = "groups";

const QString aHref           = "xlink:href";
const QString aType           = "tb:type";
const QString aReqExt         = "requiredExtensions";
const QString aSrc            = "tb:src";
const QString aMediaType      = "mediaType";
const QString aRelativePath   = "relativePath";
const QString aActionMedia    = "tb:actionFirstParameter";

const QString vText           = "text";
const QString vReqExt         = "http://ns.adobe.com/pdf/1.3/";

const QString wgtSuff         = ".wgt";
const QString thumbSuff       = ".png";

const QString     scanDirs    = "audios,images,videos,teacherGuideObjects,widgets";
const QStringList trashFilter = QStringList() << "*.swf";

static QString strIdFrom(const QString &filePath)
{
    if ((filePath).isEmpty())
    {
        return QString();
    }

    QRegExp rx("\\{.(?!.*\\{).*\\}");
    if (rx.indexIn(filePath) == -1)
    {
        return QString();
    }

    return rx.cap();
}

static bool rm_r(const QString &rmPath)
{
    QFileInfo fi(rmPath);
    if (!fi.exists())
    {
        qDebug() << rmPath << "does not exist";
        return false;
    }
    else if (fi.isFile())
    {
        if (!QFile::remove(rmPath))
        {
            qDebug() << "can't remove file" << rmPath;
            return false;
        }
        return true;
    }
    else if (fi.isDir())
    {
        QFileInfoList fList = QDir(rmPath).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
        foreach (QFileInfo sub, fList)
        {
            rm_r(sub.absoluteFilePath());
        }
        if (!QDir().rmdir(rmPath))
        {
            qDebug() << "can't remove dir" << rmPath;
            return false;
        }
        return true;
    }
    return false;
}

static bool cp_rf(const QString &what, const QString &where)
{
    QFileInfo whatFi(what);
    QFileInfo whereFi = QFileInfo(where);

    if (!whatFi.exists())
    {
        qDebug() << what << "does not exist" << Q_FUNC_INFO;
        return false;
    }
    else if (whatFi.isFile())
    {
        QString whereDir    = where.section("/", 0, -2, QString::SectionSkipEmpty | QString::SectionIncludeLeadingSep);
        QString newFilePath = where;
        if (!whereFi.exists())
        {
            QDir().mkpath(whereDir);
        }
        else if (whereFi.isDir())
        {
            newFilePath = whereDir + "/" + whatFi.fileName();
        }
        if (QFile::exists(newFilePath))
        {
            QFile::remove(newFilePath);
        }
        if (!QFile::copy(what, newFilePath))
        {
            qDebug() << "can't copy" << what << "to" << where << Q_FUNC_INFO;
            return false;
        }
        return true;
    }
    else if (whatFi.isDir())
    {
        if (whereFi.isFile() && whereFi.fileName().toLower() == whatFi.fileName().toLower())
        {
            qDebug() << "can't copy dir" << what << "to file" << where << Q_FUNC_INFO;
            return false;
        }
        else if (whereFi.isDir())
        {
            rm_r(where);
        }

        QDir().mkpath(where);

        QFileInfoList fList = QDir(what).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
        foreach (QFileInfo sub, fList)
        {
            if (!cp_rf(sub.absoluteFilePath(), where + "/" + sub.fileName()))
            {
                return false;
            }
        }
        return true;
    }
    return true;
}

static QString thumbFileNameFrom(const QString &filePath)
{
    if (filePath.isEmpty())
    {
        return QString();
    }

    QString thumbPath = filePath;
    thumbPath.replace(QRegularExpression("[\\{\\}]"), "").replace(wgtSuff, thumbSuff);

    return thumbPath;
}

QString svgPageName(int pageIndex)
{
    return QString("page%1.svg").arg(pageIndex, 3, 10, QLatin1Char('0'));
}

static QDomDocument createDomFromSvg(const QString &svgUrl)
{
    Q_ASSERT(QFile::exists(svgUrl));
    QString mFoldersXmlStorageName = svgUrl;

    if (QFileInfo(mFoldersXmlStorageName).exists())
    {
        QDomDocument xmlDom;
        QFile        inFile(mFoldersXmlStorageName);
        if (inFile.open(QIODevice::ReadOnly))
        {
            QString domString(inFile.readAll());

            QDomDocument::ParseResult parseResult = xmlDom.setContent(domString);
            if (parseResult)
            {
                return xmlDom;
            }
            else
            {
                qDebug() << "Error reading content of " << mFoldersXmlStorageName << Qt::endl
                         << "Error:" << inFile.errorString() << "Line:" << parseResult.errorLine << "Column:" << parseResult.errorColumn
                         << parseResult.errorMessage;
            }
            inFile.close();
        }
        else
        {
            qDebug() << "Error reading" << mFoldersXmlStorageName << Qt::endl << "Error:" << inFile.errorString();
        }
    }

    return QDomDocument();
}

class Cleaner
{
public:
    void cure(const QUrl &dir)
    {
        mCurrentDir = dir.toLocalFile();
        cleanTrash();

        // 先收集各 SVG 页面里引用到的资源标识。
        QFileInfoList svgInfos = QDir(mCurrentDir).entryInfoList(QStringList() << "*.svg", QDir::NoDotAndDotDot | QDir::Files);
        foreach (QFileInfo svgInfo, svgInfos)
        {
            cureIdsFromSvgDom(createDomFromSvg(svgInfo.absoluteFilePath()));
        }

        fitIdsFromFileSystem();
        QVector<QString> deleteCandidates;
        findRedundandElements(deleteCandidates);

        foreach (QString key, deleteCandidates)
        {
            QString delPath = mPresentIdsMap.value(key);
            if (delPath.isNull())
            {
                continue;
            }
            else if (delPath.endsWith(wgtSuff))
            { // 控件文件对应的缩略图也要删除
                QString thumbPath = thumbFileNameFrom(delPath);

                if (QFile::exists(thumbPath))
                {
                    rm_r(thumbPath);
                }
            }
            rm_r(delPath);
            // 删除后若父目录已空，则一并移除。局部量不叫 dir：与本函数的参数同名，
            // 读起来分不清哪个是待清理目录、哪个是它的父目录。
            QDir parentDir(delPath);
            parentDir.cdUp();
            if (parentDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty())
            {
                // QDir::rmdir 收到绝对路径时按绝对路径处理，这里删的就是 cdUp 后的父目录本身。
                parentDir.rmdir(parentDir.absolutePath());
            }
        }

        qDebug() << "Ok on cure";
    }

private:
    void cleanTrash()
    {
        QFileInfoList ifs = QDir(mCurrentDir).entryInfoList(trashFilter, QDir::NoDotAndDotDot | QDir::Files);
        foreach (QFileInfo ifo, ifs)
        {
            rm_r(ifo.absoluteFilePath());
        }
    }

    void cureIdsFromSvgDom(const QDomDocument &dom)
    {
        Q_ASSERT(!dom.isNull());

        QDomElement nextElement = dom.documentElement().firstChildElement();
        while (!nextElement.isNull())
        {
            QString nextTag = nextElement.tagName();
            qDebug() << "Tag name of the next parsed element is" << nextTag;
            if (nextTag == tGroups)
            {
                nextElement = nextElement.firstChildElement("group");
            }

            invokeFromText(nextTag, nextElement);
            nextElement = nextElement.nextSiblingElement();
        }
    }

    void fitIdsFromFileSystem()
    {
        QString     absPrefix = mCurrentDir + "/";
        QStringList dirsList  = scanDirs.split(",", Qt::SkipEmptyParts);
        foreach (QString dirName, dirsList)
        {
            QString absPath = absPrefix + dirName;
            if (!QFile::exists(absPath))
            {
                continue;
            }
            fitIdsFromDir(absPath);
        }
    }

    void fitIdsFromDir(const QString &scanDir)
    {
        QFileInfoList fileList = QDir(scanDir).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        foreach (QFileInfo nInfo, fileList)
        {
            QString uid = strIdFrom(nInfo.fileName());
            if (uid.isNull())
            {
                continue;
            }
            mPresentIdsMap.insert(uid, nInfo.absoluteFilePath());
        }
    }

    void findRedundandElements(QVector<QString> &v)
    {
        // 再扫描磁盘上实际存在的资源标识，与 SVG 中记录的集合做对称差。
        QStringList domIds     = mDomIdsMap.keys();
        QStringList presentIds = mPresentIdsMap.keys();
        v.resize(qMax(domIds.count(), presentIds.count()));
        QVector<QString>::iterator it_diff;

        it_diff = std::set_symmetric_difference(domIds.begin(), domIds.end(), presentIds.begin(), presentIds.end(), v.begin());
        v.resize(it_diff - v.begin());
    }

    void invokeFromText(const QString &what, const QDomElement &element)
    {
        if (what == tVideo || what == tAudio || what == tImage)
        {
            mediaToContainer(element);
        }
        else if (what == tForeignObject)
        {
            foreingObjectToContainer(element);

            cleanObjectFolder(element);

            // 外部对象可能引用了 SVG 里没有记录的资源，需要补登记。
            addResourceIdToSvg(element);
        }
        else if (what == tTeacherGuide)
        {
            teacherGuideToContainer(element);
        }

        pullActionFromElement(element);
    }

    // 导出前清理对象目录，减小文档包体积。
    void cleanObjectFolder(const QDomElement &element)
    {
        QDomNodeList childrenNode = element.elementsByTagName("tb:preference");

        QVector<QString> objectsIdUsed;

        for (int i = 0; i < childrenNode.size(); i++)
        {
            QDomElement preference = childrenNode.at(i).toElement();

            if (!preference.isNull())
            {
                QString value = preference.attribute("value");

                int findPos   = value.indexOf("objects/");
                int endPos;

                // 收集该偏好值里引用到的所有对象路径。
                while (findPos != -1)
                {
                    endPos = value.indexOf("\"", findPos);
                    objectsIdUsed << value.mid(findPos, endPos - findPos);
                    findPos = value.indexOf("objects/", endPos);
                }
            }
        }

        QString path              = element.attribute(aSrc);
        QString objectsFolderPath = mCurrentDir + "/" + path + "/objects/";

        QDir dir(objectsFolderPath);
        dir.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);

        // 再把 objects/ 目录里剩下、却没被本文档引用的文件删掉，
        // 否则每次保存都会把用不上的对象越堆越多。
        QFileInfoList list = dir.entryInfoList();
        for (int i = 0; i < list.size(); i++)
        {
            QFileInfo fileInfo = list.at(i);

            if (!objectsIdUsed.contains("objects/" + fileInfo.fileName()))
            {
                QFile(fileInfo.filePath()).remove();
            }
        }
    }

    void addResourceIdToSvg(const QDomElement &element)
    {
        QDomElement textContent = element.firstChildElement("itemTextContent");

        QString value           = textContent.text();

        int findPos             = value.indexOf("images/");
        int endPos;

        // 记下 SVG 里引用到的每个 images/ 资源，按 UUID 建索引
        while (findPos != -1)
        {
            endPos       = value.indexOf("\"", findPos);

            QString path = value.mid(findPos, endPos - findPos);

            QString uuid = path.split("/").at(1).split(".").at(0);

            mDomIdsMap.insert(uuid, path);

            findPos = value.indexOf("images/", endPos);
        }
    }

    void pullActionFromElement(const QDomElement &element)
    {
        if (!element.hasAttribute(aActionMedia))
        {
            return;
        }

        QString path = element.attribute(aActionMedia);
        if (path.isNull())
        {
            return;
        }

        QString uid = strIdFrom(path);
        if (uid.isNull())
        {
            return;
        }

        mDomIdsMap.insert(uid, path);
    }

    void teacherGuideToContainer(const QDomElement &element)
    {
        QDomElement nMediaElement = element.firstChildElement(tMedia);
        while (!nMediaElement.isNull())
        {
            // 先取下一个兄弟再处理当前节点：下面有 continue，不推进游标会死循环。
            const QDomElement currentMediaElement = nMediaElement;
            nMediaElement                         = nMediaElement.nextSiblingElement(tMedia);

            QString path                          = currentMediaElement.attribute(aRelativePath);
            if (path.isNull())
            {
                continue;
            }

            QString uid = strIdFrom(path);
            if (uid.isNull())
            {
                return;
            }
            mDomIdsMap.insert(uid, path);
        }
    }

    void mediaToContainer(const QDomElement &element)
    {
        QString path = element.attribute(aHref);
        if (path.isNull())
        {
            return;
        }
        QString uid = strIdFrom(path);
        if (uid.isNull())
        {
            return;
        }
        mDomIdsMap.insert(uid, path);
    }

    void foreingObjectToContainer(const QDomElement &element)
    {
        QString type = element.attribute(aType);
        if (type == vText)
        { // 文本对象这里不用管，它走自己的序列化通道
            return;
        }

        QString path = element.attribute(aSrc);
        if (path.isNull())
        {
            return;
        }

        QString uid = strIdFrom(path);
        if (uid.isNull())
        {
            return;
        }

        mDomIdsMap.insert(uid, path);
    }

private:
    QString                mCurrentDir;
    QDomDocument           mSvgData;
    QMap<QString, QString> mDomIdsMap;
    QMap<QString, QString> mPresentIdsMap;
};

class PageCopier
{
public:
    void copyPage(const QUrl &fromDir, int fromIndex, const QUrl &toDir, int toIndex)
    {
        mFromDir             = fromDir.toLocalFile();
        mToDir               = toDir.toLocalFile();
        mFromIndex           = fromIndex;
        mToIndex             = toIndex;

        QString      svgFrom = mFromDir + "/" + svgPageName(fromIndex);
        QString      svgTo   = toDir.toLocalFile() + "/" + svgPageName(toIndex);
        QDomDocument dd      = createDomFromSvg(svgFrom);
        QFile        fl(svgTo);
        if (!fl.open(QIODevice::WriteOnly))
        {
            qDebug() << Q_FUNC_INFO << "can't open" << fl.fileName() << "for writing";
            return;
        }
        cureIdsFromSvgDom(dd);

        QTextStream str(&fl);
        dd.save(str, 0);
        fl.close();
        qDebug() << Q_FUNC_INFO;
    }

private:
    void cureIdsFromSvgDom(const QDomDocument &dom)
    {
        Q_ASSERT(!dom.isNull());

        QDomElement nextElement = dom.documentElement().firstChildElement();
        while (!nextElement.isNull())
        {
            qDebug() << "Tag name of the next parsed element is" << nextElement.tagName();
            QString nextTag = nextElement.tagName();
            cureFromText(nextTag, nextElement);
            nextElement = nextElement.nextSiblingElement();
        }
    }

    void cureFromText(const QString &tagName, QDomElement element)
    {
        if (tagName == tVideo || tagName == tAudio || tagName == tImage)
        {
            QString newRelative = cureNCopy(element.attribute(aHref));
            element.setAttribute(aHref, newRelative);
            if (element.hasAttribute(aActionMedia))
            {
                QString newActionPath = cureNCopy(element.attribute(aActionMedia));
                element.setAttribute(aActionMedia, newActionPath);
            }
        }
        else if (tagName == tForeignObject)
        {
            // PDF 对象是特例：以 #reference 结尾表示引用外部文档，而不是内嵌内容
            QString reqExt = element.attribute(aReqExt);
            if (reqExt == vReqExt)
            { // pdf reference
                QString ref  = element.attribute(aHref);
                int     i    = ref.indexOf("#page");
                QString dest = ref.replace(i, ref.length() - i, "");
                if (!QFileInfo::exists(dest))
                {
                    cureNCopy(dest, false);
                }
                if (ref.isEmpty())
                {
                    return;
                }
                ref.replace(QRegularExpression("^(.*pdf\\#page\\=).*$"), QString("\\1%1").arg(mToIndex));
                return;
            }

            QString type = element.attribute(aType);
            if (type == vText)
            { // 文本对象这里不用管，它走自己的序列化通道
                if (element.hasAttribute(aActionMedia))
                {
                    QString newRelative = cureNCopy(element.attribute(aActionMedia));
                    element.setAttribute(aActionMedia, newRelative);
                }
                return;
            }
            QString newRelative = cureNCopy(element.attribute(aSrc));
            element.setAttribute(aSrc, newRelative);
        }
        else if (tagName == tTeacherGuide)
        {
            QDomElement nMediaElement = element.firstChildElement(tMedia);
            while (!nMediaElement.isNull())
            {
                QString newRelative = cureNCopy(nMediaElement.attribute(aRelativePath));
                nMediaElement.setAttribute(aRelativePath, newRelative);
                nMediaElement = nMediaElement.nextSiblingElement(tMedia);
            }
        }
    }

    QString cureNCopy(const QString &relativePath, bool createNewUuid = true)
    {
        QString relative = relativePath;
        if (createNewUuid)
        {
            QUuid   newUuid = QUuid::createUuid();
            QString newPath = relative.replace(QRegularExpression("\\{.*\\}"), newUuid.toString());

            cp_rf(mFromDir + "/" + relativePath, mToDir + "/" + newPath);

            return newPath;
        }
        else
        {
            cp_rf(mFromDir + "/" + relativePath, mToDir + "/" + relativePath);
            return relativePath;
        }
    }

private:
    QString mFromDir;
    QString mToDir;
    int     mFromIndex;
    int     mToIndex;
};

class ForeighnObjectsHandlerPrivate
{
    ForeighnObjectsHandlerPrivate(ForeighnObjectsHandler *pq) : q(pq)
    {
    }

public:
    void cure(const QUrl &dir)
    {
        Cleaner *cleaner = new Cleaner;
        cleaner->cure(dir);
        delete cleaner;
        cleaner = 0;
    }

    void copyPage(const QUrl &fromDir, int fromIndex, const QUrl &toDir, int toIndex)
    {
        PageCopier *copier = new PageCopier;
        copier->copyPage(fromDir, fromIndex, toDir, toIndex);
        delete copier;
        copier = 0;
    }

private:
    ForeighnObjectsHandler *q;
    friend class ForeighnObjectsHandler;
};

ForeighnObjectsHandler::ForeighnObjectsHandler() : d(new ForeighnObjectsHandlerPrivate(this))
{
}

ForeighnObjectsHandler::~ForeighnObjectsHandler()
{
    delete d;
}

void ForeighnObjectsHandler::cure(const QList<QUrl> &dirs)
{
    foreach (QUrl dir, dirs)
    {
        cure(dir);
    }
}

void ForeighnObjectsHandler::cure(const QUrl &dir)
{
    d->cure(dir);
}

void ForeighnObjectsHandler::copyPage(const QUrl &fromDir, int fromIndex, const QUrl &toDir, int toIndex)
{
    d->copyPage(fromDir, fromIndex, toDir, toIndex);
}
