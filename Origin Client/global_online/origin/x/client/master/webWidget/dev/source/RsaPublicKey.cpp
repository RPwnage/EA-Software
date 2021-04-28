#include <QByteArray>
#include <QIODevice>

#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>

#include "RsaPublicKey.h"

namespace WebWidget
{
    RsaPublicKey::RsaPublicKey() :
        mKeyContainer(new KeyContainer())
    {
    }

    RsaPublicKey::RsaPublicKey(RSA *rsa) :
        mKeyContainer(new KeyContainer(rsa))
    {
    }

    RsaPublicKey RsaPublicKey::fromPemEncodedData(QIODevice *dev)
    {
        // Create a BIO for the QIODevice contents
        const QByteArray keyData(dev->readAll());
        BIO *input = BIO_new_mem_buf(static_cast<void*>(const_cast<char*>(keyData.constData())), keyData.size());

        if (input == NULL)
        {
            // Couldn't create the BIO
            return RsaPublicKey();
        }

        // Read the key
        RSA *key = PEM_read_bio_RSA_PUBKEY(input, NULL, NULL, NULL);
        BIO_free(input);

        // If we pass NULL here we create an invalid key
        return RsaPublicKey(key);
    }

    bool RsaPublicKey::isNull() const
    {
        return openSslKey() == NULL;
    }
            
    RsaPublicKey::KeyContainer::KeyContainer(RSA *key) :
        mKey(key)
    {
    }

    // This is called when the last reference to the key goes away
    RsaPublicKey::KeyContainer::~KeyContainer()
    {
        if (mKey)
        {
            RSA_free(mKey);
        }
    }
}
