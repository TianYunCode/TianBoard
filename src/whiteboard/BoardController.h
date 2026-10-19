#pragma once

#include <QtWidgets>

#include <QObject>
#include <QHBoxLayout>
#include <QUndoCommand>

#include "model/DocumentContainer.h"
#include "core/ApplicationController.h"

class MainWindow;
class Application;
class BoardView;

class DocumentController;
class MessageWindow;
class GraphicsScene;
class DocumentProxy;
class BlackoutWidget;
class ToolWidget;
class Version;
class GraphicsMediaItem;
class GraphicsWidgetItem;
class BoardPaletteManager;
class Item;
class GraphicsItem;


class BoardController : public DocumentContainer
{
    Q_OBJECT

    public:
        enum SaveFlag {
            sf_none = 0x0,
            sf_showProgress = 0x1
        };
    Q_DECLARE_FLAGS(SaveFlags, SaveFlag)

    public:
        BoardController(MainWindow *mainWindow);
        virtual ~BoardController();

        void init();
        void setupLayout();

        GraphicsScene* activeScene() const;
        int activeSceneIndex() const;
        void setActiveSceneIndex(int i);
        QSize displayViewport();
        QSize controlViewport();
        QRectF controlGeometry();
        void closing();

        int currentPage();

        QWidget* controlContainer()
        {
            return mControlContainer;
        }

        BoardView* controlView()
        {
            return mControlView;
        }

        BoardView* displayView()
        {
            return mDisplayView;
        }

        GraphicsScene* activeScene()
        {
            return mActiveScene;
        }

        void setPenColorOnDarkBackground(const QColor& pColor)
        {
            if (mPenColorOnDarkBackground == pColor)
                return;

            mPenColorOnDarkBackground = pColor;
            emit penColorChanged();
        }

        void setPenColorOnLightBackground(const QColor& pColor)
        {
            if (mPenColorOnLightBackground == pColor)
                return;

            mPenColorOnLightBackground = pColor;
            emit penColorChanged();
        }

        void setMarkerColorOnDarkBackground(const QColor& pColor)
        {
            mMarkerColorOnDarkBackground = pColor;
        }

        void setMarkerColorOnLightBackground(const QColor& pColor)
        {
            mMarkerColorOnLightBackground = pColor;
        }

        QColor penColorOnDarkBackground()
        {
            return mPenColorOnDarkBackground;
        }

        QColor penColorOnLightBackground()
        {
            return mPenColorOnLightBackground;
        }

        QColor markerColorOnDarkBackground()
        {
            return mMarkerColorOnDarkBackground;
        }

        QColor markerColorOnLightBackground()
        {
            return mMarkerColorOnLightBackground;
        }

        qreal systemScaleFactor()
        {
            return mSystemScaleFactor;
        }
        qreal currentZoom();
        void persistViewPositionOnCurrentScene();
        void persistCurrentScene(bool isAnAutomaticBackup = false, bool forceImmediateSave = false);
        void setBoxing(QRect displayRect);
        void setToolbarTexts();
        static QUrl expandWidgetToTempDir(const QByteArray& pZipedData, const QString& pExtension = QString("wgt"));

        void setPageSize(QSize newSize);
        BoardPaletteManager *paletteManager()
        {
            return mPaletteManager;
        }

        void notifyCache(bool visible);
        void notifyPageChanged();
        void displayMetaData(QMap<QString, QString> metadatas);

        void findUniquesItems(const QUndoCommand *parent, QSet<QGraphicsItem *> &itms);
        void ClearUndoStack();

        void setActiveDocumentScene(DocumentProxy* pDocumentProxy, int pSceneIndex = 0, bool forceReload = false, bool onImport = false);
        void setActiveDocumentScene(int pSceneIndex);

        void moveSceneToIndex(int source, int target);
        void duplicateScene(int index);
        GraphicsItem *duplicateItem(Item *item);
        void deleteScene(int index);

        bool cacheIsVisible() {return mCacheWidgetIsEnabled;}

        QString actionGroupText(){ return mActionGroupText;}
        QString actionUngroupText(){ return mActionUngroupText;}

