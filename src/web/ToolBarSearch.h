#pragma once

#include "web/SearchLineEdit.h"

#include <QtWidgets>

class AutoSaver;

class ToolbarSearch : public SearchLineEdit
{
    Q_OBJECT;

public:
    ToolbarSearch(QWidget *parent = 0);
    ~ToolbarSearch();

signals:
	void search(const QUrl &url);

public slots:
    void clear();
    void searchNow();

private slots:
    void save();
    void aboutToShowMenu();
    void triggeredMenuAction(QAction *action);

private:
    void load();

    AutoSaver *mAutosaver;
    int mMaxSavedSearches;
    QStringListModel *mStringListModel;
};


