#pragma once

#include <QRegExp>
#include <QRegularExpression>

#include <QWidget>
#include <QListView>
#include <QGraphicsView>
#include <QAbstractListModel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QSlider>
#include <QUrl>
#include <QStyledItemDelegate>
#include <QLocale>
#include <QGraphicsLinearLayout>
#include <QStackedWidget>
#include <QDropEvent>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QTreeWidget>

#include <memory>

#include "ui/DockPaletteWidget.h"
#include "settings/Settings.h"
#include "whiteboard/FeaturesController.h"
#include "api/WebWidgetBoardAPI.h"
#include "ui/FeaturesActionBar.h"
#include "ui/RubberBand.h"
#include <QtWebEngineWidgets>
#include <QWebEngineView>
#include <QWebEngineSettings>

#define THUMBNAIL_WIDTH 400
#define ID_LISTVIEW 0
#define ID_PROPERTIES 1
#define ID_WEBVIEW 2

class ListModel;

class FeaturesModel;
class FeaturesItemDelegate;
class FeaturesPathItemDelegate;
class FeaturesProxyModel;
class FeaturesSearchProxyModel;
class FeaturesPathProxyModel;
class FeaturesPathViewer;
class FeatureProperties;
class FeatureItemButton;
class FeaturesListView;
class FeaturesWebWidget;
class FeaturesNavigatorWidget;
class FeaturesMimeData;
class FeaturesCentralWidget;
class FeaturesNewFolderDialog;
class FeaturesProgressBar;

class FeaturesWidget : public DockPaletteWidget
{
    Q_OBJECT
public:
    FeaturesWidget(QWidget* parent=0, const char* name="FeaturesWidget");
    virtual ~FeaturesWidget();

    bool visibleInMode(DockWidgetMode mode)
    {
        return mode == DockWidgetMode::Board
            || mode == DockWidgetMode::Desktop;
    }
    FeaturesController * getFeaturesController() const;
    void importImage(const QImage &image, const QString &fileName = QString());

    static const int minThumbnailSize = 20;
    static const int maxThumbnailSize = 100;
    static const int defaultThumbnailSize = 40;

    static const char *objNamePathList;
    static const char *objNameFeatureList;

public:
    int scrollbarHorisontalPadding() const { return 10;}
    int scrollbarVerticalIndent() const { return 0;}

signals:
    void sendFileNameList(const QStringList lst);

private slots:
    void onPreviewLoaded(int id, bool pSuccess, QUrl sourceUrl, QUrl originalUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void currentSelected( const QModelIndex & );
    void searchStarted( const QString & );
    void createNewFolder();
    void deleteElements( const FeaturesMimeData * );
    void addToFavorite( const FeaturesMimeData  *);
    void removeFromFavorite( const FeaturesMimeData * );
    void onDisplayMetadata( QMap<QString,QString> );
    void onAddDownloadedFileToLibrary(bool, QUrl, QString, QByteArray, QString pTitle);
    void addElementsToFavorite();
    void removeElementsFromFavorite();
    void deleteSelectedElements();
    void rescanModel();
    void lockIt(bool pLock);

private:
    void switchToListView();
    void switchToProperties();
    void switchToWebView();

    struct Private;
    std::unique_ptr<Private> d;
};

class FeaturesMimeData : public QMimeData
{
    Q_OBJECT

public:
    FeaturesMimeData();
    virtual ~FeaturesMimeData();

    virtual QStringList formats() const;
    QList<Feature> features() const;
    void setFeatures(const QList<Feature> &fList);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class FeaturesListView : public QListView
{
    Q_OBJECT

public:
    FeaturesListView( QWidget* parent=0, const char* name="FeaturesListView" );
    virtual ~FeaturesListView() {;}

protected:
    virtual void dragEnterEvent( QDragEnterEvent *event );
    virtual void dropEvent( QDropEvent *event );
    virtual void dragMoveEvent( QDragMoveEvent *event );

public slots:
    // FeaturesNavigatorWidget 的滑块连它；跨类连接必须公开
    void thumbnailSizeChanged(int);

};

class FeaturesNavigatorWidget: public QWidget
{
    Q_OBJECT

public:
    FeaturesNavigatorWidget(QWidget *parent, const char* name = "");
    FeaturesListView *listView();
    void setSliderPosition(int pValue);

private:
    struct Private;
    std::unique_ptr<Private> d;

};

class FeaturesCentralWidget : public QWidget
{
  Q_OBJECT

public:
    enum StackElement{
        MainList = 0,
        FeaturePropertiesList,
        FeaturesWebView
    };

    enum AddWidget {
        NewFolderDialog = 0,
        ProgressBarWidget
    };

    enum AddWidgetState {
        NonModal = 0,
        Modal
    };

    FeaturesCentralWidget(QWidget *parent = 0);
    void setSliderPosition(int pValue);

    FeaturesListView *listView();
    void showElement(const Feature &feature, StackElement pView);
    void switchTo(StackElement pView);
    void setPropertiesPixmap(const QPixmap &pix);
    void setPropertiesThumbnail(const QPixmap &pix);
    StackElement currentView() const;
    Feature getCurElementFromProperties();

    void setLockedExcludingAdditional(bool pLock);

signals:
    void lockMainWidget(bool pLock);
    void createNewFolderSignal(QString pStr);
    void sendFileNameList(const QStringList lst);

//    progressbar widget related signals
    void maxFilesCountEvaluated(int pValue);
    void increaseStatusBarValue();
    void scanCategory(const QString &);
    void scanPath(const QString &);

public slots:
    void showAdditionalData(AddWidget pWidgetType, AddWidgetState pState = NonModal);