    public slots:
        void showDocumentsDialog();
        void showKeyboard(bool show);
        void toggleRecording(bool checked);
        void blackout();
        void addScene();
        void addScene(DocumentProxy* proxy, int sceneIndex, bool replaceActiveIfEmpty = false);
        void addScene(GraphicsScene* scene, bool replaceActiveIfEmpty = false);
        void duplicateScene();
        void importPage();
        void clearScene();
        void clearSceneItems();
        void clearSceneAnnotation();
        void clearSceneBackground();
        void zoomIn(QPointF scenePoint = QPointF(0,0));
        void zoomOut(QPointF scenePoint = QPointF(0,0));
        void zoomRestore();
        void centerRestore();
        void centerOn(QPointF scenePoint = QPointF(0,0));
        void zoom(const qreal ratio, QPointF scenePoint);
        void handScroll(qreal dx, qreal dy);
        void previousScene();
        void nextScene();
        void firstScene();
        void lastScene();
        void downloadURL(const QUrl& url, QString contentSourceUrl = QString(), const QPointF& pPos = QPointF(0.0, 0.0), const QSize& pSize = QSize(), bool isBackground = false, bool internalData = false);
        Item *downloadFinished(bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pHeader,
                                 QByteArray pData, QPointF pPos, QSize pSize,
                                 bool isBackground = false, bool internalData = false);
        void changeBackground(bool isDark, PageBackground pageBackground);
        void setToolCursor(int tool);
        void showMessage(const QString& message, bool showSpinningWheel = false);
        void hideMessage();
        void setDisabled(bool disable);
        void setColorIndex(int pColorIndex);
        void removeTool(ToolWidget* toolWidget);
        void hide();
        void show();
        void setWidePageSize(bool checked);
        void setRegularPageSize(bool checked);
        void stylusToolChanged(int tool);
        void grabScene(const QRectF& pSceneRect);
        GraphicsMediaItem* addVideo(const QUrl& pUrl, bool startPlay, const QPointF& pos, bool bUseSource = false);
        GraphicsMediaItem* addAudio(const QUrl& pUrl, bool startPlay, const QPointF& pos, bool bUseSource = false);
        GraphicsWidgetItem *addWebWidget(const QUrl& pUrl, const QPointF& pos);
        void adjustDisplayViews();
        void cut();
        void copy();
        void paste();
        void processMimeData(const QMimeData* pMimeData, const QPointF& pPos);
        void moveGraphicsWidgetToControlView(GraphicsWidgetItem* graphicWidget);
        void moveToolWidgetToScene(ToolWidget* toolWidget);
        void addItem();

        void freezeWebWidgets(bool freeze);
        void freezeWebWidget(QGraphicsItem* item, bool freeze);
        void startScript();
        void stopScript();

        void saveData(SaveFlags fls = sf_none);

        //void regenerateThumbnails();

    signals:
        void newPageAdded();
        void activeSceneChanged();
        void zoomChanged(qreal pZoomFactor);
        void penColorChanged();
        void controlViewportChanged();
        void backgroundChanged();
        void cacheEnabled();
        void documentReorganized(int index);
        void displayMetadata(QMap<QString, QString> metadata);
        void pageSelectionChanged(int index);
        void centerOnThumbnailRequired(int index);
        void npapiWidgetCreated(const QString &Url);

    protected:
        void setupViews();
        void setupToolbar();
        void connectToolbar();
        void initToolbarTexts();
        void updateActionStates();
        void updateSystemScaleFactor();
        QString truncate(QString text, int maxWidth);

    protected slots:
        void selectionChanged();
        void undoRedoStateChange(bool canUndo);
        void documentSceneChanged(DocumentProxy* proxy, int pIndex);

    private slots:
        void autosaveTimeout();
        void appMainModeChanged(ApplicationController::MainMode);

    private:
        void initBackgroundGridSize();
        void updatePageSizeState();
        void saveViewState();
        int autosaveTimeoutFromSettings();

        MainWindow *mMainWindow;
        GraphicsScene* mActiveScene;
        int mActiveSceneIndex;
        BoardPaletteManager *mPaletteManager;
        MessageWindow *mMessageWindow;
        BoardView *mControlView;
        BoardView *mDisplayView;
        QWidget *mControlContainer;
        QHBoxLayout *mControlLayout;
        qreal mZoomFactor;
        bool mIsClosing;
        QColor mPenColorOnDarkBackground;
        QColor mPenColorOnLightBackground;
        QColor mMarkerColorOnDarkBackground;
        QColor mMarkerColorOnLightBackground;
        qreal mSystemScaleFactor;
        bool mCleanupDone;
        QMap<QAction*, QPair<QString, QString> > mActionTexts;
        bool mCacheWidgetIsEnabled;
        QGraphicsItem* mLastCreatedItem;
        int mDeletingSceneIndex;
        int mMovingSceneIndex;
        QString mActionGroupText;
        QString mActionUngroupText;

        QTimer *mAutosaveTimer;

    private slots:
        void stylusToolDoubleClicked(int tool);
        void boardViewResized(QResizeEvent* event);
        void documentWillBeDeleted(DocumentProxy* pProxy);
        void updateBackgroundActionsState(bool isDark, PageBackground pageBackground);
        void colorPaletteChanged();
        void libraryDialogClosed(int ret);
        void lastWindowClosed();
        void onDownloadModalFinished();
};
