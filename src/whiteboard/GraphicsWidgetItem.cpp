#include <QtNetwork>
#include <QtXml>
#include <QtWebChannel/QWebChannel>

#include "whiteboard/GraphicsWidgetItem.h"
#include "whiteboard/BoardDropHandler.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsItemDelegate.h"
#include "whiteboard/GraphicsWidgetItemDelegate.h"
#include "whiteboard/GraphicsDelegateFrame.h"

#include "whiteboard/BoardController.h"

#include "core/ApplicationController.h"
#include "core/Application.h"
#include "settings/Settings.h"

#include "utils/FileSystemUtils.h"

#include "core/NetworkAccessManager.h"

#include "web/WebKitUtils.h"
#include "web/WebController.h"

GraphicsWidgetItem::GraphicsWidgetItem(const QUrl &pWidgetUrl, QGraphicsItem *parent)
    : QGraphicsWidget(parent),
      mInitialLoadDone(false),
      mIsFreezable(true),
      mIsResizable(false),
      mLoadIsErronous(false),
      mCanBeContent(0),
      mCanBeTool(0),
      mWidgetUrl(pWidgetUrl),
      mIsFrozen(false),
      mIsTakingSnapshot(false),
      mShouldMoveWidget(false),
      mDropHandler(nullptr) {
    setData(
        GraphicsItemData::ItemLayerType,
        QVariant(itemLayerType::ObjectItem)); //Necessary to set if we want z value to be assigned correctly
                                              //QWebEngineView
                                              //QGraphicsWidget::setGraphicsEffect
                                              //   QGraphicsView::setPage(new WebPage(this));
    //QGraphicsWidget::setAttribute(QWebEngineSettings::JavaEnabled, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::LocalStorageDatabaseEnabled, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::OfflineWebApplicationCacheEnabled, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::OfflineStorageDatabaseEnabled, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    //QGraphicsWebView::settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    //page()->setNetworkAccessManager(NetworkAccessManager::defaultAccessManager());

    setAcceptDrops(true);
    setAutoFillBackground(false);

    //QPalette pagePalette = page()->palette();
    //pagePalette.setBrush(QPalette::Base, QBrush(Qt::transparent));
    //pagePalette.setBrush(QPalette::Window, QBrush(Qt::transparent));
    //page()->setPalette(pagePalette);

    QPalette viewPalette = palette();
    //pagePalette.setBrush(QPalette::Base, QBrush(Qt::transparent));
    viewPalette.setBrush(QPalette::Window, QBrush(Qt::transparent));
    setPalette(viewPalette);

    setDelegate(new GraphicsWidgetItemDelegate(this));

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    QGraphicsWidget::setAcceptHoverEvents(true);
}


GraphicsWidgetItem::~GraphicsWidgetItem() {
    /* NOOP */
}

void GraphicsWidgetItem::initialize() {
    setMinimumSize(nominalSize());
    setData(
        GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::ObjectItem)); // Necessary to set if we want z value to be assigned correctly

    if (Delegate() && Delegate()->frame() && resizable())
        Delegate()->frame()->setOperationMode(GraphicsDelegateFrame::Resizing);

    /* QPalette palette = page()->palette();
    palette.setBrush(QPalette::Base, QBrush(Qt::transparent));
    page()->setPalette(palette);
    page()->setLinkDelegationPolicy(QWebEnginePage::DelegateAllLinks);*/

    //connect(page(), SIGNAL(geometryChangeRequested(const QRect&)), this, SLOT(geometryChangeRequested(const QRect&)));
    //connect(this, SIGNAL(loadFinished(bool)), this, SLOT(mainFrameLoadFinished (bool)));
    //connect(page()->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SLOT(initialLayoutCompleted()));
    //connect(page(), SIGNAL(linkClicked(const QUrl&)), this, SLOT(onLinkClicked(const QUrl&)));
}

void GraphicsWidgetItem::onLinkClicked(const QUrl &url) {
    //load(url);
}

void GraphicsWidgetItem::initialLayoutCompleted() {
    mInitialLoadDone = true;
}

QUrl GraphicsWidgetItem::mainHtml() {
    return mMainHtmlUrl;
}

void GraphicsWidgetItem::loadMainHtml() {
    mInitialLoadDone = false;
    //load(mMainHtmlUrl);
}

QUrl GraphicsWidgetItem::widgetUrl() {
    return mWidgetUrl;
}

QString GraphicsWidgetItem::mainHtmlFileName() {
    return mMainHtmlFileName;
}

