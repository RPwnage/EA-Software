#include <QMetaObject>
#include <QNetworkRequest>

#include "services/rest/CloudSyncServiceResponses.h"
#include "services/debug/DebugService.h"
#include "services/network/NetworkAccessManager.h"
#include "services/compression/GzipCompress.h"
#include "engine/cloudsaves/AuthorizedS3Transfer.h"
#include "engine/cloudsaves/S3Operation.h"

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    // Gzip must be this much smaller to be a win due to the 
    // "Content-Encoding: gzip" header
    static const unsigned int GzipHeaderOverhead = 24;

    static QNetworkRequest requestForSignature(const Origin::Services::CloudSyncServiceSignedTransfer &signature)
    {
        QNetworkRequest req(signature.url());

        for(Origin::Services::HeadersMap::const_iterator it = signature.headers().constBegin();
            it != signature.headers().constEnd();
            it++)
        {
            req.setRawHeader(it.key(), it.value());
        }

        return req;
    }

    void S3Operation::replyFinished()
    {
        finalize();

        if (m_reply->error() == QNetworkReply::NoError)
        {
            // Something 2xx-y
            emit(succeeded());
        }
        else
        {
            emit(failed());
        }
    }

    S3UploadOperation::S3UploadOperation(const AuthorizedS3Upload &transfer, const Origin::Services::CloudSyncServiceSignedTransfer &signature)
    {
        m_localFile.setFileName(transfer.sourcePath());

        if (!m_localFile.open(QIODevice::ReadOnly))
        {
            m_hasLocalFailure = true;
            return;
        }

        if (m_localFile.size() != transfer.expectedSize())
        {
            // The file size has changed under us!
            m_hasLocalFailure = true;
            return;
        }

        QNetworkRequest req(requestForSignature(signature));
        req.setRawHeader("Expect", "100-continue");

        m_reply = NULL;
        if (transfer.gzip())
        {
            bool gzipSucceeded = transfer.performGzip();
            QFile *zipFile(transfer.zipFile());

            if (gzipSucceeded &&
                (zipFile->size() + GzipHeaderOverhead) < transfer.expectedSize())
            {
                // Use our gzipped data
                req.setRawHeader("Content-Encoding", "gzip");

                // Don't double buffer this
                req.setHeader(QNetworkRequest::ContentLengthHeader, zipFile->size());
                req.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, true);

                // Open the file for the http put
                zipFile->open(QIODevice::ReadOnly);

                m_reply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->put(req, zipFile);
            }
        }

        if (!m_reply)
        {
            // Gzip was either disabled or not a win; upload the file directly
            m_reply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->put(req, &m_localFile);
        }

        ORIGIN_VERIFY_CONNECT(m_reply, SIGNAL(finished()), this, SLOT(replyFinished()));
        ORIGIN_VERIFY_CONNECT(m_reply, SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(progress(qint64, qint64)));
    }

    void S3UploadOperation::finalize()
    {
        m_localFile.close();
    }
    
    S3DownloadOperation::S3DownloadOperation(const AuthorizedS3Download &transfer, const Origin::Services::CloudSyncServiceSignedTransfer &signature) 
    {
        m_localFile.setFileName(transfer.destPath());

        if (!m_localFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            m_hasLocalFailure = true;
            return;
        }

        QNetworkRequest req(requestForSignature(signature));

        m_reply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->get(req);

        // The disk itself is our cache in the case of cloud saves
        // The Amazon auth seems to also break caching
        // Disable caching entirely to avoid the disk I/IO usage on our HTTP cache
        req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QVariant(QNetworkRequest::AlwaysNetwork));
        req.setAttribute(QNetworkRequest::CacheSaveControlAttribute, QVariant(false));

        ORIGIN_VERIFY_CONNECT(m_reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
        ORIGIN_VERIFY_CONNECT(m_reply, SIGNAL(finished()), this, SLOT(replyFinished()));
        ORIGIN_VERIFY_CONNECT(m_reply, SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(progress(qint64, qint64)));
    }
    
    void S3DownloadOperation::readyRead()
    {
        // Really?
        if (m_localFile.write(m_reply->readAll()) == -1)
        {
            m_hasLocalFailure = true;
            emit failed();
        }
    }
    
    void S3DownloadOperation::finalize()
    {
        m_localFile.close();
    }
        
    S3DeleteOperation::S3DeleteOperation(const AuthorizedS3Delete &, const Origin::Services::CloudSyncServiceSignedTransfer &signature)
    {
        QNetworkRequest req(requestForSignature(signature));

        m_reply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->deleteResource(req);

        ORIGIN_VERIFY_CONNECT(m_reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    }
    
    S3PutOperation::S3PutOperation(const AuthorizedS3Put &transfer, const Origin::Services::CloudSyncServiceSignedTransfer &signature)
    {
        QNetworkRequest req(requestForSignature(signature));
        
        if (!transfer.contentType().isNull())
        {
            req.setHeader(QNetworkRequest::ContentTypeHeader, transfer.contentType());
        }

        QByteArray toUpload;

        if (transfer.gzip())
        {
            toUpload = Services::gzipData(transfer.data());
            req.setRawHeader("Content-Encoding", "gzip");
        }
        else
        {
            toUpload = transfer.data();
        }

        m_reply = Origin::Services::NetworkAccessManager::threadDefaultInstance()->put(req, toUpload);

        ORIGIN_VERIFY_CONNECT(m_reply, SIGNAL(finished()), this, SLOT(replyFinished()));
        ORIGIN_VERIFY_CONNECT(m_reply, SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(progress(qint64, qint64)));
    }
}
}
}
