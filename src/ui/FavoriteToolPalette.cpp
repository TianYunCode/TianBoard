#include "ui/FavoriteToolPalette.h"

#include "settings/Settings.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsWidgetItem.h"

#include "whiteboard/ToolsManager.h"

#include "ui/MainWindow.h"


struct FavoriteToolPalette::Private {};

FavoriteToolPalette::FavoriteToolPalette(QWidget *parent)
    : ActionPalette(Qt::Horizontal, parent), d(new Private) {
    QWidget *container = new QWidget(this);
    container->setStyleSheet("QWidget {background-color: transparent}");

    QGridLayout *gridLayout = new QGridLayout();
    container->setLayout(gridLayout);
    layout()->addWidget(container);

    QList<QAction *> toolsActions;

    // 内置工具的全集以前是这条设置的默认值；Settings 不再认识 ToolsManager，
    // 没设置过就按“全部内置工具”兜底（这条设置从来没有写方，行为不变）。
    QStringList favoritesToolUris = Settings::settings()->favoritesNativeToolUris->get().toStringList();
    if (favoritesToolUris.isEmpty())
        favoritesToolUris = ToolsManager::manager()->allToolIDs();

    foreach (QString uri, favoritesToolUris) {
        ToolsManager::ToolDescriptor desc = ToolsManager::manager()->toolByID(uri);

        if (desc.label.length() > 0 && !desc.icon.isNull()) {
            QAction *action = new QAction(desc.label + " " + desc.version, this);
            action->setData(QUrl(desc.id));
            action->setIcon(desc.icon);
            connect(action, &QAction::triggered, this, &FavoriteToolPalette::addFavorite);

            toolsActions << action;
        }
    }

    QDir favoritesDir(Settings::settings()->userInteractiveFavoritesDirectory());
    QStringList favoritesSubDirs = favoritesDir.entryList(
        QStringList(), QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);
    QStringList appPathes;

    foreach (QString subDirName, favoritesSubDirs) {
        appPathes << favoritesDir.path() + "/" + subDirName;
    }

    foreach (QString widgetPath, appPathes) {
        QAction *action = new QAction(GraphicsWidgetItem::widgetName(QUrl::fromLocalFile(widgetPath)), this);
        action->setData(QUrl::fromLocalFile(widgetPath));
        action->setIcon(QIcon(GraphicsWidgetItem::iconFilePath(QUrl::fromLocalFile(widgetPath))));
        connect(action, &QAction::triggered, this, &FavoriteToolPalette::addFavorite);

        toolsActions << action;
    }


    if (toolsActions.size() < 4) {
        QStringList toolsIDs = ToolsManager::manager()->allToolIDs();

        foreach (QString id, favoritesToolUris)
            toolsIDs.removeAll(id);

        while (toolsIDs.size() > 0 && toolsActions.size() < 4) {
            ToolsManager::ToolDescriptor desc = ToolsManager::manager()->toolByID(toolsIDs.takeFirst());

            if (desc.label.length() > 0) {
                QAction *action = new QAction(desc.label + " " + desc.version, this);
                action->setData(QUrl(desc.id));
                action->setIcon(desc.icon);
                connect(action, &QAction::triggered, this, &FavoriteToolPalette::addFavorite);

                toolsActions << action;
            }
        }
    }

    int i = 0;

    foreach (QAction *action, toolsActions) {
        ActionPaletteButton *button = createPaletteButton(action, container);
        gridLayout->addWidget(button, i / 4, i % 4);
        appendAction(action);
        i++;
    }

    setClosable(true);
    setButtonIconSize(QSize(128, 128));
    setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    groupActions();
}


void FavoriteToolPalette::addFavorite() {
    // we need the sender :-( hugly ...
    QAction *action = qobject_cast<QAction *>(sender());

    if (action) {
        QVariant widgetPathVar = action->data();
        if (!widgetPathVar.isNull()) {
            Application::boardController->downloadURL(widgetPathVar.toUrl());
        }
    }
}

FavoriteToolPalette::~FavoriteToolPalette() {
    // NOOP
}
