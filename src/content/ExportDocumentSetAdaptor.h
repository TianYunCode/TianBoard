#pragma once

#include <QtCore>
#include "content/ExportAdaptor.h"
#include "utils/FileSystemUtils.h"
#include "utils/Globals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
THIRD_PARTY_WARNINGS_ENABLE

class DocumentProxy;
class DocumentTreeModel;

class ExportDocumentSetAdaptor : public ExportAdaptor
{
    Q_OBJECT

    public:
        ExportDocumentSetAdaptor(QObject *parent = 0);
        virtual ~ExportDocumentSetAdaptor();

        virtual QString exportName();
        virtual QString exportExtention();

        virtual void persist(DocumentProxy* pDocument);
        bool persistData(const QModelIndex &pRootIndex, QString filename);
        bool addDocumentToZip(const QModelIndex &pIndex, DocumentTreeModel *model, QuaZip &zip);

        virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);
};

