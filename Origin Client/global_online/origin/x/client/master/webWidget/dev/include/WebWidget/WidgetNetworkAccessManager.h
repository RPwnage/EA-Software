#ifndef _WEBWIDGET_WIDGETNETWORKACCESSMANAGER_H
#define _WEBWIDGET_WIDGETNETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

#include "WebWidgetPluginAPI.h"
#include "WidgetPackageReader.h"

namespace WebWidget
{
    class WidgetPackageFile;

    ///
    /// Network access manager for handling resource requests from widgets
    ///
    /// Two types of resources can be requested by widgets: 
    /// - Widget package files using the widget:// URI scheme. Requests to the widget:// scheme are passed directly to 
    ///   WidgetPackageReader. This is specified at http://www.w3.org/TR/widgets-uri/
    /// - Network requests using the HTTP or HTTPS scheme. Network requests are validated against the widget's access
    ///   requests retrieved from WidgetConfiguration::accessRequests(). Approved requests are then sent to the outgoing
    ///   network access manager. This is specified at http://www.w3.org/TR/widgets-access/
    ///
    class WEBWIDGET_PLUGIN_API WidgetNetworkAccessManager : public QNetworkAccessManager
    {
    public:
        ///
        /// Creates a new widget access manager for the given widget
        ///
        /// \param  authority       Expected authority for widget:// URIs. 
        ///                         All other authorities will be denied with 403 errors.
        /// \param  packageReader   Package reader used to access the widget package.
        ///                         Used to satisfy widget:// requests and enumerate widget access requests
        /// \param  outgoingAccess  Network access manager to send approved network requests to.
        ///                         If this is NULL outgoing network access is disabled.
        /// \param  parent          Qt parent object
        ///
        WidgetNetworkAccessManager(const QString &authority,
            const WidgetPackageReader &packageReader,
            QNetworkAccessManager *outgoingAccess = NULL, 
            QObject *parent = NULL);

    private:
        QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData); 

        ///
        /// Proxies an allowed request to the outgoing access manager
        /// 
        QNetworkReply *allowRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData);
        
        ///
        /// Denies a request by returning a failed reply
        ///
        QNetworkReply *denyRequest(Operation op, const QNetworkRequest &req);

        QNetworkAccessManager *mOutgoingAccess;
        QString mAuthority;
        WidgetPackageReader mPackageReader;
    };
    
    ///
    /// Network reply for immediately failed requests
    ///
    class WEBWIDGET_PLUGIN_API FailedNetworkReply : public QNetworkReply
    {
        friend class WidgetNetworkAccessManager;

    public:
        ///
        /// Aborts the request
        ///
        /// This is a noop as the reply is constructed as finished.
        ///
        void abort();

    private:
        FailedNetworkReply(QNetworkAccessManager::Operation op, const QNetworkRequest &req, NetworkError networkError, unsigned int statusCode, const QString &message);
        qint64 readData(char *data, qint64 maxSize);
    };

    ///
    /// Network reply for the widget:// URI scheme
    ///
    /// See http://www.w3.org/TR/widgets-uri/
    ///
    class WEBWIDGET_PLUGIN_API WidgetSchemeNetworkReply : public QNetworkReply
    {
        friend class WidgetNetworkAccessManager;

    public:
        void abort();
        bool isSequential() const;
        qint64 bytesAvailable() const;
        qint64 size() const;

    private:
        WidgetSchemeNetworkReply(const WidgetPackageFile &, QNetworkAccessManager::Operation op, const QNetworkRequest &req);
        qint64 readData(char *data, qint64 maxSize);

    private:
        QFile mBackingFile;

        qint64 mFileSize;
        qint64 mFilePosition;
    };
}

#endif