bool GraphicsWidgetItem::canBeContent() {
// if we under MAC OS
#if defined(Q_OS_MAC)
    return mCanBeContent & GraphicsWidgetItem::type_MAC;
#endif

// if we under UNIX OS
#if defined(Q_OS_UNIX)
    return mCanBeContent & GraphicsWidgetItem::type_UNIX;
#endif

// if we under WINDOWS OS
#if defined(Q_OS_WIN)
    return mCanBeContent & GraphicsWidgetItem::type_WIN;
#endif
}

bool GraphicsWidgetItem::canBeTool() {
// if we under MAC OS
#if defined(Q_OS_MAC)
    return mCanBeTool & GraphicsWidgetItem::type_MAC;
#endif

    // if we under UNIX OS
#if defined(Q_OS_UNIX)
    return mCanBeTool & GraphicsWidgetItem::type_UNIX;
#endif

    // if we under WINDOWS OS
#if defined(Q_OS_WIN)
    return mCanBeTool & GraphicsWidgetItem::type_WIN;
#endif
}

QString GraphicsWidgetItem::preference(const QString &key) const {
    return mPreferences.value(key);
}

void GraphicsWidgetItem::setPreference(const QString &key, QString value) {
    if (key == "" || (mPreferences.contains(key) && mPreferences.value(key) == value))
        return;

    mPreferences.insert(key, value);
    if (scene())
        scene()->setModified(true);
}

QMap<QString, QString> GraphicsWidgetItem::preferences() const {
    return mPreferences;
}


void GraphicsWidgetItem::removePreference(const QString &key) {
    mPreferences.remove(key);
}


void GraphicsWidgetItem::removeAllPreferences() {
    mPreferences.clear();
}

QString GraphicsWidgetItem::datastoreEntry(const QString &key) const {
    if (mDatastore.contains(key))
        return mDatastore.value(key);
    else
        return QString();
}

void GraphicsWidgetItem::setDatastoreEntry(const QString &key, QString value) {
    if (key == "" || (mDatastore.contains(key) && mDatastore.value(key) == value))
        return;

    mDatastore.insert(key, value);
    if (scene())
        scene()->setModified(true);
}

QMap<QString, QString> GraphicsWidgetItem::datastoreEntries() const {
    return mDatastore;
}


void GraphicsWidgetItem::removeDatastoreEntry(const QString &key) {
    mDatastore.remove(key);
}


void GraphicsWidgetItem::removeAllDatastoreEntries() {
    mDatastore.clear();
}

void GraphicsWidgetItem::removeScript() {
    //if (page() && page()->mainFrame())
    //    page()->mainFrame()->evaluateJavaScript("if(widget && widget.onremove) { widget.onremove();}");
}

void GraphicsWidgetItem::processDropEvent(QGraphicsSceneDragDropEvent *event) {
    // 处理拖放的是 api 的门面（它要 DownloadManager 与文档对象目录），画板只认
    // BoardDropHandler 这个接口；实现由装配点在启动时装上。
    // 原来 mBoardAPI 只在 javaScriptWindowObjectCleared() 里建，而 Qt6 的
    // QWebEnginePage 没有那个信号去触发它 —— 拖放落到小部件上就是一次空指针。
    if (!mDropHandler)
        mDropHandler = createBoardDropHandler(scene(), this);

    if (mDropHandler)
        mDropHandler->processDropEvent(event);
}

QUrl GraphicsWidgetItem::getOwnFolder() const {
    return ownFolder;
}

void GraphicsWidgetItem::setOwnFolder(const QUrl &newFolder) {
    ownFolder = newFolder;
}

void GraphicsWidgetItem::setSnapshotPath(const QUrl &newFilePath) {
    SnapshotFile = newFilePath;
}

QUrl GraphicsWidgetItem::getSnapshotPath() {
    return SnapshotFile;
}

void GraphicsWidgetItem::clearSource() {
    FileSystemUtils::deleteDir(getOwnFolder().toLocalFile());
    FileSystemUtils::deleteFile(getSnapshotPath().toLocalFile());
}

