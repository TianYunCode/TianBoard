#include "api/WebWidgetBoardAPI.h"

#include <QWebEngineView>
#include <QDomDocument>
#include <QtWidgets>

#include "core/Application.h"
#include "settings/Settings.h"

#include "model/DocumentProxy.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsWidgetItem.h"

#include "content/ThumbnailAdaptor.h"

#include "api/WebWidgetMessageAPI.h"
#include "utils/FileSystemUtils.h"
#include "core/DownloadManager.h"

// 已知的受支持后缀，遇到新格式时在这里补充
const QString audioExtentions = ".mp3.wma.ogg";
const QString videoExtentions = ".avi.flv";
const QString imageExtentions = ".png.jpg.tif.bmp.tga";
const QString htmlExtentions  = ".htm.html.xhtml";

// 一律使用别名常量，避免直接使用 const char*
const QString imageAlias      = "image";
const QString imageAliasCap   = "Image";
const QString videoAlias      = "video";
const QString videoAliasCap   = "Video";
const QString audioAlias      = "audio";
const QString audioAliasCap   = "Audio";

// MIME 元数据的 XML 标签名
const QString tMainSection    = "mimedata";
const QString tType           = "type";
const QString tPath           = "path";
const QString tMessage        = "message";
const QString tReady          = "ready";

const QString tMimeText       = "text/plain";

// 小部件内部存放对象文件的子目录名
const QString objectsPath     = "objects";

WebWidgetBoardAPI::WebWidgetBoardAPI(GraphicsScene *pScene, GraphicsWidgetItem *widget)
    : QObject(pScene), mScene(pScene), mGraphicsWidget(widget), mIsVisible(false), mMessagesAPI(0), mDatastoreAPI(0)
{
    // 只有网页小部件才需要脚本通道与文档存储，普通图形项只保留场景门面
    GraphicsWebWidgetItem *webGraphicsWidget = dynamic_cast<GraphicsWebWidgetItem *>(widget);

    if (webGraphicsWidget)
    {
        mMessagesAPI  = new WebWidgetMessageAPI(webGraphicsWidget);
        mDatastoreAPI = new DatastoreAPI(webGraphicsWidget);
    }

    // 只听全局下载完成事件，再用 webDownloadIds 判断是否属于本小部件
    connect(DownloadManager::downloadManager(), qOverload<bool, sDownloadFileDesc, QByteArray>(&DownloadManager::downloadFinished), this,
        &WebWidgetBoardAPI::onDownloadFinished);
}

WebWidgetBoardAPI::~WebWidgetBoardAPI()
{
    // 刻意留空
}

QObject *WebWidgetBoardAPI::messages()
{
    return mMessagesAPI;
}

QObject *WebWidgetBoardAPI::datastore()
{
    return mDatastoreAPI;
}

void WebWidgetBoardAPI::setTool(const QString &toolString)
{
    if (Application::boardController->activeScene() != mScene)
    {
        return;
    }

    const QString lower = toolString.toLower();

    if (lower == "pen")
    {
        DrawingController::drawingController()->setStylusTool(StylusTool::Pen);
    }
    else if (lower == "marker")
    {
        DrawingController::drawingController()->setStylusTool(StylusTool::Marker);
    }
    else if (lower == "arrow")
    {
        DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
    }
    else if (lower == "play")
    {
        DrawingController::drawingController()->setStylusTool(StylusTool::Play);
    }
    else if (lower == "line")
    {
        DrawingController::drawingController()->setStylusTool(StylusTool::Line);
    }
}

void WebWidgetBoardAPI::setPenColor(const QString &penColor)
{
    if (Application::boardController->activeScene() != mScene)
    {
        return;
    }

    Settings *settings   = Settings::settings();

    bool conversionState = false;

    // 脚本传 1..4 时按调色板序号取色；减 1 后必须在 1..4 之间才视为序号
    int index            = penColor.toInt(&conversionState) - 1;

    if (conversionState && index > 0 && index <= 4)
    {
        Application::boardController->setPenColorOnDarkBackground(settings->penColors(true).at(index - 1));
        Application::boardController->setPenColorOnLightBackground(settings->penColors(false).at(index - 1));
    }
    else
    {
        QColor svgColor;
        svgColor = QColor::fromString(penColor);
        if (svgColor.isValid())
        {
            Application::boardController->setPenColorOnDarkBackground(svgColor);
            Application::boardController->setPenColorOnLightBackground(svgColor);
        }
    }
}

