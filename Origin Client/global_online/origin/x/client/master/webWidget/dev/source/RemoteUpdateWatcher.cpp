#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include <QTimer>

#include "RemoteUpdateWatcher.h"
#include "UpdateCache.h"

namespace
{
    // Maximum number of redirects we'll follow
    const int MaximumRedirectCount = 10;

    // Time between update checks in milliseconds
    const int CheckIntervalMs = 60 * 60 * 1000;

    // Define the custom attributes we tag to our update requests
    const QNetworkRequest::Attribute BaseCustomAttribute = QNetworkRequest::Attribute(QNetworkRequest::User + 1000);

    const QNetworkRequest::Attribute RedirectCountAttribute = QNetworkRequest::Attribute(BaseCustomAttribute);
    const QNetworkRequest::Attribute UpdateUrlAttribute = QNetworkRequest::Attribute(BaseCustomAttribute + 1);
}

namespace WebWidget
{
    RemoteUpdateWatcher::RemoteUpdateWatcher(const UpdateCache *updateCache, QNetworkAccessManager *networkAccess, QObject *parent) :
        QObject(parent),
        mNetworkAccess(networkAccess),
        mUpdateCache(updateCache)
    {
        QTimer *intervalCheckTimer = new QTimer(this);

        intervalCheckTimer->setInterval(CheckIntervalMs);
        connect(intervalCheckTimer, SIGNAL(timeout()), this, SLOT(checkNow()));
        intervalCheckTimer->start();
    }
        
    void RemoteUpdateWatcher::addWatch(const QUrl &updateUrl, bool initialCheck)
    {
        if (mWatchedUrls.contains(updateUrl))
        {
            // Nothing to do
            return;
        }

        mWatchedUrls << updateUrl;
        
        if (initialCheck)
        {
            // Do a check now
            checkNow(updateUrl);
        }
    }
    
    void RemoteUpdateWatcher::removeWatch(const QUrl &updateUrl)
    {
        mWatchedUrls.remove(updateUrl);
    }
    
    void RemoteUpdateWatcher::checkNow()
    {
        // Check everything at once because they're likely on the same server so we can use persistent connections.
        // This helps with the extremely common case of no updates being available
        for(QSet<QUrl>::const_iterator it = mWatchedUrls.constBegin();
            it != mWatchedUrls.constEnd();
            it++)
        {
            checkNow(*it);
        }
    }

    void RemoteUpdateWatcher::checkNow(const QUrl &updateUrl)
    {
        QNetworkRequest req(updateUrl);
        sendUpdateRequest(req, updateUrl);
    }

    bool RemoteUpdateWatcher::sendUpdateRequest(QNetworkRequest &request, const QUrl &updateUrl, int redirectionCount)
    {
        if (redirectionCount > MaximumRedirectCount)
        {
            // Fail
            return false;
        }
        
        // Have we hit a 301 redirect for this URL before?
        QUrl cachedRedirectTarget = mPermanentRedirects[request.url()];
        if (cachedRedirectTarget.isValid())
        {
            // Simulate a redirect without hitting the network
            QNetworkRequest newRequest(request);
            request.setUrl(cachedRedirectTarget);

            return sendUpdateRequest(request, updateUrl, redirectionCount + 1);
        }

        // Always use the network; we do our own cache management
        request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, false);

        // Set our tracking attributes
        request.setAttribute(UpdateUrlAttribute, updateUrl);
        request.setAttribute(RedirectCountAttribute, redirectionCount);
        
        // Hint about the media type we accept
        // Servers typically aren't configured for application/widget yet so we have to be liberal here and allow the
        // fallback to */*
        request.setRawHeader("Accept", "application/widget,*/*;q=0.9");

        // Look up the our cached widget
        UnpackedUpdate cachedWidget(mUpdateCache->installedUpdate(updateUrl));

        // Do we have an ETag for an existing installed update?
        if (!cachedWidget.isNull())
        {
            UpdateIdentifier updateIdentifier(cachedWidget.identifier());

            // Does it apply to this URL?
            if  (updateIdentifier.finalUrl() == request.url())
            {
                request.setRawHeader("If-None-Match", updateIdentifier.etag());
            }
        }
        
        QNetworkReply *reply = mNetworkAccess->get(request);
        connect(reply, SIGNAL(metaDataChanged()), this, SLOT(updateRequestMetaDataChanged()));

        return true;
    }
    
    void RemoteUpdateWatcher::updateRequestMetaDataChanged()
    {
        QNetworkReply *reply = dynamic_cast<QNetworkReply*>(sender());

        // We don't need any more signals
        disconnect(reply, NULL, this, NULL);

        // Find our update URL
        QUrl updateUrl(reply->request().attribute(UpdateUrlAttribute).toUrl());

        if (!mWatchedUrls.contains(updateUrl))
        {
            // This isn't being watched anymore
            // WidgetRepository depends on removeWatch() during update installation to prevent concurrent update
            // installations. There's a small window where we might try to removeWatch() while a check is in progress
            // and accidentally start a concurrent installation which would be bad.
            reply->deleteLater();
            return;
        }

        if (reply->error() != QNetworkReply::NoError)
        {
            // Some error occurred; we can't continue
            reply->deleteLater();
            return;
        }

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        if (statusCode == 200)
        {
            // Build an update identifier
            QUrl finalUrl(reply->url());
            QByteArray etag(reply->rawHeader("ETag"));

            if (etag.isNull())
            {
                // We need an ETag to register the update
                qDebug() << "Update URL" << finalUrl << "did not return an ETag";
                reply->deleteLater();
                return;
            }
            
            UpdateIdentifier identifier(updateUrl, finalUrl, etag);
            
            // Don't delete the reply here
            emit updateAvailable(identifier, reply);
            return;
        }
        
        // This reply isn't useful beyond this function
        reply->deleteLater();
        
        if ((statusCode == 301) || (statusCode == 302) || (statusCode == 303) || (statusCode == 307))
        {
            // Find our target URL
            QUrl targetUrl(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
            
            if (statusCode == 301)
            {
                // Record this redirection for future use
                mPermanentRedirects[reply->request().url()] = targetUrl;
            }
            
            // Follow the redirection
            QNetworkRequest req(targetUrl);

            // Set up our tracking attributes
            int currentRedirectCount = reply->request().attribute(RedirectCountAttribute).toInt();

            sendUpdateRequest(req, updateUrl, currentRedirectCount + 1);
        }
        else if (statusCode == 304)
        {
            // Not modified
        }
        else if ((statusCode == 404) || (statusCode == 403))
        {
            // Amazon returns 403 for unlistable buckets when a file doesn't exist
        }
        else
        {
            qWarning() << "Unexpected status code" << statusCode << "while checking for widget update";
        }
    }
}
