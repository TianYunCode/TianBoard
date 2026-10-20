#pragma once

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEngineView>
#include <QDomElement>
#include <QGraphicsView>
#include <QGraphicsWidget>
#include "utils/Enums.h"

#include "whiteboard/Item.h"
#include "whiteboard/ResizableGraphicsItem.h"

class BoardDropHandler;
class GraphicsScene;

struct WidgetType {
    enum Enum { Web = 0, Apple, Other };
};

class GraphicsWidgetItem : public QGraphicsWidget,
                           public Item,
                           public ResizableGraphicsItem,
                           public GraphicsItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem);

public:
    GraphicsWidgetItem(const QUrl &pWidgetUrl = QUrl(), QGraphicsItem *parent = 0);
    ~GraphicsWidgetItem();

    enum { Type = GraphicsItemType::GraphicsWidgetItemType };

    virtual int type() const { return Type; }

    virtual void initialize();

    virtual void resize(qreal w, qreal h);
    virtual void resize(const QSizeF &size);
    virtual QSizeF size() const;

    QUrl mainHtml();
    void loadMainHtml();
    QUrl widgetUrl();
    void widgetUrl(QUrl url) { mWidgetUrl = url; }
    QString mainHtmlFileName();

    bool canBeContent();
    bool canBeTool();

    QString preference(const QString &key) const;
    void setPreference(const QString &key, QString value);
    QMap<QString, QString> preferences() const;
    void removePreference(const QString &key);
    void removeAllPreferences();

    QString datastoreEntry(const QString &key) const;
    void setDatastoreEntry(const QString &key, QString value);
    QMap<QString, QString> datastoreEntries() const;
    void removeDatastoreEntry(const QString &key);
    void removeAllDatastoreEntries();

    void removeScript();

    void processDropEvent(QGraphicsSceneDragDropEvent *event);

    virtual QUrl getOwnFolder() const;
    virtual void setOwnFolder(const QUrl &newFolder);
    virtual void setSnapshotPath(const QUrl &newFilePath);
    virtual QUrl getSnapshotPath();

    virtual void clearSource();

    virtual void setUuid(const QUuid &pUuid);

    QSize nominalSize() const;

    bool hasLoadedSuccessfully() const;

    bool freezable();
    bool resizable();
    bool isFrozen();

    QPixmap snapshot();
    void setSnapshot(const QPixmap &pix);
    QPixmap takeSnapshot();

    virtual Item *deepCopy() const = 0;
    virtual GraphicsScene *scene();

    static int widgetType(const QUrl &pUrl);
    static QString widgetName(const QUrl &pUrl);
    static QString iconFilePath(const QUrl &pUrl);

public slots:
    void freeze();
    void unFreeze();

protected:
    enum OSType {
        type_NONE = 0, // 0000
        type_WIN = 1,  // 0001
        type_MAC = 2,  // 0010
        type_UNIX = 4, // 0100
        type_ALL = 7   // 0111
    };

    bool mFirstReleaseAfterMove;
    bool mInitialLoadDone;
    bool mIsFreezable;
    bool mIsResizable;
    bool mLoadIsErronous;
    bool mMouseIsPressed;
    int mCanBeContent;
    int mCanBeTool;
    QSize mNominalSize;
    QString mMainHtmlFileName;
    QUrl mMainHtmlUrl;
    QUrl mWidgetUrl;
    QMap<QString, QString> mDatastore;
    QMap<QString, QString> mPreferences;

    virtual bool event(QEvent *event);
    virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void sendJSEnterEvent();
    virtual void sendJSLeaveEvent();
    virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
    //virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

protected slots:
    void geometryChangeRequested(const QRect &geom);
    void mainFrameLoadFinished(bool ok);

private slots:
    void onLinkClicked(const QUrl &url);
    void initialLayoutCompleted();

private:
    bool mIsFrozen;
    bool mIsTakingSnapshot;
    bool mShouldMoveWidget;
    BoardDropHandler *mDropHandler;
    QPixmap mSnapshot;
    QPointF mLastMousePos;
    QUrl ownFolder;
    QUrl SnapshotFile;
};

class GraphicsAppleWidgetItem : public GraphicsWidgetItem {
    Q_OBJECT

public:
    GraphicsAppleWidgetItem(const QUrl &pWidgetUrl, QGraphicsItem *parent = 0);
    ~GraphicsAppleWidgetItem();

    virtual void copyItemParameters(Item *copy) const;
    virtual void setUuid(const QUuid &pUuid);
    virtual Item *deepCopy() const;
    QRectF boundingRect() const override;
};

class GraphicsWebWidgetItem : public GraphicsWidgetItem {
    Q_OBJECT

public:
    class PreferenceValue {
    public:
        PreferenceValue() {}

        PreferenceValue(const QString &pValue, bool pReadonly) {
            value = pValue;
            readonly = pReadonly;
        }

        bool readonly;
        QString value;
    };

    class Metadata {
    public:
        QString id;
        QString name;
        QString description;
        QString author;
        QString authorEmail;
        QString authorHref;
        QString version;
    };

    GraphicsWebWidgetItem(const QUrl &pWidgetUrl, QGraphicsItem *parent = 0);
    ~GraphicsWebWidgetItem();

    virtual void setUuid(const QUuid &pUuid);
    virtual Item *deepCopy() const;
    virtual void copyItemParameters(Item *copy) const;
    QMap<QString, PreferenceValue> preferences();
    Metadata metadatas() const;
    QRectF boundingRect() const override;

    static QString freezedWidgetFilePath();
    static QString createNPAPIWrapper(const QString &url, const QString &pMimeType = QString(),
                                      const QSize &sizeHint = QSize(300, 150),
                                      const QString &pName = QString());
    static QString createNPAPIWrapperInDir(const QString &url, const QDir &pDir,
                                           const QString &pMimeType = QString(),
                                           const QSize &sizeHint = QSize(300, 150),
                                           const QString &pName = QString());
    static QString createHtmlWrapperInDir(const QString &html, const QDir &pDir, const QSize &sizeHint,
                                          const QString &pName);
    static QString freezedWidgetPage();
    static bool hasNPAPIWrapper(const QString &pMimeType);

    Metadata mMetadatas;

private:
    static void loadNPAPIWrappersTemplates();
    static QString textForSubElementByLocale(QDomElement rootElement, QString subTagName, QLocale locale);

    QMap<QString, PreferenceValue> mPreferences;

    static bool sTemplateLoaded;
    static QString sNPAPIWrappperConfigTemplate;
    static QMap<QString, QString> sNPAPIWrapperTemplates;
};