void WebWidgetBoardAPI::setMarkerColor(const QString &penColor)
{
    if (Application::boardController->activeScene() != mScene)
    {
        return;
    }

    Settings *settings   = Settings::settings();

    bool conversionState = false;

    int index            = penColor.toInt(&conversionState);

    if (conversionState && index > 0 && index <= 4)
    {
        Application::boardController->setMarkerColorOnDarkBackground(settings->markerColors(true).at(index - 1));
        Application::boardController->setMarkerColorOnLightBackground(settings->markerColors(false).at(index - 1));
    }
    else
    {
        QColor svgColor;
        svgColor = QColor::fromString(penColor);
        if (svgColor.isValid())
        {
            Application::boardController->setMarkerColorOnDarkBackground(svgColor);
            Application::boardController->setMarkerColorOnLightBackground(svgColor);
        }
    }
}

void WebWidgetBoardAPI::addObject(QString pUrl, int width, int height, int x, int y, bool background)
{
    // 脚本调用只给 URL，真正的下载与落盘交给 BoardController；
    // 宽高为 0 时由控制器按对象类型选择默认尺寸

    if (Application::boardController->activeScene() != mScene)
    {
        return;
    }

    Application::boardController->downloadURL(QUrl(pUrl), QString(), QPointF(x, y), QSize(width, height), background);
}

void WebWidgetBoardAPI::setBackground(bool pIsDark, bool pIsCrossed)
{
    if (mScene)
    {
        if (pIsCrossed)
        {
            mScene->setBackground(pIsDark, PageBackground::crossed);
        }
        else
        {
            mScene->setBackground(pIsDark, PageBackground::plain);
        }
    }
}

void WebWidgetBoardAPI::moveTo(const qreal x, const qreal y)
{
    if (qIsNaN(x) || qIsNaN(y) || qIsInf(x) || qIsInf(y))
    {
        return;
    }

    if (mScene)
    {
        mScene->moveTo(QPointF(x, y));
    }
}

void WebWidgetBoardAPI::drawLineTo(const qreal x, const qreal y, const qreal pWidth)
{
    if (qIsNaN(x) || qIsNaN(y) || qIsNaN(pWidth) || qIsInf(x) || qIsInf(y) || qIsInf(pWidth))
    {
        return;
    }

    if (mScene)
    {
        mScene->drawLineTo(QPointF(x, y), pWidth, DrawingController::drawingController()->stylusTool() == StylusTool::Line);
    }
}

void WebWidgetBoardAPI::eraseLineTo(const qreal x, const qreal y, const qreal pWidth)
{
    if (qIsNaN(x) || qIsNaN(y) || qIsNaN(pWidth) || qIsInf(x) || qIsInf(y) || qIsInf(pWidth))
    {
        return;
    }

    if (mScene)
    {
        mScene->eraseLineTo(QPointF(x, y), pWidth);
    }
}

void WebWidgetBoardAPI::clear()
{
    if (mScene)
    {
        mScene->clearContent(GraphicsScene::clearItemsAndAnnotations);
    }
}

void WebWidgetBoardAPI::zoom(const qreal factor, const qreal x, const qreal y)
{
    if (qIsNaN(factor) || qIsNaN(x) || qIsNaN(y) || qIsInf(factor) || qIsInf(x) || qIsInf(y))
    {
        return;
    }

    if (Application::boardController->activeScene() != mScene)
    {
        return;
    }

    Application::boardController->zoom(factor, QPointF(x, y));
}

void WebWidgetBoardAPI::centerOn(const qreal x, const qreal y)
{
    if (qIsNaN(x) || qIsNaN(y) || qIsInf(x) || qIsInf(y))
    {
        return;
    }

    if (Application::boardController->activeScene() != mScene)
    {
        return;
    }

    Application::boardController->centerOn(QPointF(x, y));
}

void WebWidgetBoardAPI::move(const qreal x, const qreal y)
{
    if (qIsNaN(x) || qIsNaN(y) || qIsInf(x) || qIsInf(y))
    {
        return;
    }

    if (Application::boardController->activeScene() != mScene)
    {
        return;
    }

    Application::boardController->handScroll(x, y);
}

void WebWidgetBoardAPI::addText(const QString &text, const qreal x, const qreal y, const int size, const QString &font, bool bold, bool italic)
{
    if (qIsNaN(x) || qIsNaN(y) || qIsInf(x) || qIsInf(y))
    {
        return;
    }

    if (Application::boardController->activeScene() != mScene)
    {
        return;
    }

    if (mScene)
    {
        mScene->addTextWithFont(text, QPointF(x, y), size, font, bold, italic);
    }
}

