#ifndef _CLOUDSAVES_AUTHORIZEDS3TRANSFER
#define _CLOUDSAVES_AUTHORIZEDS3TRANSFER

#include <QString>
#include <QByteArray>
#include <QUrl>
#include <QFuture>

#include "engine/cloudsaves/S3Operation.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class CloudSyncServiceSignedTransfer;
    }
}

namespace Origin
{
namespace Engine
{ 

namespace CloudSaves
{
    class ORIGIN_PLUGIN_API AuthorizedS3Transfer
    {
    public:
        enum HttpVerb 
        {
            HttpGet,
            HttpPost,
            HttpPut,
            HttpDelete
        };

        virtual ~AuthorizedS3Transfer()
        {
        }

        virtual S3Operation *start(const Origin::Services::CloudSyncServiceSignedTransfer &signature) const = 0;
        virtual HttpVerb verb() const = 0;
        
        const QUrl &remoteUrl() const 
        {
            return m_remoteUrl;
        }

        virtual QByteArray contentType() const
        {
            return QByteArray();
        }

        virtual void prepare()
        {
            // Start any prefetching/compression/etc required
            // const?
        }

        // Used for progress calculations
        virtual qint64 expectedSize() const = 0;

    protected:
        AuthorizedS3Transfer(const QUrl &remoteUrl) :
            m_remoteUrl(remoteUrl)
        {
        }

    private:
        QUrl m_remoteUrl;
    };

    class ORIGIN_PLUGIN_API AuthorizedS3Upload : public AuthorizedS3Transfer
    {
    public:
        AuthorizedS3Upload(const QString &sourcePath, QFile *zipFile, const QUrl &remoteUrl, qint64 expectedSize, bool gzip = true) :
            AuthorizedS3Transfer(remoteUrl),
            m_sourcePath(sourcePath),
            m_zipFile(zipFile),
            m_expectedSize(expectedSize),
            m_prepared(false),
            m_gzip(gzip)
        {
        }    

        S3Operation *start(const Origin::Services::CloudSyncServiceSignedTransfer &signature) const
        {
            return new S3UploadOperation(*this, signature);
        }    

        const QString &sourcePath() const
        {
            return m_sourcePath;
        }

        QFile *zipFile() const
        {
            return m_zipFile;
        }
        
        HttpVerb verb() const
        {
            return HttpPut;
        }

        qint64 expectedSize() const
        {
            return m_expectedSize;
        }

        bool gzip() const
        {
            return m_gzip;
        }

        void prepare();

        /**
         * Gzip save file into the temp zip file.  Return whether successful.
         */
        bool performGzip() const;

    private:
        QString m_sourcePath;
        QFile *m_zipFile;
        qint64 m_expectedSize;

        bool m_prepared;
        QFuture<bool> m_zipSucceeded;
        bool m_gzip;
    };

    class ORIGIN_PLUGIN_API AuthorizedS3Download : public AuthorizedS3Transfer
    {
    public:
        AuthorizedS3Download(const QUrl &remoteUrl, const QString &destPath, qint64 expectedSize) :
            AuthorizedS3Transfer(remoteUrl),
            m_destPath(destPath),
            m_expectedSize(expectedSize)
        {
        }    

        S3Operation *start(const Origin::Services::CloudSyncServiceSignedTransfer &signature) const
        {
            return new S3DownloadOperation(*this, signature);
        }    

        const QString &destPath() const
        {
            return m_destPath;
        }
        
        HttpVerb verb() const
        {
            return HttpGet;
        }
        
        qint64 expectedSize() const
        {
            return m_expectedSize;
        }

    private:
        QString m_destPath;
        QUrl m_remoteUrl;
        qint64 m_expectedSize;
    };
    
    class ORIGIN_PLUGIN_API AuthorizedS3Delete : public AuthorizedS3Transfer
    {
    public:
        AuthorizedS3Delete(const QUrl &remoteUrl) :
            AuthorizedS3Transfer(remoteUrl)
        {
        }    

        S3Operation *start(const Origin::Services::CloudSyncServiceSignedTransfer &signature) const
        {
            return new S3DeleteOperation(*this, signature);
        }    
        
        HttpVerb verb() const
        {
            return HttpDelete;
        }

        qint64 expectedSize() const
        {
            // DELETE has no response body we care about
            return 0;
        }
    };

    class ORIGIN_PLUGIN_API AuthorizedS3Put : public AuthorizedS3Transfer
    {
    public:
        AuthorizedS3Put(const QByteArray &data, const QUrl &remoteUrl, const QByteArray &contentType = QByteArray(), bool gzip = true) :
            AuthorizedS3Transfer(remoteUrl),
            m_data(data),
            m_contentType(contentType),
            m_gzip(gzip)
        {
        }

        S3Operation *start(const Origin::Services::CloudSyncServiceSignedTransfer &signature) const
        {
            return new S3PutOperation(*this, signature);
        }    

        const QByteArray &data() const 
        {
            return m_data;
        }

        QByteArray contentType() const
        {
            return m_contentType;
        }
        
        HttpVerb verb() const
        {
            return HttpPut;
        }

        qint64 expectedSize() const
        {
            return m_data.size();
        }

        bool gzip() const
        {
            return m_gzip;
        }

    private:
        QUrl m_remoteUrl;
        QByteArray m_data;
        QByteArray m_contentType;
        bool m_gzip;
    };
}

}
}

#endif
