#include "content/ImportAdaptor.h"

#include "core/Application.h"
#include "core/PersistenceManager.h"

#include "model/DocumentProxy.h"


ImportAdaptor::ImportAdaptor(bool _documentBased, QObject *parent)
    :QObject(parent),
    documentBased(_documentBased)
{
    // NOOP
}

ImportAdaptor::~ImportAdaptor()
{
    // NOOP
}

PageBasedImportAdaptor::PageBasedImportAdaptor(QObject *parent)
    :ImportAdaptor(false, parent)
{
    // NOOP
}

DocumentBasedImportAdaptor::DocumentBasedImportAdaptor(QObject *parent)
    :ImportAdaptor(true, parent)
{
    // NOOP
}
