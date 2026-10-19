#include <QFile>
#include <QGuiApplication>
#include <QScreen>

#include "content/PDFRenderer.h"

#include "content/QPdfRenderer.h"



QMap< QUuid, QPointer<PDFRenderer> > PDFRenderer::sRenderers;

PDFRenderer::PDFRenderer() : dpiForRendering(96)
{
}

PDFRenderer::~PDFRenderer()
{
    // NOOP
}

PDFRenderer* PDFRenderer::rendererForUuid(const QUuid &uuid, const QString &filename, bool importingFile)
{
    if (sRenderers.contains(uuid))
    {
        return sRenderers.value(uuid);
    }
    else
    {
		PDFRenderer *newRenderer = new QPdfRenderer(filename, importingFile);

        newRenderer->setRefCount(0);
        newRenderer->setFileUuid(uuid);

        QFile file(filename);
        file.open(QIODevice::ReadOnly);
        newRenderer->setFileData(file.readAll());
        file.close();

        sRenderers.insert(newRenderer->fileUuid(), newRenderer);

        QScreen *screen = QGuiApplication::primaryScreen();
        int dpiCommon = screen ? static_cast<int>((screen->physicalDotsPerInchX() + screen->physicalDotsPerInchY()) / 2) : 96;
        newRenderer->setDPI(dpiCommon);

        return newRenderer;
    }
}

void PDFRenderer::setRefCount(const QAtomicInt &refCount)
{
    mRefCount = refCount;
}

void PDFRenderer::setFileData(const QByteArray &fileData)
{
    mFileData = fileData;
}

void PDFRenderer::setFileUuid(const QUuid &fileUuid)
{
    mFileUuid = fileUuid;
}

void PDFRenderer::attach()
{
    mRefCount.ref();
}

void PDFRenderer::detach()
{
    mRefCount.deref();
    if (mRefCount.loadAcquire() == 0)
    {
        sRenderers.remove(mFileUuid);
        delete this;
    }
}
