#ifndef _WEBWIDGET_REMOTEUPDATEWATCHER_H
#define _WEBWIDGET_REMOTEUPDATEWATCHER_H

#include <QObject>
#include <QUrl>
#include <QSet>
#include <QHash>

#include "WebWidgetPluginAPI.h"

class QNetworkReply;
class QNetworkAccessManager;
class QNetworkRequest;

namespace WebWidget
{
    class UpdateCache;
    class UpdateIdentifier;

    ///
    /// Periodically checks remote URLs for updated widget packages
    ///
    class WEBWIDGET_PLUGIN_API RemoteUpdateWatcher : public QObject
    {
        Q_OBJECT
    public:
        ///
        /// Creates a new RemoteUpdateWatcher
        ///
        /// \param  updateCache    UpdateCache used to query the current installed version of widgets
        /// \param  networkAccess  QNetworkAccessManager to use for network access
        /// \param  parent         Optional QObject parent
        ///
        RemoteUpdateWatcher(const UpdateCache *updateCache, QNetworkAccessManager *networkAccess, QObject *parent = NULL);

        ///
        /// Watches a URL for package updates
        ///
        /// \param  updateUrl    URL to watch for updated widget packages
        /// \param  initialCheck If true an asynchronous update check will be started if the URL wasn't previously
        ///                      watched
        ///
        void addWatch(const QUrl &updateUrl, bool initialCheck = true);

        ///
        /// Stops watching a URL for package updates
        ///
        /// \param  updateUrl URL to stop watching for package updates
        ///
        void removeWatch(const QUrl &updateUrl);
    
    signals:
        ///
        /// Emitted when a new update is available
        ///
        /// \param  identifier  Unique identifer for the widget update
        /// \param  unreadReply Unread QNetworkReply containing the zipped update package. This should be deleted
        ///                     when it is no longer needed.
        ///
        void updateAvailable(const WebWidget::UpdateIdentifier &identifier, QNetworkReply *unreadReply);
    
    private slots:
        void updateRequestMetaDataChanged();
        void checkNow();

    private:
        void checkNow(const QUrl &updateUrl);
        bool sendUpdateRequest(QNetworkRequest &request, const QUrl &updateUrl, int redirectionCount = 0);

        QNetworkAccessManager *mNetworkAccess;
        const UpdateCache *mUpdateCache;

        QSet<QUrl> mWatchedUrls;
        
        // Record of all 301 redirects we've seen
        QHash<QUrl, QUrl> mPermanentRedirects;

        Q_DISABLE_COPY(RemoteUpdateWatcher)
    };
}

#endif
