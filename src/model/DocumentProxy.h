#pragma once

#include <QtWidgets>

#include "utils/StringUtils.h"

#include "settings/Settings.h"

class GraphicsScene;

class DocumentProxy : public QObject
{
    Q_OBJECT

	friend class PersistenceManager;

	public:
		DocumentProxy();
		DocumentProxy(const DocumentProxy &rValue);
		DocumentProxy(const QString& pPersistencePath);
		DocumentProxy(const QString& pPersistencePath, QMap<QString, QVariant> metadatas);

		virtual ~DocumentProxy();

		DocumentProxy * deepCopy() const;
		bool theSameDocument(DocumentProxy *proxy);

		QString persistencePath() const;

		void setPersistencePath(const QString& pPersistencePath);

		void setMetaData(const QString& pKey , const QVariant& pValue);
		QVariant metaData(const QString& pKey) const;
		QMap<QString, QVariant> metaDatas() const;

		QString name() const;
		QString groupName() const;
		QDateTime documentDate();

		QDateTime lastUpdate();

		QSize defaultDocumentSize() const;
		void setDefaultDocumentSize(QSize pSize);
		void setDefaultDocumentSize(int pWidth, int pHeight);

		QUuid uuid() const;
		void setUuid(const QUuid& uuid);

		bool isModified() const;

		int pageCount();

		int pageDpi();
		void setPageDpi(int dpi);

	protected:
		void setPageCount(int pPageCount);
		int incPageCount();
		int decPageCount();

	private:
		void init();

		QString mPersistencePath;

		QMap<QString, QVariant> mMetaDatas;

		bool mIsModified;

		int mPageCount;

		int mPageDpi;

};

inline bool operator==(const DocumentProxy &proxy1, const DocumentProxy &proxy2)
{
    return proxy1.persistencePath() == proxy2.persistencePath();
}

inline size_t qHash(const DocumentProxy &key, size_t seed = 0)
{
    return qHash(key.persistencePath(), seed);
}

