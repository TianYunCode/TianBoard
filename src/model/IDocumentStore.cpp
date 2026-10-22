#include "model/IDocumentStore.h"

#include <QDebug>

namespace
{
    IDocumentStore *gStore = nullptr;
} // namespace

void installDocumentStore(IDocumentStore *store)
{
    gStore = store;
}

IDocumentStore *documentStore()
{
    if (!gStore)
    {
        qFatal("文档仓库没装上：content 的导入导出器在 PersistenceManager 构造之前被调用");
    }

    return gStore;
}
