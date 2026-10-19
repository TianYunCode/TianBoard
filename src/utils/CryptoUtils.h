#pragma once

#include <QtCore>
#include <openssl/evp.h>

#include "core/Application.h"

class CryptoUtils : public QObject
{
    Q_OBJECT;

    public:
        static CryptoUtils* instance();
        static void destroy();

        QString symetricEncrypt(const QString& clear);
        QString symetricDecrypt(const QString& encrypted);

    private:
        CryptoUtils(QObject * pParent = 0);
        virtual ~CryptoUtils();

        static CryptoUtils* sInstance;
        static QString sAESKey;
        static QString sAESSalt;

        void aesInit();

#if OPENSSL_VERSION_NUMBER >= 10100000L
        EVP_CIPHER_CTX *mAesEncryptContext;
        EVP_CIPHER_CTX *mAesDecryptContext;
#else
        EVP_CIPHER_CTX mAesEncryptContext;
        EVP_CIPHER_CTX mAesDecryptContext;
#endif

};

