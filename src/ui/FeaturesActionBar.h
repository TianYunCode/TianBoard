#pragma once

#include <QWidget>
#include <QToolButton>
#include <QDropEvent>
#include <QLineEdit>
#include <QLayout>

#include "whiteboard/FeaturesController.h"

#include <memory>

#define BUTTON_SIZE 24
#define ACTIONBAR_HEIGHT 42

class FeaturesMimeData;

typedef enum
{
    eButtonSet_Default,
    eButtonSet_Properties,
    eButtonSet_Favorite
} eButtonSet;

class ActionButton : public QToolButton
{
public:
    ActionButton(QWidget* parent=0, QAction* action=0, const char* name="ActionButton");
    ~ActionButton();
};

enum FeaturesActionBarState
{
    IN_ROOT,
    IN_FOLDER,
    IN_PROPERTIES,
    IN_FAVORITE,
    IN_TRASH
};

class FeaturesActionBar : public QWidget
{
    Q_OBJECT
public:
    FeaturesActionBar(FeaturesController *controller, QWidget* parent=0, const char* name="FeaturesActionBar");
    ~FeaturesActionBar();
    
    void setCurrentState( FeaturesActionBarState state );
    void cleanText();

signals:
    void searchElement(const QString &text);
    void newFolderToCreate();
    void deleteElements(const FeaturesMimeData *data);
    void addToFavorite(const FeaturesMimeData *data);
    void removeFromFavorite(const FeaturesMimeData *data);
    void addElementsToFavorite();
    void removeElementsFromFavorite();
    void deleteSelectedElements();
    void rescanModel();

private slots:
    void onSearchTextChanged(QString txt);
    void onActionNewFolder();
    void onActionFavorite();
    void onActionRemoveFavorite();
    void onActionTrash();
    void onActionRescanModel();
    void lockIt();
    void unlockIt();

protected:
    void dragEnterEvent( QDragEnterEvent *event );
    void dropEvent( QDropEvent *event );

private:
    void setButtons();

    struct Private;
    std::unique_ptr<Private> d;
};

