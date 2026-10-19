#pragma once

#include <QtCore>

#include "content/ExportAdaptor.h"
#include "utils/FileSystemUtils.h"

class DocumentProxy;

class ExportDocument : public ExportAdaptor, public ProcessingProgressListener
{
    Q_OBJECT

    public:
        ExportDocument(QObject *parent = 0);
        virtual ~ExportDocument();

        virtual QString exportName();
        virtual QString exportExtention();
        virtual void persist(DocumentProxy* pDocument);

        virtual bool persistsDocument(DocumentProxy* pDocument, const QString& filename);

        virtual void processing(const QString& pObjectName, int pCurrent, int pTotal);

        virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);
};

