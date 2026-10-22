#include "utils/StaticMemory.h"

#include <QObject>

namespace
{
    QObject *gCleaner = nullptr;
} // namespace

namespace StaticMemory
{
    QObject *cleaner()
    {
        if (!gCleaner)
        {
            gCleaner = new QObject();
        }

        return gCleaner;
    }

    void release()
    {
        delete gCleaner;
        gCleaner = nullptr;
    }
} // namespace StaticMemory