int WebWidgetBoardAPI::pageCount()
{
    if (mScene && mScene->document())
    {
        return mScene->document()->pageCount();
    }
    else
    {
        return -1;
    }
}

int WebWidgetBoardAPI::currentPageNumber()
{
    if (Application::boardController->activeScene() != mScene)
    {
        return -1;
    }

    return Application::boardController->activeSceneIndex() + 1;
}

QString WebWidgetBoardAPI::getObjDir()
{
    // 对象文件与网页资源放在小部件自己的目录下，保证文档打包时可整体搬走
    return mGraphicsWidget->getOwnFolder().toLocalFile() + "/" + objectsPath + "/";
}

void WebWidgetBoardAPI::showMessage(const QString &message)
{
    Application::boardController->showMessage(message, false);
}

QString WebWidgetBoardAPI::pageThumbnail(const int pageNumber)
{
    if (Application::boardController->activeScene() != mScene)
    {
        return "";
    }

    DocumentProxy *doc = Application::boardController->selectedDocument();

    if (!doc)
    {
        return "";
    }

    // 页码从 1 开始，缩略图适配器从 0 开始
    if (pageNumber > doc->pageCount())
    {
        return "";
    }

    QUrl url = ThumbnailAdaptor::thumbnailUrl(doc, pageNumber - 1);

    return url.toString();
}

void WebWidgetBoardAPI::resize(qreal width, qreal height)
{
    if (qIsNaN(width) || qIsNaN(height) || qIsInf(width) || qIsInf(height))
    {
        return;
    }

    if (mGraphicsWidget)
    {
        mGraphicsWidget->resize(width, height);
    }
}

void WebWidgetBoardAPI::setPreference(const QString &key, QString value)
{
    if (mGraphicsWidget)
    {
        mGraphicsWidget->setPreference(key, value);
    }
}

QString WebWidgetBoardAPI::preference(const QString &key, const QString &pDefault)
{
    if (mGraphicsWidget && mGraphicsWidget->preferences().contains(key))
    {
        return mGraphicsWidget->preference(key);
    }
    else
    {
        return pDefault;
    }
}

QStringList WebWidgetBoardAPI::preferenceKeys()
{
    QStringList keys;

    if (mGraphicsWidget)
    {
        keys = mGraphicsWidget->preferences().keys();
    }

    return keys;
}

QString WebWidgetBoardAPI::uuid()
{
    if (mGraphicsWidget)
    {
        return StringUtils::toCanonicalUuid(mGraphicsWidget->uuid());
    }
    else
    {
        return "";
    }
}

QString WebWidgetBoardAPI::locale()
{
    return QLocale().name();
}

QString WebWidgetBoardAPI::lang()
{
    QString lang = QLocale().name();

    if (lang.length() > 2)
    {
        lang[2] = QLatin1Char('-');
    }

    return lang;
}

void WebWidgetBoardAPI::returnStatus(const QString &method, const QString &status)
{
    QString msg = QString(tr("%0 called (method=%1, status=%2)")).arg("returnStatus").arg(method).arg(status);
    Application::showMessage(msg);
}

void WebWidgetBoardAPI::usedMethods(QStringList methods)
{
    // TODO: 统计脚本实际调用过的方法，目前接口保留但无实现
    foreach (QString method, methods)
    {
    }
}

void WebWidgetBoardAPI::response(bool correct)
{
    Q_UNUSED(correct);
    // TODO: 接入答题结果反馈，目前接口保留但无实现
}

void WebWidgetBoardAPI::sendFileMetadata(QString metaData)
{
    // 元数据以 XML 传入，逐个子节点取出 key/value 后交给控制器展示
    QMap<QString, QString> qmMetaDatas;
    QDomDocument           domDoc;
    domDoc.setContent(metaData);
    QDomElement  rootElem = domDoc.documentElement();
    QDomNodeList children = rootElem.childNodes();
    for (int i = 0; i < children.size(); i++)
    {
        QDomNode    dataNode        = children.at(i);
        QDomElement keyElem         = dataNode.firstChildElement("key");
        QDomElement valueElem       = dataNode.firstChildElement("value");
        qmMetaDatas[keyElem.text()] = valueElem.text();
    }
    Application::boardController->displayMetaData(qmMetaDatas);
}

void WebWidgetBoardAPI::enableDropOnWidget(bool enable)
{
    if (mGraphicsWidget)
    {
        mGraphicsWidget->setAcceptDrops(enable);
    }
}