void GraphicsWidgetItem::setUuid(const QUuid &pUuid) {
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid,
            QVariant(
                pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

QSize GraphicsWidgetItem::nominalSize() const {
    return mNominalSize;
}

bool GraphicsWidgetItem::hasLoadedSuccessfully() const {
    return (mInitialLoadDone && !mLoadIsErronous);
}

bool GraphicsWidgetItem::freezable() {
    return mIsFreezable;
}

bool GraphicsWidgetItem::resizable() {
    return mIsResizable;
}

bool GraphicsWidgetItem::isFrozen() {
    return mIsFrozen;
}

QPixmap GraphicsWidgetItem::snapshot() {
    return mSnapshot;
}

QPixmap GraphicsWidgetItem::takeSnapshot() {
    mIsTakingSnapshot = true;

    QPixmap pixmap(size().toSize());
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);

    QStyleOptionGraphicsItem options;
    paint(&painter, &options);

    mIsTakingSnapshot = false;

    mSnapshot = pixmap;

    return pixmap;
}

void GraphicsWidgetItem::setSnapshot(const QPixmap &pix) {
    mSnapshot = pix;
}

GraphicsScene *GraphicsWidgetItem::scene() {
    return qobject_cast<GraphicsScene *>(QGraphicsItem::scene());
}

int GraphicsWidgetItem::widgetType(const QUrl &pUrl) {
    QString mime = FileSystemUtils::mimeTypeFromFileName(pUrl.toString());

    if (mime == "application/vnd.apple-widget")
        return WidgetType::Apple;
    else if (mime == "application/widget")
        return WidgetType::Web;
    else
        return WidgetType::Other;
}

QString GraphicsWidgetItem::widgetName(const QUrl &widgetPath) {
    QString name;
    QString version;
    QFile widgetConfigFile(widgetPath.toLocalFile() + "/config.xml");
    QFile appleConfigFile(widgetPath.toLocalFile() + "/Info.plist");

    if (widgetConfigFile.exists() && widgetConfigFile.open(QFile::ReadOnly)) {
        QDomDocument doc;
        doc.setContent(widgetConfigFile.readAll());
        QDomElement root = doc.firstChildElement("widget");
        if (!root.isNull()) {
            QDomElement nameElement = root.firstChildElement("name");
            if (!nameElement.isNull())
                name = nameElement.text();
            version = root.attribute("version", "");
        }
        widgetConfigFile.close();
    } else if (appleConfigFile.exists() && appleConfigFile.open(QFile::ReadOnly)) {
        QDomDocument doc;
        doc.setContent(appleConfigFile.readAll());
        QDomElement root = doc.firstChildElement("plist");
        if (!root.isNull()) {
            QDomElement dictElement = root.firstChildElement("dict");
            if (!dictElement.isNull()) {
                QDomNodeList childNodes = dictElement.childNodes();

                /* looking for something like
                 * ..
                 * <key>CFBundleDisplayName</key>
                 * <string>brain scans</string>
                 * ..
                 */

                for (int i = 0; i < childNodes.count() - 1; i++) {
                    if (childNodes.at(i).isElement()) {
                        QDomElement elKey = childNodes.at(i).toElement();
                        if (elKey.text() == "CFBundleDisplayName") {
                            if (childNodes.at(i + 1).isElement()) {
                                QDomElement elValue = childNodes.at(i + 1).toElement();
                                name = elValue.text();
                            }
                        } else if (elKey.text() == "CFBundleShortVersionString") {
                            if (childNodes.at(i + 1).isElement()) {
                                QDomElement elValue = childNodes.at(i + 1).toElement();
                                version = elValue.text();
                            }
                        }
                    }
                }
            }
        }
        appleConfigFile.close();
    }
    QString result;

    if (name.length() > 0) {
        result = name;
        if (version.length() > 0) {
            result += " ";
            result += version;
        }
    }
    return result;
}

QString GraphicsWidgetItem::iconFilePath(const QUrl &pUrl) {
    QStringList files;

    files << "icon.svg"; /* W3C widget default 1 */
    files << "icon.ico"; /* W3C widget default 2 */
    files << "icon.png"; /* W3C widget default 3 */
    files << "icon.gif"; /* W3C widget default 4 */
    files << "Icon.png"; /* Apple widget default */

    QString file = FileSystemUtils::getFirstExistingFileFromList(pUrl.toLocalFile(), files);
    /* default */
    if (file.length() == 0) {
        file = QString(":/images/defaultWidgetIcon.png");
    }
    return file;
}

void GraphicsWidgetItem::freeze() {
    QPixmap pix = takeSnapshot();
    mIsFrozen = true;
    setSnapshot(pix);
}

void GraphicsWidgetItem::unFreeze() {
    mIsFrozen = false;
}

bool GraphicsWidgetItem::event(QEvent *event) {
    if (mShouldMoveWidget && event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseMoveEvent = static_cast<QMouseEvent *>(event);
        if (mouseMoveEvent->buttons() & Qt::LeftButton) {
            QPointF scenePos = mapToScene(mouseMoveEvent->pos());
            QPointF newPos = pos() + scenePos - mLastMousePos;
            setPos(newPos);
            mLastMousePos = scenePos;
            event->accept();
            return true;
        }
    } else if (event->type() == QEvent::ShortcutOverride)
        event->accept();

    return true; //QGraphicsWebView::event(event);
}

void GraphicsWidgetItem::dropEvent(QGraphicsSceneDragDropEvent *event) {
    processDropEvent(event);
    //QGraphicsWebView::dropEvent(event);
}

void GraphicsWidgetItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (!Delegate()->mousePressEvent(event))
        setSelected(true); /* forcing selection */

    QGraphicsWidget::mousePressEvent(event);

    // did webkit consume the mouse press ?
    mShouldMoveWidget = !event->isAccepted() && (event->buttons() & Qt::LeftButton);

    mLastMousePos = mapToScene(event->pos());

    event->accept();
}

void GraphicsWidgetItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    mShouldMoveWidget = false;

    Delegate()->mouseReleaseEvent(event);
    QGraphicsWidget::mouseReleaseEvent(event);
}

void GraphicsWidgetItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    sendJSEnterEvent();
    Delegate()->hoverEnterEvent(event);
}
void GraphicsWidgetItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    sendJSLeaveEvent();
    Delegate()->hoverLeaveEvent(event);
}

void GraphicsWidgetItem::sendJSEnterEvent() {
    //if (page() && page()->mainFrame())
    //    page()->mainFrame()->evaluateJavaScript("if(widget && widget.onenter) { widget.onenter();}");
}

void GraphicsWidgetItem::sendJSLeaveEvent() {
    //if (page() && page()->mainFrame())
    //    page()->mainFrame()->evaluateJavaScript("if(widget && widget.onleave) { widget.onleave();}");
}

void GraphicsWidgetItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    //QGraphicsView::paint(painter, option, widget);


    if (!mInitialLoadDone) {
        QString message;

        message = tr("Loading ...");

        painter->setFont(QFont("Arial", 12));

        QFontMetrics fm = painter->fontMetrics();
        QRect txtBoundingRect = fm.boundingRect(message);

        //txtBoundingRect.moveCenter(rect().center().toPoint());
        txtBoundingRect.adjust(-10, -5, 10, 5);

        painter->setPen(Qt::NoPen);
        painter->setBrush(Settings::paletteColor);
        painter->drawRoundedRect(txtBoundingRect, 3, 3);

        painter->setPen(Qt::white);
        //painter->drawText(rect(), Qt::AlignCenter, message);
    }

    Delegate()->postpaint(painter, option, widget);
}

void GraphicsWidgetItem::geometryChangeRequested(const QRect &geom) {
    resize(geom.width(), geom.height());
}

void GraphicsWidgetItem::mainFrameLoadFinished(bool ok) {
    mLoadIsErronous = !ok;
    //update(boundingRect());

    if (mInitialLoadDone && scene() && scene()->renderingContext() == GraphicsScene::Screen)
        takeSnapshot();
}

void GraphicsWidgetItem::wheelEvent(QGraphicsSceneWheelEvent *event) {
    if (Delegate()->wheelEvent(event)) {
        //QGraphicsView::wheelEvent(event);
        event->accept();
    }
}

//QVariant GraphicsWidgetItem::itemChange(GraphicsItemChange change, const QVariant &value)
//{
//    if ((change == QGraphicsItem::ItemSelectedHasChanged) &&  scene()) {
//        if (isSelected())
//            scene()->setActiveWindow(this);
//        else
//            if(scene()->activeWindow() == this)
//                scene()->setActiveWindow(0);
//    }
//
//    QVariant newValue = Delegate()->itemChange(change, value);
//    return QGraphicsView::itemChange(change, newValue);
//}

void GraphicsWidgetItem::resize(qreal w, qreal h) {
    GraphicsWidgetItem::resize(QSizeF(w, h));
}


void GraphicsWidgetItem::resize(const QSizeF &pSize) {
    if (pSize != size()) {
        //QGraphicsWidget::setMaximumSize(pSize.width(), pSize.height());
        //QGraphicsWidget::resize(pSize.width(), pSize.height());
        //      if (Delegate())
        //          Delegate()->positionHandles();
        //      if (scene())
        //          scene()->setModified(true);
    }
}

QSizeF GraphicsWidgetItem::size() const {
    QSizeF sz;
    return sz; //QGraphicsWidget::size();
}


GraphicsAppleWidgetItem::GraphicsAppleWidgetItem(const QUrl &pWidgetUrl, QGraphicsItem *parent)
    : GraphicsWidgetItem(pWidgetUrl, parent) {
    QString path = pWidgetUrl.toLocalFile();

    if (!path.endsWith(".wdgt") && !path.endsWith(".wdgt/")) {
        int lastSlashIndex = path.lastIndexOf("/");
        if (lastSlashIndex > 0)
            path = path.mid(0, lastSlashIndex + 1);
    }

    QFile plistFile(path + "/Info.plist");
    plistFile.open(QFile::ReadOnly);

    QByteArray plistBin = plistFile.readAll();
    QString plist = QString::fromUtf8(plistBin);

    int mainHtmlIndex = plist.indexOf("MainHTML");
    int mainHtmlIndexStart = plist.indexOf("<string>", mainHtmlIndex);
    int mainHtmlIndexEnd = plist.indexOf("</string>", mainHtmlIndexStart);

    if (mainHtmlIndex > -1 && mainHtmlIndexStart > -1 && mainHtmlIndexEnd > -1)
        mMainHtmlFileName = plist.mid(mainHtmlIndexStart + 8, mainHtmlIndexEnd - mainHtmlIndexStart - 8);

    mMainHtmlUrl = pWidgetUrl;
    mMainHtmlUrl.setPath(pWidgetUrl.path() + "/" + mMainHtmlFileName);

    //load(mMainHtmlUrl);

    QPixmap defaultPixmap(pWidgetUrl.toLocalFile() + "/Default.png");

    //setMaximumSize(defaultPixmap.size());

    mNominalSize = defaultPixmap.size();

    initialize();
}


