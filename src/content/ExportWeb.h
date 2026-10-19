#pragma once

#include <QtCore>

#include "content/ExportAdaptor.h"

class DocumentProxy;

class ExportWeb : public ExportAdaptor {
    Q_OBJECT;

public:
    ExportWeb(QObject *parent = 0);
    virtual ~ExportWeb();

    virtual QString exportName();
    virtual void persist(DocumentProxy *pDocument);
};
