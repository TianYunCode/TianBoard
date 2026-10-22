#include <QFileDialog>
#include "utils/UserNotices.h"

#include "content/ExportAdaptor.h"

#include "model/DocumentProxy.h"

#include "utils/FileSystemUtils.h"

#include "settings/Settings.h"

ExportAdaptor::ExportAdaptor(QObject *parent) : QObject(parent), mIsVerbose(true)
{
}

ExportAdaptor::~ExportAdaptor()
{
}

QString ExportAdaptor::askForFileName(DocumentProxy *pDocument, const QString &pDialogTitle)
{
    QString defaultName;

    defaultName         += pDocument->metaData(Settings::documentName).toString() + exportExtention();

    defaultName          = FileSystemUtils::cleanName(defaultName);

    QString defaultPath  = Settings::settings()->lastExportFilePath->get().toString() + "/" + defaultName;

    QString filename     = QFileDialog::getSaveFileName(UserNotices::dialogHost(), pDialogTitle, defaultPath, QString());

    if (filename.size() == 0)
    {
        return filename;
    }

    // QFileDialog 不会强制补后缀：用户手输文件名后这里补齐，避免生成无扩展名文件。
    QFileInfo fileNameInfo(filename);
    if (fileNameInfo.suffix() != exportExtention().mid(1, exportExtention().length() - 1))
    {
        filename += exportExtention();
    }
    Settings::settings()->lastExportFilePath->set(QVariant(fileNameInfo.absolutePath()));
    QApplication::processEvents();

    return filename;
}

QString ExportAdaptor::askForDirName(DocumentProxy *pDocument, const QString &pDialogTitle)
{
    QString defaultPath = Settings::settings()->lastExportDirPath->get().toString();

    QString container   = QFileDialog::getExistingDirectory(UserNotices::dialogHost(), pDialogTitle, defaultPath);

    QString dirname;

    if (container.size() > 0)
    {
        Settings::settings()->lastExportDirPath->set(QVariant(container));

        QString docname;

        if (pDocument->metaData(Settings::documentGroupName).toString().length() > 0)
        {
            docname += pDocument->metaData(Settings::documentGroupName).toString() + QString(" ");
        }

        docname += pDocument->metaData(Settings::documentName).toString() + exportExtention();
        docname  = FileSystemUtils::cleanName(docname);

        dirname  = container + "/" + docname;
    }

    QApplication::processEvents();

    return dirname;
}

void ExportAdaptor::persistLocally(DocumentProxy *pDocumentProxy, const QString &pDialogTitle)
{
    if (!pDocumentProxy)
    {
        return;
    }

    QString filename = askForFileName(pDocumentProxy, pDialogTitle);

    if (filename.length() > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        if (mIsVerbose)
        {
            UserNotices::presentStatusMessage(tr("Exporting document..."));
        }

        // 只检查目录权限而不是文件本身：文件可能尚不存在，QFileInfo::isWritable 对
        // 不存在的路径不可靠。
        QFileInfo info(filename);
        info.setFile(info.absolutePath());

        if (!info.isWritable())
        {
            QMessageBox errorBox;
            errorBox.setWindowTitle(tr("Export failed"));
            errorBox.setText(tr("Unable to export to the selected location. You do not have the permissions "
                                "necessary to save the file."));
            errorBox.setIcon(QMessageBox::Critical);
            errorBox.exec();

            if (mIsVerbose)
            {
                UserNotices::presentStatusMessage(tr("Export failed: location not writable"));
            }

            QApplication::restoreOverrideCursor();

            return;
        }

        bool persisted = this->persistsDocument(pDocumentProxy, filename);

        if (mIsVerbose && persisted)
        {
            UserNotices::presentStatusMessage(tr("Export successful."));
        }

        QApplication::restoreOverrideCursor();
    }
}

bool ExportAdaptor::persistsDocument(DocumentProxy *pDocument, const QString &filename)
{
    // 基类没有写出能力；子类必须覆写，否则只弹对话框不产出文件。

    Q_UNUSED(pDocument);
    Q_UNUSED(filename);
    return false;
}

void ExportAdaptor::reportErrorsList(const QList<QString> &errorsList)
{
    if (errorsList.count())
    {
        emit exportWarnings(tr("Warnings during export was appeared"), errorsList);
    }
}
