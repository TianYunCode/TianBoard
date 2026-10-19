#pragma once

#include <QtCore>

#include "content/ExportAdaptor.h"

#include "utils/FileSystemUtils.h"

class DocumentProxy;

class ExportIwb : public ExportAdaptor
{
    Q_OBJECT

public:
    ExportIwb(QObject *parent = 0);
    virtual ~ExportIwb();

    virtual QString exportName();
    virtual QString exportExtention();
    virtual void persist(DocumentProxy* pDocument);
    virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);
};

