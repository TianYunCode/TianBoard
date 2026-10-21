#include "content/ImportAdaptor.h"

ImportAdaptor::ImportAdaptor(bool _documentBased, QObject *parent) : QObject(parent), documentBased(_documentBased)
{
}

ImportAdaptor::~ImportAdaptor()
{
}

PageBasedImportAdaptor::PageBasedImportAdaptor(QObject *parent) : ImportAdaptor(false, parent)
{
}

DocumentBasedImportAdaptor::DocumentBasedImportAdaptor(QObject *parent) : ImportAdaptor(true, parent)
{
}
