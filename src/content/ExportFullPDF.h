#pragma once

#include <QtCore>
#include "content/ExportAdaptor.h"
#include "content/ExportPDF.h"

class DocumentProxy;

class ExportFullPDF : public ExportAdaptor
{
    Q_OBJECT;

    public:
        ExportFullPDF(QObject *parent = 0);
        virtual ~ExportFullPDF();

        virtual QString exportName();
        virtual QString exportExtention();
        virtual void persist(DocumentProxy* pDocument);
        virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);

        virtual bool persistsDocument(DocumentProxy* pDocument, const QString& filename);

    protected:
        void saveOverlayPdf(DocumentProxy* pDocumentProxy, const QString& filename);

    private:
        float mScaleFactor;
        bool mHasPDFBackgrounds;

        ExportPDF * mSimpleExporter;
};

