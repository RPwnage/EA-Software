#ifndef _WEBWIDGET_WIDGETREPOSITORY_H
#define _WEBWIDGET_WIDGETREPOSITORY_H

#include <QDir>
#include <QUrl>
#include <QList>

#include "WebWidgetPluginAPI.h"
#include "RsaPublicKey.h"
#include "InstalledWidget.h"
#include "UriTemplate.h"
#include "UpdateIdentifier.h"
#include "UpdateError.h"

class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;

namespace WebWidget
{
    class UpdateCache;
    class RemoteUpdateWatcher;

    ///
    /// Provides InstalledWidget instances based on a widget name
    ///
    /// There are three important locations that make up a widget repository:
    /// - The <b>bundled root</b> contains the default versions of the widgets bundled with the application. Widget
    ///   package directories are expected to be named after the widget name.
    /// - The <b>update URL template</b> is a template used to build URLs to check for individual zipped widget update
    ///   packages. If a package is found at an update URL it is automatically downloaded and unpacked. The following
    ///   variables are available in the template:
    ///   - {widgetName} is replaced with the name of the widget being checked for updates. 
    ///   - {appName} is replaced with the application name as retrieved from QCoreApplication::applicationName()
    ///   - {appVersion} is replaced with the application name as retrieved from QCoreApplication::applicationVersion()
    /// - The <b>update cache</b> is a local directory which contains updates downloaded and unpacked from update URLs
    ///   This is managed by an internal UpdateCache instance.
    ///
    class WEBWIDGET_PLUGIN_API WidgetRepository : public QObject
    {
        Q_OBJECT
    public:
        ///
        /// Creates a new widget repository supporting updates from a remote repository
        ///
        /// \param  bundledRoots    Root directories of the widgets bundled with the application. These directories
        ///                         will be searched in order until a candidate directory is found.
        /// \param  updateUrl       URL template to use to check for widget update packages. 
        /// \param  publicKey       RSA public key used to verify widget updates.
        /// \param  updateCacheDir  Local cache directory for unpacked widget updates.
        /// \param  networkAccess   QNetworkAccessManager to use for network access. If none is specified then a
        ///                         default QNetworkAccessManger is constructed.
        /// \sa UpdateCache::UpdateCache
        ///
        WidgetRepository(const QList<QDir> &bundledRoots, const UriTemplate &updateUrl, const RsaPublicKey &publicKey, const QDir &updateCacheDir, QNetworkAccessManager *networkAccess = NULL);
        
        ///
        /// Creates a new widget repository without remote update support
        ///
        /// \param  bundledRoots      Root directories of the widgets bundled with the application. These directories
        ///                           will be searched in order until a candidate directory is found.
        ///
        explicit WidgetRepository(const QList<QDir> &bundledRoots);

        ~WidgetRepository();
    
        ///
        /// Returns the installed version of the given widget
        ///
        /// The following process is used to find an installed widget:
        /// -# The update cache is checked for an unpacked widget update. If one is found, it is returned
        /// -# The bundled root is checked for the default version of the widget. If one is found, it is returned
        /// -# A null InstalledWidget is returned
        ///
        /// \param  widgetName  Name of the widget to return
        /// \param  authority   Authority for the security origin. If none is specified the widgetName will be used.
        /// \return Latest installed version or null if the widget wasn't found in our repository
        ///
        InstalledWidget installedWidget(const QString &widgetName, QString authority = QString());

        ///
        /// Requests a periodic update check for the given widget name
        ///
        /// It is recommended this is called as early as possible. This increases the chances that an available widget
        /// update is downloaded and unpacked before the call to installedWidget().
        /// 
        /// \param  widgetName Name of the widget to check for updates
        ///
        void startPeriodicUpdateCheck(const QString &widgetName);

        ///
        /// Returns if updates from a remote repository are supported for this instance
        ///
        bool remoteUpdatesSupported() const;

    signals:
        ///
        /// Emitted whenever a widget update attempt starts
        ///
        /// \param identifer Identifier for the widget update
        ///
        void updateStarted(const WebWidget::UpdateIdentifier &identifer);

        ///
        /// Emitted whenever an attempted widget update finishes
        ///
        /// \param  identifier  Identifier for the widget update
        /// \param  error       UpdateNoError if the update was successful; an error code otherwise
        ///                         
        void updateFinished(const WebWidget::UpdateIdentifier &identifier, WebWidget::UpdateError error);
    
    private slots:
        void remoteUpdateAvailable(const WebWidget::UpdateIdentifier &identifier, QNetworkReply *);
        void updateInstallationFinished(WebWidget::UpdateError error);
        void updateVerificationFailed();

    private:
        ///
        /// Returns the update URL of the given widget
        ///
        /// \param  widgetName  Name of the widget
        /// \return Update URL for the widget
        ///
        QUrl updateUrlForWidget(const QString &widgetName) const;

        QList<QDir> mBundledRoots;

        UriTemplate mUpdateUrl;
        RsaPublicKey mPublicKey;

        // Remote update related
        QNetworkAccessManager *mNetworkAccess;
        UpdateCache *mUpdateCache;
        RemoteUpdateWatcher *mRemoteUpdateWatcher;

        Q_DISABLE_COPY(WidgetRepository);
    };
}

#endif
