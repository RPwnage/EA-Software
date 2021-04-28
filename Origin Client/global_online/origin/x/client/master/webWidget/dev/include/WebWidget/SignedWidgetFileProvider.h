#ifndef _WEBWIDGET_SIGNEDWIDGETFILEPROVIDER_H
#define _WEBWIDGET_SIGNEDWIDGETFILEPROVIDER_H

#include <limits>
#include <QDateTime>
#include <QObject>
#include <QSharedDataPointer>
#include <QHash>

#include "WebWidgetPluginAPI.h"
#include "WidgetFileProvider.h"
#include "WidgetPackageSignature.h"
#include "RsaPublicKey.h"

namespace WebWidget
{
    ///
    /// WidgetFileProvider that implements pacakge signature verification
    ///
    /// This wraps another WidgetFileProvider to perform signature verification whenever a file's metadata changes.
    /// If signature verification fails verificationFailed() is emitted.
    ///
    class WEBWIDGET_PLUGIN_API SignedWidgetFileProvider : public QObject, public WidgetFileProvider
    {
        Q_OBJECT
    public:
        ///
        /// Creates a new SignedWidgetFileProvider
        ///
        /// \param unverifiedFileProvider  Backing WidgetFileProvider for looking up unverified package files.
        ///                                SignedWidgetFileProvider takes ownership of the unverified file provider.
        /// \param publicKey               RSA public key to verify the package signature against 
        ///
        SignedWidgetFileProvider(WidgetFileProvider *unverifiedFileProvider, const RsaPublicKey &publicKey);
        
        QFileInfo backingFileInfo(const QString &canonicalPackagePath) const;

    signals:
        ///
        /// Emitted when the widget package is detected to be inconsistent with the package signature
        ///
        /// \param unverifiedFileProvider  Backing WidgetFileProvider in an inconsistent state. This is the
        ///                                WidgetFileProvider this instance was constructed with.
        ///
        /// This can occur in two situations:
        /// -# A package file's calculated signature doesn't match the one included in the package signature
        /// -# A package file included in the package signature no longer exists
        ///
        /// Once this signal has been emitted backingFileInfo() will always return non-existent files 
        ///
        void verificationFailed(WidgetFileProvider *unverifiedFileProvider);

    private:
        struct WEBWIDGET_PLUGIN_API VerifiedFileInfo
        {
            VerifiedFileInfo(const QFileInfo &fileInfo) :
                size(fileInfo.size()),
                lastModified(fileInfo.lastModified())
            {
            }

            qint64 size;
            QDateTime lastModified;
        };

        bool initPackageSignature(WidgetFileProvider *unverifiedFileProvider, const RsaPublicKey &publicKey);
        void packageInconsistent();

        WidgetFileProvider *mUnverifiedFileProvider;
        WidgetPackageSignature mPackageSignature;

        // Hash of canonical package paths to the last file info we verified
        typedef QHash<QString, VerifiedFileInfo> VerifiedFileInfoHash;
        mutable VerifiedFileInfoHash mLastVerifiedInfo;
    };
}

#endif