void WebWidgetBoardAPI::processDropEvent(QGraphicsSceneDragDropEvent *event)
{
    const QMimeData *pMimeData = event->mimeData();

    QString destFileName;
    QString contentType;
    bool    downloaded                     = false;

    Qt::DropActions       dropActions      = event->possibleActions();
    Qt::MouseButtons      dropMouseButtons = event->buttons();
    Qt::KeyboardModifiers dropModifiers    = event->modifiers();

    // 原来这里是 `new QMimeData` 交给 event->setMimeData()——QDropEvent 不接管所有权，
    // 每放一次文件就漏一块；栈上的对象活到本函数结束就够了，落点之后没人再读这份数据。
    QMimeData dropMimeData;

    if (pMimeData->hasHtml())
    { // 从浏览器拖入的网页元素：解析出其中的 URL 再下载
        QString qsHtml = pMimeData->html();
        QString url    = Application::urlFromHtml(qsHtml);

        if (!url.isEmpty())
        {
            sDownloadFileDesc desc;
            desc.dest             = sDownloadFileDesc::graphicsWidget;
            desc.modal            = true;
            desc.srcUrl           = url;
            desc.currentSize      = 0;
            desc.name             = QFileInfo(url).fileName();
            desc.totalSize        = 0;                      // 总大小在下载过程中才能得知

            desc.dropPoint        = event->pos().toPoint(); // 直接使用事件坐标，不做转换
            desc.dropActions      = dropActions;
            desc.dropMouseButtons = dropMouseButtons;
            desc.dropModifiers    = dropModifiers;

            registerIDWidget(DownloadManager::downloadManager()->addFileToDownload(desc));
        }
    }
    else if (pMimeData->hasUrls())
    { // 本地文件：支持的类型直接复制进小部件目录
        QUrl    curUrl = pMimeData->urls().first();
        QString sUrl   = curUrl.toString();

        if (sUrl.startsWith("file://") || sUrl.startsWith("/"))
        {
            QString fileName  = curUrl.toLocalFile();
            QString extention = FileSystemUtils::extension(fileName);
            contentType       = FileSystemUtils::mimeTypeFromFileName(fileName);

            if (supportedTypeHeader(contentType))
            {
                destFileName = getObjDir() + QUuid::createUuid().toString() + "." + extention;

                if (!FileSystemUtils::copyFile(fileName, destFileName))
                {
                    qDebug() << "can't copy from" << fileName << "to" << destFileName;
                    return;
                }
                downloaded = true;
            }
        }
    }
    QString mimeText = createMimeText(downloaded, contentType, destFileName);
    dropMimeData.setData(tMimeText, mimeText.toLatin1());

    event->setMimeData(&dropMimeData);
}

void WebWidgetBoardAPI::onDownloadFinished(bool pSuccess, sDownloadFileDesc desc, QByteArray pData)
{
    // 只处理本小部件发起、且还在等待中的下载；其余事件直接忽略
    if (!takeIDWidget(desc.id))
    {
        return;
    }

    if (!pSuccess)
    {
        qDebug() << "can't download the whole data. An error occured";
        return;
    }

    QString contentType = desc.contentTypeHeader;
    QString extention   = FileSystemUtils::fileExtensionFromMimeType(contentType);

    if (!supportedTypeHeader(contentType))
    {
        qDebug() << "actions for mime type" << contentType << "are not supported";
        return;
    }

    QString objDir = getObjDir();
    if (!QDir().exists(objDir))
    {
        if (!QDir().mkpath(objDir))
        {
            qDebug() << "can't create objects directory path. Check the permissions";
            return;
        }
    }

    QString destFileName = objDir + QUuid::createUuid().toString() + "." + extention;
    QFile   destFile(destFileName);

    if (!destFile.open(QIODevice::WriteOnly))
    {
        qDebug() << "can't open" << destFileName << "for wrighting";
        return;
    }

    if (destFile.write(pData) == -1)
    {
        qDebug() << "can't implement data writing";
        return;
    }

    QGraphicsView *tmpView = mGraphicsWidget->scene()->views().at(0);
    QPoint         dropPoint(mGraphicsWidget->mapFromScene(tmpView->mapToScene(desc.dropPoint)).toPoint());

    QMimeData dropMimeData;
    QString   mimeText = createMimeText(true, contentType, destFileName);
    dropMimeData.setData(tMimeText, mimeText.toLatin1());

    destFile.close();

    // 页面脚本只认完整的拖放序列，先补一个 move 事件再发 drop
    QDragMoveEvent pseudoMove(dropPoint, desc.dropActions, &dropMimeData, desc.dropMouseButtons, desc.dropModifiers);
    QApplication::sendEvent(mGraphicsWidget, &pseudoMove);

    QDropEvent readyEvent(dropPoint, desc.dropActions, &dropMimeData, desc.dropMouseButtons, desc.dropModifiers);
    // 无论下载成功与否都通知目标，避免页面停在等待状态
    QApplication::sendEvent(mGraphicsWidget, &readyEvent);
    readyEvent.acceptProposedAction();
}

