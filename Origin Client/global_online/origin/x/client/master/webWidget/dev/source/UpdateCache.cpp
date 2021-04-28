#include <QtConcurrentRun>
#include <QCryptographicHash>
#include <QReadLocker>
#include <QWriteLocker>
#include <QDataStream>
#include <QFile>

#include "UpdateIdentifier.h"
#include "UpdateCache.h"
#include "UnpackedUpdate.h"
#include "UpdateInstallation.h"
#include "WidgetPackageSignature.h"

namespace
{
    using namespace WebWidget;

    const unsigned int InstalledUpdateManifestMagic = 0x05b68eab;
    const QString InstalledUpdateManifestName("manifest");
    const QDataStream::Version InstalledUpdateManifestVersion = QDataStream::Qt_4_8;

    QString directoryNameForUpdate(const UpdateIdentifier &updateIdentifier)
    {
        QCryptographicHash hash(QCryptographicHash::Sha1);

        hash.addData(updateIdentifier.updateUrl().toEncoded());
        hash.addData(updateIdentifier.finalUrl().toEncoded());
        hash.addData(updateIdentifier.etag());

        return hash.result().toHex();
    }

    bool validateUpdateSignature(const UnpackedArchive &archive, const RsaPublicKey &publicKey)
    {
        const QString signatureFilePath(
            archive.unpackedFilePath(WidgetPackageSignature::defaultPackagePath()));

        QFile signatureFile(signatureFilePath);

        if (!signatureFile.open(QIODevice::ReadOnly))
        {
            // Can't open
            return false;
        }

        WidgetPackageSignature packageSignature(&signatureFile, publicKey);

        if (!packageSignature.isValid())
        {
            return false;
        }

        QStringList packagePaths = packageSignature.packagePaths();
        for(QStringList::ConstIterator it = packagePaths.constBegin();
            it != packagePaths.constEnd();
            it++)
        {
            QFile packageFile(archive.unpackedFilePath(*it));

            if (!packageFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
            {
                // Can't open the package file
                return false;
            }

            if (!packageSignature.entryForPackagePath(*it).verify(&packageFile))
            {
                // Signature didn't match
                return false;
            }
        }

        return true;
    }
}

namespace WebWidget
{
    UpdateCache::UpdateCache(const QDir &rootDirectory, const RsaPublicKey &publicKey) :
        mRootDirectory(rootDirectory),
        mPublicKey(publicKey)
    {
        loadManifest();

        // Clean up our update cache in the background
        QtConcurrent::run(this, &UpdateCache::reconcileUpdateCache);
    }

    UnpackedUpdate UpdateCache::installedUpdate(const QUrl &updateUrl) const
    {
        QMutexLocker locker(&mInstalledUpdatesLock);
        return mInstalledUpdates[updateUrl];
    }

    UpdateInstallation* UpdateCache::installUpdate(const UpdateIdentifier &identifier, QNetworkReply *source)
    {
        return new UpdateInstallation(this, identifier, source);
    }
        
    bool UpdateCache::uninstallUpdate(const UpdateIdentifier &identifier)
    {
        {
            QMutexLocker locker(&mInstalledUpdatesLock);
            UnpackedUpdate unpackedUpdate(mInstalledUpdates[identifier.updateUrl()]);

            if (unpackedUpdate.isNull() || 
                (unpackedUpdate.identifier() != identifier))
            {
                // Couldn't find the update
                // It could be already uninstalled or shadowed by a newer update
                return false;
            }

            // Remove this update from our list of registered updates
            // The next time we start up the update's files will removed during reconciliation
            mInstalledUpdates.remove(identifier.updateUrl());
        }

        // Save the manifest to disk
        saveManifest();

        return true;
    }
        
