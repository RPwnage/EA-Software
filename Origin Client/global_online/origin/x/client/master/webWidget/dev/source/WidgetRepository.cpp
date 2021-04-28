#include <QDebug>
#include <QNetworkAccessManager>
#include <QByteArray>
#include <QDataStream>
#include <QCoreApplication>
#include <QMetaType>

#include "WidgetRepository.h"
#include "UpdateCache.h"
#include "RemoteUpdateWatcher.h"
#include "WidgetDirectoryFileProvider.h"
#include "UpdateInstallation.h"
#include "UnpackedArchiveFileProvider.h"
#include "SignedWidgetFileProvider.h"

namespace
{
    const char *UpdateIdentifierProperty = "webwidget_updateidentifier";
}

namespace WebWidget
{
    WidgetRepository::WidgetRepository(const QList<QDir> &bundledRoots) :
        mBundledRoots(bundledRoots),
        mNetworkAccess(NULL),
        mUpdateCache(NULL),
        mRemoteUpdateWatcher(NULL)
    {
        qRegisterMetaType<UpdateIdentifier>("WebWidget::UpdateIdentifier");
    }
    
    WidgetRepository::WidgetRepository(const QList<QDir> &bundledRoots, const UriTemplate &updateUrl, const RsaPublicKey &publicKey, const QDir &updateCacheDir, QNetworkAccessManager *networkAccess) :
        mBundledRoots(bundledRoots),
        mUpdateUrl(updateUrl),
        mPublicKey(publicKey),
        mNetworkAccess(networkAccess),
        mUpdateCache(new UpdateCache(updateCacheDir, publicKey))
    {
        if (!mNetworkAccess)
        {
            // Create our own network access manager
            mNetworkAccess = new QNetworkAccessManager(this);
        }
        
        mRemoteUpdateWatcher = new RemoteUpdateWatcher(mUpdateCache, mNetworkAccess, this);

        // Hook up our update related signals 
        connect(mRemoteUpdateWatcher, SIGNAL(updateAvailable(const WebWidget::UpdateIdentifier &, QNetworkReply *)),
            this, SLOT(remoteUpdateAvailable(const WebWidget::UpdateIdentifier &, QNetworkReply *)));
    }

    WidgetRepository::~WidgetRepository()
    {
        delete mUpdateCache;
    }
    
    bool WidgetRepository::remoteUpdatesSupported() const
    {
        // These should all either be NULL or non-NULL but be paranoid
        return mUpdateCache && mRemoteUpdateWatcher && mNetworkAccess;
    }

    InstalledWidget WidgetRepository::installedWidget(const QString &widgetName, QString authority) 
    {
        // We don't quite follow the Widgets URI working draft's recommendation here
        // They recommend using a UUID for the authority to allow for mapping multiple widget instances to a single
        // package. From our perspective we not only want one package to always map to the same authority but also
        // for multiple versions of the same package to map to the same authority. This allows sharing of stored data
        // between multiple versions.
        // The concerns about the widget URIs being guessable don't apply as WidgetNetworkAccessManager is created per
        // WidgetView and will not allow cross-widget requests.
        if (authority.isNull())
        {
            authority = widgetName;
        }

        SecurityOrigin origin("widget", authority, SecurityOrigin::UnknownPort);
        
        if (remoteUpdatesSupported())
        {
            const QUrl updateUrl(updateUrlForWidget(widgetName));

            UnpackedUpdate unpackedUpdate(mUpdateCache->installedUpdate(updateUrl));
            
            if (!unpackedUpdate.isNull())
            {
                // Create a file provider that just reads directly from the package
                UnpackedArchiveFileProvider *unverifiedFileProvider = new UnpackedArchiveFileProvider(unpackedUpdate);

                // Now wrap it in a file provider to verify the package signature
                SignedWidgetFileProvider *signedFileProvider = 
                    new SignedWidgetFileProvider(unverifiedFileProvider, mPublicKey);

                // Serialize the update identifier as we'll need it to uninstall the update if verification fails
                QByteArray serializedUpdateIdentifier;
                QDataStream updateIdentifierStream(&serializedUpdateIdentifier, QIODevice::WriteOnly);
                updateIdentifierStream << unpackedUpdate.identifier();

                // Tag the file provider with the update identifier
                signedFileProvider->setProperty(UpdateIdentifierProperty, serializedUpdateIdentifier);

                // Uninstall the widget if verification fails
                connect(signedFileProvider, SIGNAL(verificationFailed(WidgetFileProvider*)),
                        this, SLOT(updateVerificationFailed()));
                
                return InstalledWidget(signedFileProvider, origin);
            }
        }
        
        // If there's no update search for a bundled version
        for(QList<QDir>::const_iterator it = mBundledRoots.constBegin();
            it != mBundledRoots.constEnd();
            it++)
        {
            const QString rootPath(QDir(*it).absoluteFilePath(widgetName));

            // Instead of testing for the directory existing test for config.xml: the only file required to exist
            // This is to work around file engines that might not support directory access
            if (QDir(rootPath).exists("config.xml"))
            {
                WidgetFileProvider *dirFileProvider = new WidgetDirectoryFileProvider(rootPath);
                return InstalledWidget(dirFileProvider, origin);
            }
        }

        // No update or bundled version available
        return InstalledWidget();
    }
    
