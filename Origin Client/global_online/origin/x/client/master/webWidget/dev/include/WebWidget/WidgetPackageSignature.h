#ifndef _WEBWIDGET_WIDGETPACKAGESIGNATURE_H
#define _WEBWIDGET_WIDGETPACKAGESIGNATURE_H

#include <QString>
#include <QByteArray>
#include <QHash>
#include <QStringList>

#include "WebWidgetPluginAPI.h"
#include "RsaPublicKey.h"

class QIODevice;

namespace WebWidget
{
    ///
    /// Represents a file entry in a widget package's digital signature
    ///
    class WEBWIDGET_PLUGIN_API WidgetPackageSignatureEntry
    {
        friend class WidgetPackageSignature;
    public:
        ///
        /// Returns if this is a null package file signature
        ///
        bool isNull() const;

        ///
        /// Verifies the signature entry against an input device
        ///
        /// Calls to verify() on null objects will always return false
        ///
        /// \param  packageFile QIODevice of the package file to verify
        /// \return True if the signature could be verified, false if the signature does not match or an error occured
        ///
        bool verify(QIODevice *packageFile) const;

    private:
        WidgetPackageSignatureEntry(const QByteArray &sha256Hash);
        WidgetPackageSignatureEntry();

        QByteArray mSha256Hash;
        bool mNull;
    };

    ///
    /// Represents the mandatory digital signature associated with a widget package.
    ///
    /// The signature contains an entry for every file in the widget package including the signature file itself.
    /// These signatures are used to validate the integrity of widget packages.
    ///
    class WEBWIDGET_PLUGIN_API WidgetPackageSignature
    {
    public:
        ///
        /// Creates a new WidgetPackageSignature
        ///
        /// \param  signatureData  Input device containing the signature data
        /// \param  publicKey      Public key to verify the signature against
        ///
        WidgetPackageSignature(QIODevice *signatureData, const RsaPublicKey &publicKey);

        ///
        /// Create an invalid WidgetPackageSignature
        ///
        WidgetPackageSignature();

        ///
        /// Returns if the widget package signature is valid
        ///
        /// \warning  This only verifies that the signature itself is valid; individual files should be verified
        ///           with WidgetPackageSignatureEntry::verify()
        ///
        bool isValid() const;

        ///
        /// Returns a list of package paths included in the signature
        ///
        QStringList packagePaths() const;

        ///
        /// Returns the signature entry for a given package path
        ///    
        /// \param  packagePath Path of the package file to search for
        /// \return Signature entry for the package path. A null signature will be returned if the package file was not
        ///         included in the widget package signature
        ///
        WidgetPackageSignatureEntry entryForPackagePath(const QString &packagePath) const;

        ///
        /// Returns the default package path for the package signature
        ///
        static QString defaultPackagePath();

    private:
        // Hash of package paths to signatures
        QHash<QString, QByteArray> mPackageFileSignatures;

        RsaPublicKey mPublicKey;
        bool mValid;
    };
}

#endif