    UpdateError UpdateCache::unpackAndRegisterUpdate(const QString &packageFileName, const UpdateIdentifier &updateIdentifier)
    {
        // Don't collide with reconciliations
        QReadLocker locker(&mFilesystemContentLock);

        // Has this already been installed?
        UnpackedUpdate existingUpdate(installedUpdate(updateIdentifier.updateUrl()));

        if (!existingUpdate.isNull() && (existingUpdate.identifier() == updateIdentifier))
        {
            // Don't touch the existing version, it might be in use!
            return UpdateAlreadyInstalled;
        }

        QString unpackPath(mRootDirectory.absoluteFilePath(directoryNameForUpdate(updateIdentifier)));

        // Try to unpack the update
        UnpackedUpdate update(UnpackedUpdate::unpackFromCompressedPackage(
            updateIdentifier,
            unpackPath,
            packageFileName));

        if (update.isNull())
        {
            return UpdateUnpackFailed;
        }

        if (!validateUpdateSignature(update, mPublicKey))
        {
            // Bad signature
            update.remove();
            return UpdateInitialSignatureVerificationFailed;
        }
        
        // Save the update to our list of installed updates
        {
            QMutexLocker locker(&mInstalledUpdatesLock);
            mInstalledUpdates[updateIdentifier.updateUrl()] = update;
        }
        
        // Write the manifest out to disk
        saveManifest();

        return UpdateNoError;
    }
                
    bool UpdateCache::saveManifest()
    {
        QFile manifest(mRootDirectory.absoluteFilePath(InstalledUpdateManifestName));

        if (!manifest.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            return false;
        }

        QMutexLocker locker(&mInstalledUpdatesLock);

        QDataStream stream(&manifest);
        stream.setVersion(InstalledUpdateManifestVersion);
        stream << InstalledUpdateManifestMagic;

        for(InstalledUpdateHash::const_iterator it = mInstalledUpdates.constBegin();
            it != mInstalledUpdates.constEnd();
            it++)
        {
            UpdateIdentifier identifier(it->identifier());

            stream << identifier;
        }
        
        return (stream.status() == QDataStream::Ok);
    }

    bool UpdateCache::loadManifest()
    {
        QMutexLocker locker(&mInstalledUpdatesLock);

        // Always clear our installed updates for consistency
        mInstalledUpdates.clear();

        QFile manifest(mRootDirectory.absoluteFilePath(InstalledUpdateManifestName));

        if (!manifest.open(QIODevice::ReadOnly))
        {
            return false;
        }

        QDataStream stream(&manifest);
        stream.setVersion(InstalledUpdateManifestVersion);

        unsigned int magic;
        stream >> magic;
        
        if ((stream.status() != QDataStream::Ok) ||
            (magic != InstalledUpdateManifestMagic))
        {
            // Wrong magic
            return false;
        }

        while (!stream.atEnd())
        {
            UpdateIdentifier updateIdentifier;

            stream >> updateIdentifier;

            if (stream.status() != QDataStream::Ok)
            {
                mInstalledUpdates.clear();
                return false;
            }

            QString unpackedPath(mRootDirectory.absoluteFilePath(directoryNameForUpdate(updateIdentifier)));
            UnpackedUpdate update(updateIdentifier, unpackedPath);

            mInstalledUpdates[updateIdentifier.updateUrl()] = update;
        }

        return true;
    }

    void UpdateCache::reconcileUpdateCache()
    {
        // Make sure there are no concurrent unpacks while we reconcile
        QWriteLocker locker(&mFilesystemContentLock);

        // Get all of the existing package directories
        QSet<QString> packageDirectories(mRootDirectory.entryList(QDir::Dirs | QDir::NoDotAndDotDot).toSet());

        // Remove them from the set as we determine they're referenced
        {
            QMutexLocker locker(&mInstalledUpdatesLock);
            QMutableHashIterator<QUrl, UnpackedUpdate> it(mInstalledUpdates);

            while(it.hasNext())
            {
                UnpackedUpdate &update(it.next().value());
                const QString updateDirName(directoryNameForUpdate(update.identifier()));

                if (!packageDirectories.remove(updateDirName))
                {
                    // This update doesn't exist on the filesystem
                    it.remove();
                }
            }
        }

        // Any directories left over at this point are unreferenced
        for(QSet<QString>::const_iterator it = packageDirectories.constBegin();
            it != packageDirectories.constEnd();
            it++)
        {
            UnpackedArchive(mRootDirectory.absoluteFilePath(*it)).remove();
        }
    }
}
