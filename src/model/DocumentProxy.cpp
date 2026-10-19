#include "model/DocumentProxy.h"
#include <QLocale>

#include "utils/StringUtils.h"

#include "core/Application.h"
#include "core/PersistenceManager.h"
#include "core/Settings.h"
#include "core/DocumentManager.h"

#include "content/MetadataDcSubsetAdaptor.h"

DocumentProxy::DocumentProxy()
    : mPageCount(0)
    , mPageDpi(0)
    , mPersistencePath("")
{
    init();
}

DocumentProxy::DocumentProxy(const DocumentProxy &rValue) :
    QObject()
{
    mPersistencePath = rValue.mPersistencePath;
    mMetaDatas = rValue.mMetaDatas;
    mIsModified = rValue.mIsModified;
    mPageCount = rValue.mPageCount;
}

DocumentProxy::DocumentProxy(const QString& pPersistancePath)
    : mPageCount(0)
    , mPageDpi(0)
{
    init();
    setPersistencePath(pPersistancePath);

    mMetaDatas = MetadataDcSubsetAdaptor::load(pPersistancePath);
}


DocumentProxy::DocumentProxy(const QString& pPersistancePath, QMap<QString, QVariant> metadatas)
    : mPageCount(0)
    , mPageDpi(0)
{
    init();
    setPersistencePath(pPersistancePath);

    mMetaDatas = metadatas;
}


void DocumentProxy::init()
{
    setMetaData(Settings::documentGroupName, "");

    QDateTime now = QDateTime::currentDateTime();
    setMetaData(Settings::documentName, QLocale::system().toString(now, QLocale::ShortFormat));

    setUuid(QUuid::createUuid());

    setDefaultDocumentSize(Settings::settings()->pageSize->get().toSize());
}

bool DocumentProxy::theSameDocument(DocumentProxy *proxy)
{
    return  proxy && mPersistencePath == proxy->mPersistencePath;
}

DocumentProxy::~DocumentProxy()
{
    // NOOP
}

DocumentProxy* DocumentProxy::deepCopy() const
{
    DocumentProxy* copy = new DocumentProxy();

    copy->mPersistencePath = QString(mPersistencePath);
    copy->mMetaDatas = QMap<QString, QVariant>(mMetaDatas);
    copy->mIsModified = mIsModified;
    copy->mPageCount = mPageCount;

    return copy;
}


int DocumentProxy::pageCount()
{
    return mPageCount;
}


void DocumentProxy::setPageCount(int pPageCount)
{
    mPageCount = pPageCount;
}

int DocumentProxy::pageDpi()
{
    return mPageDpi;
}

void DocumentProxy::setPageDpi(int dpi)
{
    mPageDpi = dpi;
}

int DocumentProxy::incPageCount()
{
    if (mPageCount <= 0)
    {
        mPageCount = 1;
    }
    else
    {
        mPageCount++;
    }

    return mPageCount;

}


int DocumentProxy::decPageCount()
{
    mPageCount --;

    if (mPageCount < 0)
    {
        mPageCount = 0;
    }

    return mPageCount;
}

QString DocumentProxy::persistencePath() const
{
    return mPersistencePath;
}

void DocumentProxy::setPersistencePath(const QString& pPersistencePath)
{
    if (pPersistencePath != mPersistencePath)
    {
        mIsModified = true;
        mPersistencePath = pPersistencePath;
    }
}

void DocumentProxy::setMetaData(const QString& pKey, const QVariant& pValue)
{
    if (mMetaDatas.contains(pKey) && mMetaDatas.value(pKey) == pValue)
        return;
    else
    {
        mIsModified = true;
        mMetaDatas.insert(pKey, pValue);
        if (pKey == Settings::documentUpdatedAt)
        {
            DocumentManager *documentManager = DocumentManager::documentManager();
            if (documentManager)
                documentManager->emitDocumentUpdated(this);
        }
    }
}

QVariant DocumentProxy::metaData(const QString& pKey) const
{
    if (mMetaDatas.contains(pKey))
    {
        return mMetaDatas.value(pKey);
    }
    else
    {
        qDebug() << "Unknown metadata key" << pKey;
        return QString(""); // failsafe
    }
}

QMap<QString, QVariant> DocumentProxy::metaDatas() const
{
    return mMetaDatas;
}

QString DocumentProxy::name() const
{
    return metaData(Settings::documentName).toString();
}

QString DocumentProxy::groupName() const
{
    return metaData(Settings::documentGroupName).toString();
}

QSize DocumentProxy::defaultDocumentSize() const
{
    if (mMetaDatas.contains(Settings::documentSize))
        return metaData(Settings::documentSize).toSize();
    else
        return Settings::settings()->pageSize->get().toSize();
}

void DocumentProxy::setDefaultDocumentSize(QSize pSize)
{
    if (defaultDocumentSize() != pSize)
    {
        setMetaData(Settings::documentSize, QVariant(pSize));
        mIsModified = true;
    }
}

void DocumentProxy::setDefaultDocumentSize(int pWidth, int pHeight)
{
    setDefaultDocumentSize(QSize(pWidth, pHeight));
}


QUuid DocumentProxy::uuid() const
{
    QString id = metaData(Settings::documentIdentifer).toString();
    QString sUuid = id.replace(Settings::documentNamespaceUri + "/", "");

    return QUuid(sUuid);
}

void DocumentProxy::setUuid(const QUuid& uuid)
{
    setMetaData(Settings::documentIdentifer,
            Settings::documentNamespaceUri + "/" + StringUtils::toCanonicalUuid(uuid));
}


QDateTime DocumentProxy::documentDate()
{
    if(mMetaDatas.contains(Settings::documentDate))
        return StringUtils::fromUtcIsoDate(metaData(Settings::documentDate).toString());
    return QDateTime::currentDateTime();
}

QDateTime DocumentProxy::lastUpdate()
{
    if(mMetaDatas.contains(Settings::documentUpdatedAt))
        return StringUtils::fromUtcIsoDate(metaData(Settings::documentUpdatedAt).toString());
    return QDateTime::currentDateTime();
}

bool DocumentProxy::isModified() const
{
    return mIsModified;
}





