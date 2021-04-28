#ifndef _WEBWIDGET_UPDATECACHE_H
#define _WEBWIDGET_UPDATECACHE_H

#include <QObject>
#include <QDir>
#include <QUrl>
#include <QHash>
#include <QMutex>
#include <QReadWriteLock>

#include "WebWidgetPluginAPI.h"
#include "UnpackedUpdate.h"
#include "UpdateError.h"
#include "RsaPublicKey.h"

class QNetworkReply;

namespace WebWidget
{
    class UpdateIdentifier;
    class UpdateInstallation;

    ///
    /// Manages the local cache of downloaded and unpacked widget updates
    ///
    class WEBWIDGET_PLUGIN_API UpdateCache : public QObject
    {
        Q_OBJECT
        
        friend class UpdateInstallation;
    public:
        ///
        /// Creates a new UpdateCache managing the passed directory
        ///
        /// UpdateCache expects exclusive access to its root directory. Multiple concurrent instances managing the
        /// same root directory will result in undefined behavior. Unrecognized files and directories may be deleted.
        ///
        /// \param  rootDirectory  Root directory of the widget update cache. If the directory does not exist it will
        ///                        created
        /// \param  publicKey      RSA public key used to verify updates
        ///
        UpdateCache(const QDir &rootDirectory, const RsaPublicKey &publicKey);

        ///
        /// Returns the installed widget update for the given update URL
        ///
        /// \param  updateUrl URL used to check for the expected update
        /// \return Last installed update for the passed updateUrl
        ///
        /// \sa installUpdate()
        ///
        UnpackedUpdate installedUpdate(const QUrl &updateUrl) const;

        ///
        /// Installs a widget update from the QNetworkReply for a zipped widget package
        /// 
        /// Installing a widget update will shadow any previous version installed from the same update URL. However,
        /// any existing UnpackedUpdate instances pointing to previous versions will remain valid.
        ///
        /// This operation is performed asynchronously. The returned object can be used to check for completion
        ///
        /// \param identifier Identifier of the update to installed 
        /// \param source     Source network reply to install the update from. 
        ///
        /// \sa installedUpdate()
        ///
        UpdateInstallation* installUpdate(const UpdateIdentifier &identifier, QNetworkReply *source);

        ///
        /// Uninstalls a widget update with the given update identifier
        ///
        /// Any current users of the update will be unaffected as the update's files are not removed from disk until 
        /// the next time UpdateCache is instantiated for the same root directory. 
        ///
        /// \param  identifier Identifier of the update to remove
        /// \return True if the update was found and uninstalled, false otherwise
        ///
        /// \sa UnpackedUpdate::identifier()
        ///
        bool uninstallUpdate(const UpdateIdentifier &identifier);

    private:
        ///
        /// Unpacks a zipped package to the cache and register it  
        ///
        /// This can be called from a different thread
        ///
        UpdateError unpackAndRegisterUpdate(const QString &packageFileName, const UpdateIdentifier &updateIdentifier);

        ///
        /// Saves our installed update manifest to disk
        ///
        bool saveManifest();

        ///
        /// Loads our installed update manifest to disk
        ///
        bool loadManifest();

        ///
        /// Reconciles the installed updates hash with the files on disk
        ///
        /// Reconciliation does two things:
        /// -# Removes entries from the installed updates hash that don't exist on the filesystem
        /// -# Removes directories in the local 
        ///
        /// This can be long running so should be run outside of the UI thread if possible. It is unsafe to run this
        /// if there are any WidgetViews possibly still viewing the unreferenced version
        ///
        void reconcileUpdateCache();

        typedef QHash<QUrl, UnpackedUpdate> InstalledUpdateHash;
        InstalledUpdateHash mInstalledUpdates;
        // This must be taken after mFilesystemContentLock if they are to be nested
        mutable QMutex mInstalledUpdatesLock;

        QDir mRootDirectory;
        RsaPublicKey mPublicKey;

        // This is to prevent conflicting unpacks to the root directory during reconciliation
        // Unpacks should hold this for read, reconcile will hold it for write
        QReadWriteLock mFilesystemContentLock;
    };
}

#endif
