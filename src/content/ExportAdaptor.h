#pragma once

#include <QtWidgets>

class DocumentProxy;

class ExportAdaptor : public QObject {
    Q_OBJECT

public:
    ExportAdaptor(QObject *parent = 0);
    virtual ~ExportAdaptor();

    virtual QString exportName() = 0;
    virtual QString exportExtention() { return ""; }
    virtual void persist(DocumentProxy *pDocument) = 0;
    virtual bool persistsDocument(DocumentProxy *pDocument, const QString &filename);
    virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex) {
        Q_UNUSED(selectedIndex);
        return false;
    }
    QAction *associatedAction() { return mAssociatedAction; }
    void setAssociatedAction(QAction *pAssociatedAction) { mAssociatedAction = pAssociatedAction; }

    virtual void setVerbose(bool verbose) { mIsVerbose = verbose; }

    virtual bool isVerbose() const { return mIsVerbose; }

signals:
    // 导出过程中的警告。谁注册导出器谁负责显示——导出层不该认识 MainWindow 和
    // MessagesDialog（见 docs/architecture.md 拆环顺序第 6 步）。
    void exportWarnings(const QString &title, const QList<QString> &messages);

protected:
    QString askForFileName(DocumentProxy *pDocument, const QString &pDialogTitle);
    QString askForDirName(DocumentProxy *pDocument, const QString &pDialogTitle);

    virtual void persistLocally(DocumentProxy *pDocumentProxy, const QString &pDialogTitle);

    void reportErrorsList(const QList<QString> &errorsList);

    bool mIsVerbose;
    QAction *mAssociatedAction;
};
