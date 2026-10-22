#include "content/ExportWeb.h"
#include "utils/UserNotices.h"

#include "platform/PlatformUtils.h"
#include "utils/FileSystemUtils.h"

#include "model/DocumentProxy.h"

#include "utils/Globals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
THIRD_PARTY_WARNINGS_ENABLE

ExportWeb::ExportWeb(QObject *parent) : ExportAdaptor(parent)
{
    ExportWeb::tr("Page"); // 空槽：只为让翻译工具抽到这几个导出用的界面文案
}

ExportWeb::~ExportWeb()
{
}

void ExportWeb::persist(DocumentProxy *pDocumentProxy, const QModelIndex &selectedTreeIndex)
{
    Q_UNUSED(selectedTreeIndex);

    if (!pDocumentProxy)
    {
        return;
    }

    QString dirName = askForDirName(pDocumentProxy, tr("Export as Web data"));

    if (dirName.length() > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        UserNotices::presentStatusMessage(tr("Exporting document..."));

        if (FileSystemUtils::copyDir(pDocumentProxy->persistencePath(), dirName))
        {
            QString htmlPath = dirName + "/index.html";

            QFile html(":www/TianBoard-web-player.html");
            html.copy(htmlPath);

            UserNotices::presentStatusMessage(tr("Export successful."));

            QDesktopServices::openUrl(QUrl::fromLocalFile(htmlPath));
        }
        else
        {
            UserNotices::presentStatusMessage(tr("Export failed."));
        }

        QApplication::restoreOverrideCursor();
    }
}

QString ExportWeb::exportName()
{
    return tr("Export to Web Browser");
}
