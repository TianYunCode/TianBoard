#pragma once

#include <QtCore>
#include <QGraphicsSceneDragDropEvent>

#include "api/WebWidgetAPI.h"
#include "core/DownloadManager.h"

class GraphicsScene;
class GraphicsWidgetItem;
class GraphicsWebWidgetItem;

class WebWidgetMessageAPI;
class DatastoreAPI;
class DocumentDatastoreAPI;

class WebWidgetBoardAPI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int pageCount READ pageCount SCRIPTABLE true)

    Q_PROPERTY(int currentPageNumber READ currentPageNumber SCRIPTABLE true)

    Q_PROPERTY(QString uuid READ uuid SCRIPTABLE true)
    
    Q_PROPERTY(QString lang READ lang SCRIPTABLE true)

    Q_PROPERTY(QObject* messages READ messages SCRIPTABLE true)

    Q_PROPERTY(QObject* datastore READ datastore SCRIPTABLE true)

    public:
        WebWidgetBoardAPI(GraphicsScene *pScene, GraphicsWidgetItem *widget = 0);
        ~WebWidgetBoardAPI();

        QObject* messages();

        QObject* datastore();

    public slots:
        void setScene(GraphicsScene* pScene)
        {
            mScene = pScene;
        }

        void setTool(const QString& toolString);

        void setPenColor(const QString& penColor);

        void setMarkerColor(const QString& penColor);

        QString pageThumbnail(const int pageNumber);

        void zoom(const qreal factor, const qreal x, const qreal y);

        void move(const qreal x, const qreal y);

        void moveTo(const qreal x, const qreal y);

        void drawLineTo(const qreal x, const qreal y, const qreal pWidth);

        void eraseLineTo(const qreal x, const qreal y, const qreal pWidth);

        void clear();

        void setBackground(bool pIsDark, bool pIsCrossed);

        void addObject(QString pUrl, int width = 0, int height = 0, int x = 0, int y = 0, bool background = false);

        void resize(qreal width, qreal height);

        QString locale();

        void setPreference(const QString& key, QString value);

        QString preference(const QString& key, const QString& pDefault = QString());

        QStringList preferenceKeys();

        void showMessage(const QString& message);

        void centerOn(const qreal x, const qreal y);

        void addText(const QString& text, const qreal x, const qreal y, const int height = -1, const QString& font = ""
                , bool bold = false, bool italic = false);

        void returnStatus(const QString& method, const QString& status);
        void usedMethods(QStringList methods);
        void response(bool correct);

        void sendFileMetadata(QString metaData);

        void enableDropOnWidget (bool enable = true);

        void ProcessDropEvent(QGraphicsSceneDragDropEvent *);
        bool isDropableData(const QMimeData *pMimeData) const;

	private slots:
        void onDownloadFinished(bool pSuccess, sDownloadFileDesc desc, QByteArray pData);

	private:
        inline void registerIDWidget(int id){webDownloadIds.append(id);}
        inline bool takeIDWidget(int id);

    private:
        QString uuid();

        QString lang();

        int pageCount();

        int currentPageNumber();
        QString getObjDir();
        QString createMimeText(bool downloaded, const QString &mimeType, const QString &fileName);
        bool supportedTypeHeader(const QString &) const;
        QString boolToStr(bool value) const {return value ? "true" : "false";}

        GraphicsScene* mScene;

        GraphicsWidgetItem* mGraphicsWidget;

        bool mIsVisible;

        WebWidgetMessageAPI* mMessagesAPI;

        DatastoreAPI* mDatastoreAPI;
        QList<int> webDownloadIds;
};

class DatastoreAPI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QObject* document READ document SCRIPTABLE true)

    public:
        DatastoreAPI(GraphicsWebWidgetItem *widget);
        virtual ~DatastoreAPI(){;}

        QObject* document();

    private:
        DocumentDatastoreAPI* mDocumentDatastore;

};

class DocumentDatastoreAPI : public WebWidgetStorage
{
    Q_OBJECT

    public:
        DocumentDatastoreAPI(GraphicsWebWidgetItem *graphicsWidget);

        virtual ~DocumentDatastoreAPI();

    public slots:
        virtual QString key(int index);
        virtual QString getItem(const QString& key);
        virtual void setItem(const QString& key, const QString& value);
        virtual void removeItem(const QString& key);
        virtual void clear();

    protected:
        virtual int length();

    private:
        GraphicsWebWidgetItem* mGraphicsWebWidget;

};

