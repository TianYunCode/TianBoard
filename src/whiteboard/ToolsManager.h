#pragma once

#include <QtWidgets>

#include "core/Application.h"

class ToolsManager : public QObject
{
    Q_OBJECT

public:
    class ToolDescriptor
    {
        public:
            QString id;
            QPixmap icon;
            QString label;
            QString version;

    };

    static ToolsManager* manager();
    static void destroy();

    QList<ToolDescriptor> allTools()
    {
            return mDescriptors;
    }

    QStringList allToolIDs()
    {
        QStringList ids;

        foreach(ToolDescriptor tool, allTools())
        {
            ids << tool.id;
        }

        return ids;
    }

    ToolDescriptor toolByID(const QString& id)
    {
        foreach(ToolDescriptor tool, allTools())
        {
            if (tool.id == id)
                return tool;
        }

        return ToolDescriptor();
    }

    ToolDescriptor ruler;
    ToolDescriptor protractor;
    ToolDescriptor compass;
    ToolDescriptor mask;
    ToolDescriptor triangle;
    ToolDescriptor magnifier;
    ToolDescriptor cache;

    QString iconFromToolId(QString id) { return mToolsIcon.value(id);}

private:
    ToolsManager(QObject *parent = 0);
    virtual ~ToolsManager();

    static ToolsManager* sManager;

    QList<ToolDescriptor> mDescriptors;

    QMap<QString ,QString> mToolsIcon;

};