    void WidgetRepository::startPeriodicUpdateCheck(const QString &widgetName)
    {
        if (remoteUpdatesSupported())
        {
            mRemoteUpdateWatcher->addWatch(updateUrlForWidget(widgetName));
        }
    }
        
    QUrl WidgetRepository::updateUrlForWidget(const QString &widgetName) const
    {
        UriTemplate::VariableMap updateUrlVariables;
        updateUrlVariables["widgetName"] = widgetName;
        updateUrlVariables["appName"] = QCoreApplication::applicationName();
        updateUrlVariables["appVersion"] = QCoreApplication::applicationVersion();

        return mUpdateUrl.expand(updateUrlVariables);
    }

    void WidgetRepository::remoteUpdateAvailable(const WebWidget::UpdateIdentifier &identifier, QNetworkReply *unreadReply)
    {
        // Stop checking for updates to this widget while we install
        mRemoteUpdateWatcher->removeWatch(identifier.updateUrl());
        
        // Notify that we've started an update
        emit updateStarted(identifier);

        UpdateInstallation *installation = mUpdateCache->installUpdate(identifier, unreadReply);
        connect(installation, SIGNAL(finished(WebWidget::UpdateError)), this, SLOT(updateInstallationFinished(WebWidget::UpdateError)));
    }
        
    void WidgetRepository::updateInstallationFinished(WebWidget::UpdateError error)
    {
        UpdateInstallation* installation = dynamic_cast<UpdateInstallation*>(sender());
        installation->deleteLater();

        // Tell any interested parties about the update
        emit updateFinished(installation->updateIdentifier(), error);

        // Start checking for updates again but don't do an initial check
        mRemoteUpdateWatcher->addWatch(installation->updateIdentifier().updateUrl(), false);
    }
        
    void WidgetRepository::updateVerificationFailed()
    {
        SignedWidgetFileProvider *fileProvider = dynamic_cast<SignedWidgetFileProvider*>(sender());

        if (!fileProvider)
        {
            // Not what we expected
            return;
        }

        // Try to get our update identifier
        const QByteArray serializedUpdateIdentifier(fileProvider->property(UpdateIdentifierProperty).toByteArray());

        if (serializedUpdateIdentifier.isNull())
        {
            return;
        }

        // Deserialize it
        QDataStream updateIdentifierStream(serializedUpdateIdentifier);
        UpdateIdentifier updateIdentifier;

        updateIdentifierStream >> updateIdentifier;

        if (!updateIdentifier.isNull())
        {
            mUpdateCache->uninstallUpdate(updateIdentifier);
        }
    }
}
