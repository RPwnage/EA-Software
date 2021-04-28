#ifndef _WEBWIDGET_RSAPUBLICKEY_H
#define _WEBWIDGET_RSAPUBLICKEY_H

#include <QSharedDataPointer>
#include <QSharedData>
// RSA is a typedef so we can't forward declare it cleanly
#include <openssl/rsa.h>

#include "WebWidgetPluginAPI.h"

class QIODevice;

namespace WebWidget
{
    class WidgetPackageSignature;
    class WidgetPackageFileSignature;

    ///
    /// Represents an RSA public key
    ///
    class WEBWIDGET_PLUGIN_API RsaPublicKey
    {
        friend class WidgetPackageSignature;
        friend class WidgetPackageFileSignature;
    public:
        RsaPublicKey();

        ///
        /// Creates a new RSA public key from PEM encoded data
        ///
        /// Encrypted public keys are not supported
        ///
        /// \param  inputData Input device containing a PEM encoded RSA public key
        ///
        static RsaPublicKey fromPemEncodedData(QIODevice *inputData);

        ///
        /// Returns true if the RSA public key is null
        ///
        bool isNull() const;

    private:
        class WEBWIDGET_PLUGIN_API KeyContainer : public QSharedData
        {
        public:
            KeyContainer(RSA *key = NULL);
            ~KeyContainer();
            
            RSA *key() const { return mKey; }

        private:
            RSA *mKey;
        };

        RsaPublicKey(RSA *key );
        RSA *openSslKey() const { return mKeyContainer->key(); }

        QSharedDataPointer<KeyContainer> mKeyContainer;
    };
}

#endif
