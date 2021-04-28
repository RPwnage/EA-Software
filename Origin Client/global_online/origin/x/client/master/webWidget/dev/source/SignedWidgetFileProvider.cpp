#include <QDebug>

#include "SignedWidgetFileProvider.h"

namespace WebWidget
{
    SignedWidgetFileProvider::SignedWidgetFileProvider(WidgetFileProvider *unverifiedFileProvider, const RsaPublicKey &publicKey) :
        // Don't initialize this until we successfully read the package signature
        mUnverifiedFileProvider(NULL)
    {
        if (!initPackageSignature(unverifiedFileProvider, publicKey))
        {
            delete unverifiedFileProvider;
        }
        else
        {
            // The package signature itself is valid
            // Allow proxied access to the widget package
            mUnverifiedFileProvider = unverifiedFileProvider;
        }
    }
        
    bool SignedWidgetFileProvider::initPackageSignature(WidgetFileProvider *unverifiedFileProvider, const RsaPublicKey &publicKey)
    {
        // Look up our signature file
        const QFileInfo signatureFileInfo(
                unverifiedFileProvider->backingFileInfo(WidgetPackageSignature::defaultPackagePath()));

        if (!signatureFileInfo.exists())
        {
            qWarning() << "Signature file does not exist";
            return false;
        }

        QFile signatureFile(signatureFileInfo.absoluteFilePath());

        if (!signatureFile.open(QIODevice::ReadOnly))
        {
            qWarning() << "Unable to open signature file";
            return false;
        }

        // Parse the signature
        mPackageSignature = WidgetPackageSignature(&signatureFile, publicKey);

        return mPackageSignature.isValid();
    }
        
    QFileInfo SignedWidgetFileProvider::backingFileInfo(const QString &canonicalPackagePath) const
    {
        if (!mUnverifiedFileProvider)
        {
            // No unverified file provider
            return QFileInfo();
        }

        // Look up the signature for the file
        const WidgetPackageSignatureEntry signatureEntry(mPackageSignature.entryForPackagePath(canonicalPackagePath));

        if (signatureEntry.isNull())
        {
            // No signature exists; treat the file as non-existent
            return QFileInfo();
        }

        // Look up the file information from our unverified file provider
        const QFileInfo fileInfo(mUnverifiedFileProvider->backingFileInfo(canonicalPackagePath)); 

        if (!fileInfo.exists())
        {
            // The file should be there but it's missing
            const_cast<SignedWidgetFileProvider*>(this)->packageInconsistent();
            return QFileInfo();
        }

        // Find the last file info we verified
        VerifiedFileInfoHash::ConstIterator lastInfo = mLastVerifiedInfo.constFind(canonicalPackagePath);

        if ((lastInfo == mLastVerifiedInfo.constEnd()) ||
            (lastInfo->size != fileInfo.size()) ||
            (lastInfo->lastModified != fileInfo.lastModified()))
        {
            // Open the file
            QFile backingFile(fileInfo.absoluteFilePath());

            if (!backingFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
            {
                // This is tricky
                // On Windows there might be file locking in play
                // On *nix it's doubtful this is a transient failure unless it's a remote FS
                // Be nice to the user and treat this as transient
                return QFileInfo();
            }

            if (!signatureEntry.verify(&backingFile))
            {
                qWarning() << "File signature did not match for" << fileInfo.absoluteFilePath();
                const_cast<SignedWidgetFileProvider*>(this)->packageInconsistent();
                return QFileInfo();
            }

            // Remember the info of the file that passed signature verification
            mLastVerifiedInfo.insert(canonicalPackagePath, VerifiedFileInfo(fileInfo));
        }

        return fileInfo;
    }
        
    void SignedWidgetFileProvider::packageInconsistent()
    {
        emit verificationFailed(mUnverifiedFileProvider);

        // Prevent all future access
        delete mUnverifiedFileProvider;
        mUnverifiedFileProvider = NULL;
    }
}
    
