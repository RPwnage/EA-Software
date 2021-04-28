#ifndef _CLOUDSAVES_S3OPERATION
#define _CLOUDSAVES_S3OPERATION

#include <QObject>
#include <QString>
#include <QFile>
#include <QNetworkReply>
#include <QPointer>

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
    class AuthorizedS3Upload;
    class AuthorizedS3Download;
    class AuthorizedS3Delete;
    class AuthorizedS3Put;

    class ORIGIN_PLUGIN_API S3Operation : public QObject 
    {
        Q_OBJECT
    public:
        S3Operation() : m_reply(NULL), m_hasLocalFailure()
        {
        }

        virtual ~S3Operation()
        {
            delete m_reply;
        }

        /**
         * Indicates if the result of this operation can be ignored
         */
        virtual bool optional() const
        {
            return false;
        }
    
        bool hasLocalFailure() const { return m_hasLocalFailure; }

        QNetworkReply *reply() { return m_reply; }
    
    signals:
        void failed();
        void succeeded();
        void progress(qint64, qint64);

    protected slots:
        void replyFinished();
    
    protected:
        // Used to clean up resources after we've finished
        // Used purely to close files without having to use queued signals
        virtual void finalize()
        {
        }

        QPointer<QNetworkReply> m_reply;
        bool m_hasLocalFailure;
    };

    class ORIGIN_PLUGIN_API S3UploadOperation : public S3Operation
    {
        Q_OBJECT
    public:
        S3UploadOperation(const AuthorizedS3Upload &transfer, const Origin::Services::CloudSyncServiceSignedTransfer &signature);

        void finalize();

    protected:
        QFile m_localFile;
    };
    
    class ORIGIN_PLUGIN_API S3DownloadOperation : public S3Operation
    {
        Q_OBJECT
    public:
        S3DownloadOperation(const AuthorizedS3Download &transfer, const Origin::Services::CloudSyncServiceSignedTransfer &signature);
        
        void finalize();

    protected slots:
        void readyRead();

    protected:
        QFile m_localFile;
    };

    class ORIGIN_PLUGIN_API S3DeleteOperation : public S3Operation
    {
        Q_OBJECT
    public:
        S3DeleteOperation(const AuthorizedS3Delete &transfer, const Origin::Services::CloudSyncServiceSignedTransfer &signature);

        bool optional() const
        {
            // The server side will clean us up
            return true;
        }
    };
    
    class ORIGIN_PLUGIN_API S3PutOperation : public S3Operation
    {
        Q_OBJECT
    public:
        S3PutOperation(const AuthorizedS3Put &transfer, const Origin::Services::CloudSyncServiceSignedTransfer &signature);
    };
}
}
}

#endif
