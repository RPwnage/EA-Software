#include <limits>
#include <QDateTime>
#include <QMetaObject>
#include <QFileInfo>
#include <QDebug>

#include "WidgetNetworkAccessManager.h"
#include "WidgetPackage.h"

namespace WebWidget
{
    WidgetNetworkAccessManager::WidgetNetworkAccessManager(const QString &authority, const WidgetPackageReader &packageReader, QNetworkAccessManager *outgoingAccess, QObject *parent) :
        QNetworkAccessManager(parent),
        mOutgoingAccess(outgoingAccess),
        mAuthority(authority),
        mPackageReader(packageReader)
    {
    }
    
    QNetworkReply* WidgetNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
    {
        const QUrl url(req.url());
        const QString scheme(url.scheme().toLower());

        if (scheme == "widget")
        {
            if (op != GetOperation)
            {
                return new FailedNetworkReply(op, req, QNetworkReply::UnknownContentError, 501, "Not Implemented");
            }

            if (url.authority() != mAuthority)
            {
                // Wrong authority!
                return denyRequest(op, req);
            }

            // Find the filesystem path
            WidgetPackageFile packageFile(mPackageReader.findFile(url.path()));

            if (packageFile.exists())
            {
                // Rewrite the path and allow it
                return new WidgetSchemeNetworkReply(packageFile, op, req);
            }
            else
            {
                // Couldn't find the file in the package
                return new FailedNetworkReply(op, req, QNetworkReply::ContentNotFoundError, 404, "Not Found");
            }
        }
        else if (scheme == "data")
        {
            // These contain their entire response in the URI without hitting network or local resources
            // Use our superclass to answer them without invoking the outgoing access manager
            return QNetworkAccessManager::createRequest(op, req, outgoingData);
        }
        else if (mOutgoingAccess)
        {
            if ((scheme == "http") || (scheme == "https"))
            {
                QList<AccessRequest> accessRequests(mPackageReader.configuration().accessRequests());

                // See if the widget has requested access to this resource
                for(QList<WebWidget::AccessRequest>::const_iterator it = accessRequests.constBegin();
                    it != accessRequests.constEnd();
                    it++)
                {
                    if (it->appliesToResource(url))
                    {
                        // Allow the access
                        return allowRequest(op, req, outgoingData);
                    }
                }

                return denyRequest(op, req);
            }
        }

        // Deny the request
        return denyRequest(op, req);
    }

    QNetworkReply* WidgetNetworkAccessManager::allowRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
    {
        switch(op)
        {
        case HeadOperation:
            return mOutgoingAccess->head(req);
        case GetOperation:
            return mOutgoingAccess->get(req);
        case PutOperation:
            return mOutgoingAccess->put(req, outgoingData);
        case PostOperation:
            return mOutgoingAccess->post(req, outgoingData);
        case DeleteOperation:
            return mOutgoingAccess->deleteResource(req);
        case CustomOperation:
            {
                const QByteArray customVerb(req.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray());
                return mOutgoingAccess->sendCustomRequest(req, customVerb, outgoingData);
            }
        default:
            break;
        }

        // Don't know how to proxy this
        return denyRequest(op, req);
    }
    
    QNetworkReply *WidgetNetworkAccessManager::denyRequest(Operation op, const QNetworkRequest &req)
    {
        return new FailedNetworkReply(op, req, QNetworkReply::ContentAccessDenied, 403, "Forbidden");
    }

    FailedNetworkReply::FailedNetworkReply(QNetworkAccessManager::Operation op, const QNetworkRequest &req, NetworkError networkError, unsigned int statusCode, const QString &message)
    {
        // Do basic set up
        setRequest(req);
        setUrl(req.url());
        setOperation(op);
        
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, statusCode);
        setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, message);
        setError(networkError, message);

        // Simulate failure when we next enter the event loop
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection, 
            Q_ARG(QNetworkReply::NetworkError, networkError));

        setFinished(true);
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
    }
        
    void FailedNetworkReply::abort()
    {
        // Don't do anything - we'll be denied next event loop
    }

    qint64 FailedNetworkReply::readData(char *, qint64)
    {
        return -1;
    }
        
    WidgetSchemeNetworkReply::WidgetSchemeNetworkReply(const WidgetPackageFile &packageFile, QNetworkAccessManager::Operation op, const QNetworkRequest &req) :
        mBackingFile(packageFile.backingFileInfo().absoluteFilePath()),
        mFileSize(0),
        mFilePosition(0)
    {
        // Do basic set up
        setRequest(req);
        setUrl(req.url());
        setOperation(op);

        QNetworkReply::open(QIODevice::ReadOnly);

        if (!mBackingFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
        {
            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 404);
            setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, "Not Found");

            setError(ContentNotFoundError, "Not Found");

            // We have failed
            QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection, 
                Q_ARG(QNetworkReply::NetworkError, ContentNotFoundError));
        }
        else
        {
            // Get our file metadata
            QFileInfo fileInfo(packageFile.backingFileInfo());
            mFileSize = fileInfo.size();

            setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
            setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, "OK");

            setHeader(QNetworkRequest::ContentLengthHeader, mFileSize);
            setHeader(QNetworkRequest::LastModifiedHeader, fileInfo.lastModified());

            if (!packageFile.mediaType().isNull())
            {
                QByteArray mediaType(packageFile.mediaType());

                if (!packageFile.encoding().isNull())
                {
                    // Add our character encoding on
                    mediaType += "; charset=" + packageFile.encoding();
                }

                // Set our media type
                setHeader(QNetworkRequest::ContentTypeHeader, mediaType);
            }

            setError(NoError, "Success");

            // We've succeeded - let the caller know
            QMetaObject::invokeMethod(this, "metaDataChanged", Qt::QueuedConnection);
            QMetaObject::invokeMethod(this, "downloadProgress", Qt::QueuedConnection,
                Q_ARG(qint64, mFileSize), Q_ARG(qint64, mFileSize));
            QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
        }
            
        setFinished(true);
        QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
    }
    
    void WidgetSchemeNetworkReply::abort()
    {
        // Do nothing
    }

    bool WidgetSchemeNetworkReply::isSequential() const
    {
        return true;
    }
        
    qint64 WidgetSchemeNetworkReply::size() const
    {
        return mFileSize;
    }
    
    qint64 WidgetSchemeNetworkReply::bytesAvailable() const 
    {
        // Include QIODevice's buffer
        // This is important as QtWebKit will peek to sniff mime types
        return QNetworkReply::bytesAvailable() + mFileSize - mFilePosition;
    }

    qint64 WidgetSchemeNetworkReply::readData(char *data, qint64 maxSize)
    {
        qint64 bytesRead = mBackingFile.read(data, maxSize);

        if (bytesRead > 0)
        {
            mFilePosition += bytesRead;
        }

        return bytesRead;
    }
}
