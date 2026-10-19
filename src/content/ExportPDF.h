#pragma once

#include <QtCore>
#include "content/ExportAdaptor.h"

class DocumentProxy;

class ExportPDF : public ExportAdaptor
{
    Q_OBJECT

    public:
        ExportPDF(QObject *parent = 0);
        virtual ~ExportPDF();

        virtual QString exportName();
        virtual QString exportExtention();
        virtual void persist(DocumentProxy* pDocument);
        virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);

        virtual bool persistsDocument(DocumentProxy* pDocument, const QString& filename);
};