    // FeaturesController 的扫描信号连这两个；跨类连接必须公开
    void scanStarted();
    void scanFinished();

private slots:
    void createNewFolderSlot(QString pStr);
    void hideAdditionalData();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class FeaturesNewFolderDialog : public QWidget
{
    Q_OBJECT

public:
    FeaturesNewFolderDialog(QWidget *parent = 0);
    void setRegexp(const QRegularExpression &pRegExp);
    bool validString(const QString &pStr);

signals:
    void createNewFolder(QString str);
    void closeDialog();

private slots:
    void accept();
    void reject();
    void reactOnTextChanged(const QString &pStr);

public slots:
    // FeaturesCentralWidget 转发文件名列表；跨类连接必须公开
    void setFileNameList(const QStringList &pLst);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class FeaturesProgressInfo: public QWidget {
    Q_OBJECT

public:
    FeaturesProgressInfo(QWidget *parent = 0);

public slots:
    // FeaturesCentralWidget 转发扫描进度；跨类连接必须公开
    void setCommonInfoText(const QString &str);
    void setDetailedInfoText(const QString &str);
    void setProgressMax(int pValue);
    void increaseProgressValue();

private slots:
    void setProgressMin(int pValue);
    void sendFeature(Feature pFeature);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class FeaturesWebWidget : public QWidget
{
    Q_OBJECT
public:
    FeaturesWebWidget(QWidget* parent = 0, const char* name = "FeaturesWebWidget");
    virtual ~FeaturesWebWidget();

    void showElement(const Feature &elem);

private slots:
    void onLoadFinished(bool ok);
    void javaScriptWindowObjectCleared();

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class FeatureProperties : public QWidget
{
    Q_OBJECT
public:
    FeatureProperties(QWidget* parent=0, const char* name="FeatureProperties");
    ~FeatureProperties();

    void showElement(const Feature &elem);
    Feature getCurrentElement() const;
    void setOrigPixmap(const QPixmap &pix);
    void setThumbnail(const QPixmap &pix);

protected:
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);

private slots:
    void onAddToPage();
    void onAddToLib();
    void onSetAsBackground();
    //void onBack();

private:
    void populateMetadata();
    void adaptSize();

    struct Private;
    std::unique_ptr<Private> d;
};

class FeatureItemButton : public QPushButton
{
public:
    FeatureItemButton(QWidget* parent=0, const char* name="FeatureItemButton");
    ~FeatureItemButton();
};

class FeaturesModel : public QAbstractListModel
{
    Q_OBJECT

signals:
    void dataRestructured();

public:
    FeaturesModel(QList<Feature> *pFeaturesList, QObject *parent = 0);
    virtual ~FeaturesModel();

    void deleteFavoriteItem( const QString &path );
    void deleteItem( const QString &path );
    void deleteItem(const Feature &feature);

    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
    QMimeData *mimeData( const QModelIndexList &indexes ) const;
    QStringList mimeTypes() const;
    int rowCount( const QModelIndex &parent ) const;
    Qt::ItemFlags flags( const QModelIndex &index ) const;
    bool dropMimeData(const QMimeData *mimeData, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    bool removeRow(int row, const QModelIndex &parent = QModelIndex());
    //bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
    //bool insertRow(int row, const QModelIndex &parent = QModelIndex());

    void moveData(const Feature &source, const Feature &destination, Qt::DropAction action, bool deleteManualy = false);
    Qt::DropActions supportedDropActions() const { return Qt::MoveAction | Qt::CopyAction; }
    Qt::DropActions supportedDragActions() const { return Qt::MoveAction | Qt::CopyAction; }

//    void setFeaturesList(QList <Feature> *flist ) { featuresList = flist; }

public slots:
    void addItem( const Feature &item );

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class FeaturesProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    FeaturesProxyModel(QObject *parent = 0) : QSortFilterProxyModel(parent) {;}
    virtual ~FeaturesProxyModel() {}
protected:
    virtual bool filterAcceptsRow ( int sourceRow, const QModelIndex & sourceParent ) const;
};

class FeaturesSearchProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    FeaturesSearchProxyModel(QObject *parent = 0);
    virtual ~FeaturesSearchProxyModel();
    void setFilterPrefix(const QString &newPrefix);
protected:
    virtual bool filterAcceptsRow ( int sourceRow, const QModelIndex & sourceParent ) const;
private:
    struct Private;
    std::unique_ptr<Private> d;
};

class FeaturesPathProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    FeaturesPathProxyModel(QObject *parent = 0);
    virtual ~FeaturesPathProxyModel();
    void setPath( const QString &p );
protected:
    virtual bool filterAcceptsRow ( int sourceRow, const QModelIndex & sourceParent ) const;
private:
    struct Private;
    std::unique_ptr<Private> d;
};

class FeaturesItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    FeaturesItemDelegate(QObject *parent = 0, const QListView *lw = 0);
    ~FeaturesItemDelegate();
    //FeaturesItemDelegate(const QListView *lw = 0) { listView = lw; };
    //void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    //QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual QString    displayText ( const QVariant & value, const QLocale & locale ) const;
private:
    struct Private;
    std::unique_ptr<Private> d;
};

class FeaturesPathItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    FeaturesPathItemDelegate(QObject *parent = 0);
    ~FeaturesPathItemDelegate();
    virtual QString    displayText ( const QVariant & value, const QLocale & locale ) const;
    void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
private:
    struct Private;
    std::unique_ptr<Private> d;
};

