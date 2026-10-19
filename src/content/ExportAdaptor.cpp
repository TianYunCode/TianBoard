#include <QFileDialog>

#include "content/ExportAdaptor.h"

#include "model/DocumentProxy.h"

#include "utils/FileSystemUtils.h"

#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/Application.h"

#include "ui/MainWindow.h"
#include "ui/MessagesDialog.h"


ExportAdaptor::ExportAdaptor(QObject *parent)
    : QObject(parent)
    , mIsVerbose(true)
{
    // NOOP
}


ExportAdaptor::~ExportAdaptor()
{
    // NOOP
}

QString ExportAdaptor::askForFileName(DocumentProxy* pDocument, const QString& pDialogTitle)
{
    QString defaultName;

    defaultName += pDocument->metaData(Settings::documentName).toString() + exportExtention();

    defaultName = FileSystemUtils::cleanName(defaultName);

    QString defaultPath = Settings::settings()->lastExportFilePath->get().toString() + "/" + defaultName;

    QString filename = QFileDialog::getSaveFileName(Application::mainWindow, pDialogTitle, defaultPath, QString());

    if (filename.size() == 0)
    {
        return filename;
    }

    // add extension if needed
    QFileInfo fileNameInfo(filename);
    if (fileNameInfo.suffix() != exportExtention().mid(1, exportExtention().length() - 1))
    {
        filename += exportExtention();
    }
    Settings::settings()->lastExportFilePath->set(QVariant(fileNameInfo.absolutePath()));
    QApplication::processEvents();

    return filename;
}

QString ExportAdaptor::askForDirName(DocumentProxy* pDocument, const QString& pDialogTitle)
{
    QString defaultPath = Settings::settings()->lastExportDirPath->get().toString();

    QString container = QFileDialog::getExistingDirectory(Application::mainWindow, pDialogTitle, defaultPath);

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
        docname = FileSystemUtils::cleanName(docname);

        dirname = container + "/" + docname;
    }

    QApplication::processEvents();

    return dirname;
}

void ExportAdaptor::persistLocally(DocumentProxy* pDocumentProxy, const QString& pDialogTitle)
{
    if (!pDocumentProxy)
        return;

    QString filename = askForFileName(pDocumentProxy, pDialogTitle);

    if (filename.length() > 0) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        if (mIsVerbose)
            Application::showMessage(tr("Exporting document..."));

        // Check that the location is writeable
        QFileInfo info(filename);
        info.setFile(info.absolutePath());

        if (!info.isWritable()) {
            QMessageBox errorBox;
            errorBox.setWindowTitle(tr("Export failed"));
            errorBox.setText(tr("Unable to export to the selected location. You do not have the permissions necessary to save the file."));
            errorBox.setIcon(QMessageBox::Critical);
            errorBox.exec();

            if (mIsVerbose)
                Application::showMessage(tr("Export failed: location not writable"));

            QApplication::restoreOverrideCursor();

            return;
        }

        bool persisted = this->persistsDocument(pDocumentProxy, filename);

        if (mIsVerbose && persisted)
            Application::showMessage(tr("Export successful."));

        QApplication::restoreOverrideCursor();
    }
}

bool ExportAdaptor::persistsDocument(DocumentProxy* pDocument, const QString& filename)
{
    // Implemented in child classes

    Q_UNUSED(pDocument);
    Q_UNUSED(filename);
    return false;
}

void ExportAdaptor::showErrorsList(QList<QString> errorsList)
{
    if (errorsList.count())
    {
        MessagesDialog *dialog = new MessagesDialog(tr("Warnings during export was appeared"), Application::mainWindow);
        dialog->setMessages(errorsList);
        dialog->show();
    }
}