GraphicsAppleWidgetItem::~GraphicsAppleWidgetItem() {
    /* NOOP */
}

void GraphicsAppleWidgetItem::setUuid(const QUuid &pUuid) {
    Item::setUuid(pUuid);
    //setData(GraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

Item *GraphicsAppleWidgetItem::deepCopy() const {
    GraphicsAppleWidgetItem *appleWidget = new GraphicsAppleWidgetItem(QUrl(), parentItem());

    copyItemParameters(appleWidget);

    return appleWidget;
}

void GraphicsAppleWidgetItem::copyItemParameters(Item *copy) const {
    GraphicsAppleWidgetItem *cp = dynamic_cast<GraphicsAppleWidgetItem *>(copy);
    if (cp) {
        foreach (QString key, mPreferences.keys()) {
            cp->setPreference(key, mPreferences.value(key));
        }

        foreach (QString key, mDatastore.keys()) {
            cp->setDatastoreEntry(key, mDatastore.value(key));
        }

        cp->setSourceUrl(this->sourceUrl());
        //cp->setZValue(this->zValue());
    }
}


QRectF GraphicsAppleWidgetItem::boundingRect() const {
    qreal penWidth = 1;
    return QRectF(0 - penWidth / 2, 0 - penWidth / 2, 20 + penWidth, 20 + penWidth);
}


bool GraphicsWebWidgetItem::sTemplateLoaded = false;
QString GraphicsWebWidgetItem::sNPAPIWrappperConfigTemplate;
QMap<QString, QString> GraphicsWebWidgetItem::sNPAPIWrapperTemplates;

GraphicsWebWidgetItem::GraphicsWebWidgetItem(const QUrl &pWidgetUrl, QGraphicsItem *parent)
    : GraphicsWidgetItem(pWidgetUrl, parent) {
    QString path = pWidgetUrl.toLocalFile();
    QDir potentialDir(path);

    if (!path.endsWith(".wgt") && !path.endsWith(".wgt/") && !potentialDir.exists()) {
        int lastSlashIndex = path.lastIndexOf("/");
        if (lastSlashIndex > 0)
            path = path.mid(0, lastSlashIndex + 1);
    }

    if (!path.endsWith("/"))
        path += "/";

    int width = 300;
    int height = 150;

    QFile configFile(path + "config.xml");
    configFile.open(QFile::ReadOnly);

    QDomDocument doc;
    doc.setContent(configFile.readAll());
    QDomNodeList widgetDomList = doc.elementsByTagName("widget");

    if (widgetDomList.count() > 0) {
        QDomElement widgetElement = widgetDomList.item(0).toElement();

        width = widgetElement.attribute("width", "300").toInt();
        height = widgetElement.attribute("height", "150").toInt();

        mMetadatas.id = widgetElement.attribute("id", "");

        if (mMetadatas.id.length() == 0)
            mMetadatas.id = widgetElement.attribute("identifier", "");

        mMetadatas.version = widgetElement.attribute("version", "");

        mIsResizable = widgetElement.attribute("tb:resizable", "false") == "true";
        mIsFreezable = widgetElement.attribute("tb:freezable", "true") == "true";

        QString roles = widgetElement.attribute("tb:roles", "content tool").trimmed().toLower();

        if (roles == "" || roles.contains("tool"))
            mCanBeTool = GraphicsWidgetItem::type_ALL;

        if (roles.contains("twin"))
            mCanBeTool |= GraphicsWidgetItem::type_WIN;

        if (roles.contains("tmac"))
            mCanBeTool |= GraphicsWidgetItem::type_MAC;

        if (roles.contains("tunix"))
            mCanBeTool |= GraphicsWidgetItem::type_UNIX;

        /* --------- */

        if (roles == "" || roles.contains("content"))
            mCanBeContent = GraphicsWidgetItem::type_ALL;

        if (roles.contains("cwin"))
            mCanBeContent |= GraphicsWidgetItem::type_WIN;

        if (roles.contains("cmac"))
            mCanBeContent |= GraphicsWidgetItem::type_MAC;

        if (roles.contains("cunix"))
            mCanBeContent |= GraphicsWidgetItem::type_UNIX;

        //------------------------------//

        QDomNodeList contentDomList = widgetElement.elementsByTagName("content");

        if (contentDomList.count() > 0) {
            QDomElement contentElement = contentDomList.item(0).toElement();
            mMainHtmlFileName = contentElement.attribute("src", "");
        }

        mMetadatas.name = textForSubElementByLocale(widgetElement, "name", QLocale::system());
        mMetadatas.description = textForSubElementByLocale(widgetElement, "description ", QLocale::system());

        QDomNodeList authorDomList = widgetElement.elementsByTagName("author");

        if (authorDomList.count() > 0) {
            QDomElement authorElement = authorDomList.item(0).toElement();

            mMetadatas.author = authorElement.text();
            mMetadatas.authorHref = authorElement.attribute("href", "");
            mMetadatas.authorEmail = authorElement.attribute("email ", "");
        }

        QDomNodeList propertiesDomList = widgetElement.elementsByTagName("preference");

        for (int i = 0; i < propertiesDomList.length(); i++) {
            QDomElement preferenceElement = propertiesDomList.at(i).toElement();
            QString prefName = preferenceElement.attribute("name", "");

            if (prefName.length() > 0) {
                QString prefValue = preferenceElement.attribute("value", "");
                bool readOnly = (preferenceElement.attribute("readonly", "false") == "true");

                mPreferences.insert(prefName, PreferenceValue(prefValue, readOnly));
            }
        }
    }

    if (mMainHtmlFileName.length() == 0) {
        QFile defaultStartFile(path + "index.htm");

        if (defaultStartFile.exists())
            mMainHtmlFileName = "index.htm";
        else {
            QFile secondDefaultStartFile(path + "index.html");

            if (secondDefaultStartFile.exists())
                mMainHtmlFileName = "index.html";
        }
    }

    mMainHtmlUrl = pWidgetUrl;
    mMainHtmlUrl.setPath(pWidgetUrl.path() + "/" + mMainHtmlFileName);
    /* is it a valid local file ? */
    QFile f(mMainHtmlUrl.toLocalFile());

    if (!f.exists())
        mMainHtmlUrl = QUrl(mMainHtmlFileName);

    //load(mMainHtmlUrl);

    //setMaximumSize(QSize(width, height));

    mNominalSize = QSize(width, height);

    initialize();
    setOwnFolder(pWidgetUrl);
}

GraphicsWebWidgetItem::~GraphicsWebWidgetItem() {
    /* NOOP */
}

void GraphicsWebWidgetItem::setUuid(const QUuid &pUuid) {
    Item::setUuid(pUuid);
    //setData(GraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

Item *GraphicsWebWidgetItem::deepCopy() const {
    GraphicsWebWidgetItem *copy = new GraphicsWebWidgetItem(mWidgetUrl, parentItem());
    copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable
    copyItemParameters(copy);

    return copy;
}

QMap<QString, GraphicsWebWidgetItem::PreferenceValue> GraphicsWebWidgetItem::preferences() {
    return mPreferences;
}

GraphicsWebWidgetItem::Metadata GraphicsWebWidgetItem::metadatas() const {
    return mMetadatas;
}

QRectF GraphicsWebWidgetItem::boundingRect() const {
    qreal penWidth = 1;
    return QRectF(0 - penWidth / 2, 0 - penWidth / 2, 200 + penWidth, 200 + penWidth);
}

QString GraphicsWebWidgetItem::createNPAPIWrapper(const QString &url, const QString &pMimeType,
                                                  const QSize &sizeHint, const QString &pName) {
    const QString userWidgetPath = Settings::settings()->userInteractiveDirectory() + "/" + tr("Web");
    QDir userWidgetDir(userWidgetPath);

    return createNPAPIWrapperInDir(url, userWidgetDir, pMimeType, sizeHint, pName);
}

QString GraphicsWebWidgetItem::createNPAPIWrapperInDir(const QString &pUrl, const QDir &pDir,
                                                       const QString &pMimeType, const QSize &sizeHint,
                                                       const QString &pName) {
    QString url = pUrl;
    url = FileSystemUtils::removeLocalFilePrefix(url);
    QString name = pName;

    QFileInfo fi(url);

    if (name.length() == 0)
        name = fi.baseName();

    if (fi.exists())
        url = fi.fileName();

    loadNPAPIWrappersTemplates();

    QString htmlTemplate;

    if (pMimeType.length() > 0 && sNPAPIWrapperTemplates.contains(pMimeType))
        htmlTemplate = sNPAPIWrapperTemplates.value(pMimeType);
    else {
        QString extension = FileSystemUtils::extension(url);
        if (sNPAPIWrapperTemplates.contains(extension))
            htmlTemplate = sNPAPIWrapperTemplates.value(extension);
    }

    if (htmlTemplate.length() > 0) {
        htmlTemplate = htmlTemplate.replace(QString("{in.url}"), url)
                           .replace(QString("{in.width}"), QString("%1").arg(sizeHint.width()))
                           .replace(QString("{in.height}"), QString("%1").arg(sizeHint.height()));

        QString configTemplate = sNPAPIWrappperConfigTemplate.replace(QString("{in.id}"), url)
                                     .replace(QString("{in.width}"), QString("%1").arg(sizeHint.width()))
                                     .replace(QString("{in.height}"), QString("%1").arg(sizeHint.height()))
                                     .replace(QString("{in.name}"), name)
                                     .replace(QString("{in.startFile}"), QString("index.htm"));

        QString dirPath = pDir.path();
        if (!pDir.exists())
            pDir.mkpath(dirPath);

        QString widgetLibraryPath = dirPath + "/" + name + ".wgt";
        QDir widgetLibraryDir(widgetLibraryPath);

        if (widgetLibraryDir.exists())
            if (!FileSystemUtils::deleteDir(widgetLibraryDir.path()))
                qWarning() << "Cannot delete old widget " << widgetLibraryDir.path();

        widgetLibraryDir.mkpath(widgetLibraryPath);
        if (fi.exists()) {
            QString target = widgetLibraryPath + "/" + fi.fileName();
            QString source = pUrl;
            source = FileSystemUtils::removeLocalFilePrefix(source);
            QFile::copy(source, target);
        }

        QFile configFile(widgetLibraryPath + "/config.xml");

        if (!configFile.open(QIODevice::WriteOnly)) {
            qWarning() << "Cannot open file " << configFile.fileName();
            return QString();
        }

        QTextStream outConfig(&configFile);
        outConfig.setEncoding(QStringConverter::Utf8);

        outConfig << configTemplate;
        configFile.close();

        QFile indexFile(widgetLibraryPath + "/index.htm");

        if (!indexFile.open(QIODevice::WriteOnly)) {
            qWarning() << "Cannot open file " << indexFile.fileName();
            return QString();
        }

        QTextStream outIndex(&indexFile);
        outIndex.setEncoding(QStringConverter::Utf8);

        outIndex << htmlTemplate;
        indexFile.close();

        return widgetLibraryPath;
    } else
        return QString();
}

QString GraphicsWebWidgetItem::createHtmlWrapperInDir(const QString &html, const QDir &pDir,
                                                      const QSize &sizeHint, const QString &pName) {
    QString widgetPath = pDir.path() + "/" + pName + ".wgt";
    widgetPath = FileSystemUtils::nextAvailableFileName(widgetPath);
    QDir widgetDir(widgetPath);

    if (!widgetDir.exists())
        widgetDir.mkpath(widgetDir.path());

    QFile configFile(widgetPath + "/" + "config.xml");

    if (configFile.exists())
        configFile.remove(configFile.fileName());

    if (!configFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open file " << configFile.fileName();
        return "";
    }

    QTextStream outConfig(&configFile);
    outConfig.setEncoding(QStringConverter::Utf8);
    outConfig << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << Qt::endl;
    outConfig << "<widget xmlns=\"http://www.w3.org/ns/widgets\"" << Qt::endl;
    outConfig << "    xmlns:tb=\"http://www.tianboard.org/2026/widgets\"" << Qt::endl;
    outConfig << "    id=\"http://www.tianboard.org/" << pName << "\"" << Qt::endl;

    outConfig << "    version=\"1.0\"" << Qt::endl;
    outConfig << "    width=\"" << sizeHint.width() << "\"" << Qt::endl;
    outConfig << "    height=\"" << sizeHint.height() << "\"" << Qt::endl;
    outConfig << "    tb:resizable=\"true\">" << Qt::endl;

    outConfig << "  <name>" << pName << "</name>" << Qt::endl;
    outConfig << "  <content src=\"" << pName << ".html\"/>" << Qt::endl;

    outConfig << "</widget>" << Qt::endl;

    configFile.close();

    const QString fullHtmlFileName = widgetPath + "/" + pName + ".html";

    QFile widgetHtmlFile(fullHtmlFileName);
    if (widgetHtmlFile.exists())
        widgetHtmlFile.remove(widgetHtmlFile.fileName());
    if (!widgetHtmlFile.open(QIODevice::WriteOnly)) {
        qWarning() << "cannot open file " << widgetHtmlFile.fileName();
        return QString();
    }

    QTextStream outStartFile(&widgetHtmlFile);
    outStartFile.setEncoding(QStringConverter::Utf8);

    outStartFile
        << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">"
        << Qt::endl;
    outStartFile << "<html>" << Qt::endl;
    outStartFile << "<head>" << Qt::endl;
    outStartFile << "    <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">" << Qt::endl;
    outStartFile << "</head>" << Qt::endl;
    outStartFile << "  <body>" << Qt::endl;
    outStartFile << html << Qt::endl;
    outStartFile << "  </body>" << Qt::endl;
    outStartFile << "</html>" << Qt::endl;

    widgetHtmlFile.close();

    return widgetPath;
}

QString GraphicsWebWidgetItem::freezedWidgetPage() {
    static QString defaultcontent;

    if (defaultcontent.isNull()) {
        QString freezedWidgetDefaultContentFilePath = freezedWidgetFilePath();
        QFile wrapperFile(freezedWidgetDefaultContentFilePath);
        if (!wrapperFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "can't open wrapper file " + freezedWidgetDefaultContentFilePath;
            defaultcontent = "";
        } else {
            QByteArray arr = wrapperFile.readAll();
            if (!arr.isEmpty())
                defaultcontent = QString(arr);
            else {
                qDebug() << "content of " + freezedWidgetDefaultContentFilePath + "is empty";
                defaultcontent = QString();
            }
        }
    }

    return defaultcontent;
}

QString GraphicsWebWidgetItem::freezedWidgetFilePath() {
    return FileSystemUtils::applicationResourcesDirectory() + "/etc/" + "freezedWidgetWrapper.html";
}

bool GraphicsWebWidgetItem::hasNPAPIWrapper(const QString &pMimeType) {
    loadNPAPIWrappersTemplates();

    return sNPAPIWrapperTemplates.contains(pMimeType);
}

void GraphicsWebWidgetItem::loadNPAPIWrappersTemplates() {
    if (!sTemplateLoaded) {
        sNPAPIWrapperTemplates.clear();

        QString etcPath = FileSystemUtils::applicationResourcesDirectory() + "/etc/";

        QDir etcDir(etcPath);

        foreach (QString fileName, etcDir.entryList()) {
            if (fileName.startsWith("npapi-wrapper") &&
                (fileName.endsWith(".htm") || fileName.endsWith(".html"))) {
                QString htmlContent = FileSystemUtils::readTextFile(etcPath + fileName);

                if (htmlContent.length() > 0) {
                    QStringList tokens = fileName.split(".");

                    if (tokens.length() >= 4) {
                        QString mime = tokens.at(tokens.length() - 4);
                        mime += "/" + tokens.at(tokens.length() - 3);

                        QString fileExtension = tokens.at(tokens.length() - 2);

                        sNPAPIWrapperTemplates.insert(mime, htmlContent);
                        sNPAPIWrapperTemplates.insert(fileExtension, htmlContent);
                    }
                }
            }
        }
        sNPAPIWrappperConfigTemplate = FileSystemUtils::readTextFile(etcPath + "npapi-wrapper.config.xml");
        sTemplateLoaded = true;
    }
}

QString GraphicsWebWidgetItem::textForSubElementByLocale(QDomElement rootElement, QString subTagName,
                                                         QLocale locale) {
    QDomNodeList subList = rootElement.elementsByTagName(subTagName);

    QString lang = locale.name();

    if (lang.length() > 2)
        lang[2] = QLatin1Char('-');

    if (subList.count() > 1) {
        for (int i = 0; i < subList.count(); i++) {
            QDomNode node = subList.at(i);
            QDomElement element = node.toElement();

            QString configLang = element.attribute("xml:lang", "");

            if (lang == configLang || (configLang.length() == 2 && configLang == lang.left(2)))
                return element.text();
        }
    }

    if (subList.count() >= 1) {
        QDomElement element = subList.item(0).toElement();
        return element.text();
    }

    return QString();
}

void GraphicsWebWidgetItem::copyItemParameters(Item *copy) const {
    GraphicsWebWidgetItem *cp = dynamic_cast<GraphicsWebWidgetItem *>(copy);
    if (cp) {
        /*cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));
        cp->setData(GraphicsItemData::ItemLocked, this->data(GraphicsItemData::ItemLocked));*/
        cp->setSourceUrl(this->sourceUrl());

        cp->resize(this->size());

        foreach (QString key, this->GraphicsWidgetItem::preferences().keys()) {
            cp->setPreference(key, GraphicsWidgetItem::preferences().value(key));
        }

        foreach (QString key, mDatastore.keys()) {
            cp->setDatastoreEntry(key, mDatastore.value(key));
        }

        //cp->setZValue(this->zValue());
    }
}
