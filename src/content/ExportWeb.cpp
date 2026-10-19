#include "content/ExportWeb.h"

#include "platform/PlatformUtils.h"
#include "utils/FileSystemUtils.h"

#include "core/DocumentManager.h"
#include "core/Application.h"

#include "model/DocumentProxy.h"

#include "utils/Globals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
THIRD_PARTY_WARNINGS_ENABLE


ExportWeb::ExportWeb(QObject *parent)
    : ExportAdaptor(parent)
{
    ExportWeb::tr("Page"); // dummy slot for translation
}


ExportWeb::~ExportWeb()
{
    // NOOP
}


void ExportWeb::persist(DocumentProxy* pDocumentProxy)
{
    if (!pDocumentProxy)
        return;

    QString dirName = askForDirName(pDocumentProxy, tr("Export as Web data"));

    if (dirName.length() > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        Application::showMessage(tr("Exporting document..."));

        if(FileSystemUtils::copyDir(pDocumentProxy->persistencePath(), dirName))
        {
            QString htmlPath = dirName + "/index.html";

            QFile html(":www/TianBoard-web-player.html");
            html.copy(htmlPath);

            Application::showMessage(tr("Export successful."));

            QDesktopServices::openUrl(QUrl::fromLocalFile(htmlPath));
        }
        else
        {
            Application::showMessage(tr("Export failed."));
        }


        QApplication::restoreOverrideCursor();
    }
}


QString ExportWeb::exportName()
{
    return tr("Export to Web Browser");
}