QString WebWidgetBoardAPI::createMimeText(bool downloaded, const QString &mimeType, const QString &fileName)
{
    QString          mimeXml;
    QXmlStreamWriter writer(&mimeXml);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement(tMainSection);

    writer.writeTextElement(tReady, boolToStr(downloaded));

    if (downloaded)
    {
        if (!mimeType.isEmpty())
        {
            writer.writeTextElement(tType, mimeType); // 写入对象类型
        }
        if (!QFile::exists(fileName))
        {
            qDebug() << "file" << fileName << "doesn't exist";
            return QString();
        }

        QString relatedFileName = fileName;
        // 路径必须相对小部件目录，文档整体迁移后仍然有效
        relatedFileName         = relatedFileName.remove(mGraphicsWidget->getOwnFolder().toLocalFile() + "/");
        writer.writeTextElement(tPath, relatedFileName); // 写入新建对象的相对路径
    }

    writer.writeEndElement();
    writer.writeEndDocument();

    return mimeXml;
}

bool WebWidgetBoardAPI::supportedTypeHeader(const QString &typeHeader) const
{
    return typeHeader.startsWith(imageAlias) || typeHeader.startsWith(imageAliasCap) || typeHeader.startsWith(audioAlias) ||
           typeHeader.startsWith(audioAliasCap) || typeHeader.startsWith(videoAlias) || typeHeader.startsWith(videoAliasCap);
}

bool WebWidgetBoardAPI::takeIDWidget(int id)
{
    if (webDownloadIds.contains(id))
    {
        webDownloadIds.removeAll(id);
        return true;
    }
    return false;
}

bool WebWidgetBoardAPI::isDropableData(const QMimeData *pMimeData) const
{
    QString fileName = QString();
    if (pMimeData->hasHtml())
    {
        fileName = Application::urlFromHtml(pMimeData->html());
        if (fileName.isEmpty())
        {
            return false;
        }
    }
    else if (pMimeData->hasUrls())
    {
        fileName = pMimeData->urls().at(0).toLocalFile();
        if (fileName.isEmpty())
        {
            return false;
        }
    }

    if (supportedTypeHeader(FileSystemUtils::mimeTypeFromFileName(fileName)))
    {
        return true;
    }

    return false;
}

DocumentDatastoreAPI::DocumentDatastoreAPI(GraphicsWebWidgetItem *graphicsWidget)
    : WebWidgetStorage(graphicsWidget), mGraphicsWebWidget(graphicsWidget)
{
    // 刻意留空：数据全部保存在宿主图形对象里
}

DocumentDatastoreAPI::~DocumentDatastoreAPI()
{
    // 刻意留空
}

QString DocumentDatastoreAPI::key(int index)
{
    // QMap 按 key 排序，因此 storage 的枚举顺序是稳定的字典序
    QMap<QString, QString> entries = mGraphicsWebWidget->datastoreEntries();

    if (index < entries.size())
    {
        return entries.keys().at(index);
    }
    else
    {
        return "";
    }
}

QString DocumentDatastoreAPI::getItem(const QString &key)
{
    QMap<QString, QString> entries = mGraphicsWebWidget->datastoreEntries();
    if (entries.contains(key))
    {
        return entries.value(key);
    }
    else
    {
        return "";
    }
}

int DocumentDatastoreAPI::length()
{
    return mGraphicsWebWidget->datastoreEntries().size();
}

void DocumentDatastoreAPI::setItem(const QString &key, const QString &value)
{
    if (mGraphicsWebWidget)
    {
        mGraphicsWebWidget->setDatastoreEntry(key, value);
    }
}

void DocumentDatastoreAPI::removeItem(const QString &key)
{
    mGraphicsWebWidget->removeDatastoreEntry(key);
}
void

 DocumentDatastoreAPI::clear()
{
    mGraphicsWebWidget->removeAllDatastoreEntries();
}

DatastoreAPI::DatastoreAPI(GraphicsWebWidgetItem *widget) : QObject(widget)
{
    // 以宿主小部件为 parent，保证存储实现随小部件一起销毁
    mDocumentDatastore = new DocumentDatastoreAPI(widget);
}

QObject *DatastoreAPI::document()
{
    return mDocumentDatastore;
}
