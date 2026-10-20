#pragma once

#include <QMainWindow>
#include <QMessageBox>

#include <memory>

#include "ui/MainWindowUi.h"

class QCloseEvent;
class QKeyEvent;
class QMenu;
class QStackedLayout;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~MainWindow() override;

    MainWindowUi &ui();

    void addBoardWidget(QWidget *pWidget);
    void switchToBoardWidget();

    void addWebWidget(QWidget *pWidget);
    void switchToWebWidget();

    void addDocumentsWidget(QWidget *pWidget);
    void switchToDocumentsWidget();

    bool yesNoQuestion(QString windowTitle, QString text);
    void warning(QString windowTitle, QString text);
    void information(QString windowTitle, QString text);

    void showDownloadWidget();
    void hideDownloadWidget();

signals:
    void closeEvent_Signal(QCloseEvent *event);

public slots:
    void onExportDone();

protected:
    void oneButtonMessageBox(QString windowTitle, QString text, QMessageBox::Icon type);

    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

    QMenu *createPopupMenu() override { return nullptr; }

private:
    struct Private;
    std::unique_ptr<Private> d;
};
