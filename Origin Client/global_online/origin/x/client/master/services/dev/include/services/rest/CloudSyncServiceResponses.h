#ifndef _CLOUDSYNCSERVICERESPONSES_H
#define _CLOUDSYNCSERVICERESPONSES_H

#include <QByteArray>
#include <QHash>

#include "OriginAuthServiceResponse.h"
#include "HttpStatusCodes.h"
#include "PropertiesMap.h"
#include "OriginServiceValues.h"

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        struct CloudSyncServiceLock
        {
            CloudSyncServiceLock() 
                : lockName(QByteArray()), timeToLiveSeconds(-1)
            {
            }

            CloudSyncServiceLock(const QByteArray &newLockName, int newTtl) 
                : lockName(newLockName), timeToLiveSeconds(newTtl)
            {
            }

            bool isValid() const
            {
                return timeToLiveSeconds != -1;
            }

            QByteArray lockName;
            int timeToLiveSeconds;
        };

        class ORIGIN_PLUGIN_API CloudSyncLockingResponse
        {
        public:
            explicit CloudSyncLockingResponse(const QByteArray &lockName = QByteArray()) : 
            m_lock(lockName, -1)
            {
            }

            const CloudSyncServiceLock &lock() const { return m_lock; }

        protected:
            virtual bool processLockHeaders(const QNetworkReply *reply);

            CloudSyncServiceLock m_lock;

            virtual ~CloudSyncLockingResponse() {}
        };

        class ORIGIN_PLUGIN_API CloudSyncLockExtensionResponse : public OriginAuthServiceResponse, public CloudSyncLockingResponse
        {
            Q_OBJECT
            
        public:
        
            CloudSyncLockExtensionResponse(AuthNetworkRequest reply, const QByteArray &lockName)
                : OriginAuthServiceResponse(reply)
                , CloudSyncLockingResponse(lockName)
            {
            }

signals:
            void success();
            void error();

        protected:
            virtual void processReply();
        };

        class ORIGIN_PLUGIN_API CloudSyncLockPromotionResponse : public OriginAuthServiceResponse, public CloudSyncLockingResponse
        {
            Q_OBJECT
        public:
            CloudSyncLockPromotionResponse(AuthNetworkRequest reply, const QByteArray &lockName)
                : OriginAuthServiceResponse(reply)
                , CloudSyncLockingResponse(lockName)
            {
            }

signals:
            void success();
            void error();

        protected:
            virtual void processReply();
        };

        class ORIGIN_PLUGIN_API CloudSyncManifestDiscoveryResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            enum ManifestDiscoveryError
            {
                NoError,
                UnknownError,
                ManifestLockedError
            };

            explicit CloudSyncManifestDiscoveryResponse(AuthNetworkRequest reply)
                : OriginAuthServiceResponse(reply)
                , mError(NoError)
            {
            }

            const QUrl &manifestUrl() const { return mManifestUrl; }
            ManifestDiscoveryError error() const { return mError; }

signals:
            void error(CloudSyncManifestDiscoveryResponse::ManifestDiscoveryError);

        protected:
            virtual void processReply();
            virtual bool parseSuccessBody(QIODevice *);

            QUrl mManifestUrl;		// <manifest> provides the presigned url to allow the client to GET the manifest
            ManifestDiscoveryError mError;
        };

        class ORIGIN_PLUGIN_API CloudSyncUnlockResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            explicit CloudSyncUnlockResponse(AuthNetworkRequest reply)
                : OriginAuthServiceResponse(reply)
            {
            }

signals:
            void success();
            void error();

        protected:
            virtual void processReply();
        };

        class ORIGIN_PLUGIN_API CloudSyncSyncInitiationResponse : public CloudSyncManifestDiscoveryResponse, public CloudSyncLockingResponse
        {
        public:
            explicit CloudSyncSyncInitiationResponse(AuthNetworkRequest reply)
                : CloudSyncManifestDiscoveryResponse(reply)
            {
            }

            const QUrl &signatureRoot() const { return mSignatureRoot; }

        protected:
            virtual void processReply();
            virtual bool parseSuccessBody(QIODevice *);

            QUrl mSignatureRoot;
        };

        class ORIGIN_PLUGIN_API CloudSyncLockTypeResponse : public OriginAuthServiceResponse
        {
            Q_OBJECT
        public:
            CloudSyncLockTypeResponse(AuthNetworkRequest reply)
                : OriginAuthServiceResponse(reply)
            {
            }

            bool isWritableState() const { return m_isWriting; }
        protected:

            bool m_isWriting;
            virtual void processReply();
        };

        //////////////////////////////////////////////////////////////////////////
        ///
        /// Useful typedef for the hash containing key,value map
        ///
        typedef QHash<QByteArray, QByteArray> HeadersMap;

        ///
        /// Class that defines the elements of the initial auth request
        ///
        class ORIGIN_PLUGIN_API CloudSyncServiceAuthorizationRequest
        {
        public:
            ///
            /// Accesors - getters
            ///
            HttpVerb verb() const { return mHttpVerb;}
            QString resource() const { return mResource;}
            QByteArray md5() const { return mMd5; }
            QByteArray contentType() const { return mContentType; }
            QVariant userAttribute() const { return mUserAttribute; }

            ///
            /// setters
            ///
            void setVerb(const HttpVerb& httpVerb)
            {
                mHttpVerb = httpVerb;
            }

            void setResource(const QString &resource)
            {
                mResource = resource;
            }

            void setMd5(const QByteArray& md5)
            {
                mMd5 = md5;
            }

            void setContentType(const QByteArray &contentType)
            {
                mContentType = contentType;
            }

            void setUserAttribute(QVariant value)
            {
                mUserAttribute = value;
            }

        private:
            HttpVerb mHttpVerb;
            QString mResource;
            QByteArray mMd5;
            QVariant mUserAttribute;
            QByteArray mContentType;
        };

        class ORIGIN_PLUGIN_API CloudSyncServiceSignedTransfer
        {
        public:
            ///
            /// Accessors
            ///
            QUrl url() const { return mUrl; }
            const HeadersMap& headers() const { return mHeaders; }
            const CloudSyncServiceAuthorizationRequest& authorizationRequest() const { return mAuthorizationRequest; }

            //////////////////////////////////////////////////////////////////////////
            /// Overloaded mutators
            ///

            ///
            /// \brief Assigns a header map.
            /// \param hm The right side header map.
            ///
            void setHeaders(const HeadersMap& hm) 
            {
                mHeaders = hm;
            }

            /// 
            /// \brief Assigns the url val.
            /// \param val The right side QByteArray.
            ///
            void setUrl(const QUrl& val) 
            { 
                mUrl = val; 
            }

            void setAuthorizationRequest(const CloudSyncServiceAuthorizationRequest &req)
            {
                mAuthorizationRequest = req;
            }

        protected:
            ///
            /// Contains the url part of the authorization 
            ///
            QUrl mUrl;
            ///
            /// Contain the header key, value pair contents
            ///
            HeadersMap mHeaders;

            CloudSyncServiceAuthorizationRequest mAuthorizationRequest;
        };

        ///
        ///
        /// This class has a container containing the transfers indexed by requestId/manifestId
        ///
        class ORIGIN_PLUGIN_API CloudSyncServiceAuthenticationResponse : public OriginAuthServiceResponse, public CloudSyncLockingResponse
        {
            Q_OBJECT
        public:

            enum SyncServiceAuthenticationError
            {
                UnknownError,
                MissingAuthtokenOrLock,
                InvalidLock,
                ImproperResponse
            };

            explicit CloudSyncServiceAuthenticationResponse(AuthNetworkRequest reply, const QList<CloudSyncServiceAuthorizationRequest>& arl, const CloudSyncServiceLock & lock)
                : OriginAuthServiceResponse(reply)
                , CloudSyncLockingResponse(lock.lockName)
                , mAuthReqList (arl)
            {
            }

            /// 
            /// Returns container containing the transfers indexed by requestId/manifestId
            /// 
            const QList<CloudSyncServiceSignedTransfer>& signedTransfers() const {return mSignedTransfers;};


signals:
            void success();
            void error(SyncServiceAuthenticationError);

        protected:
            /// 
            /// Processes reply
            ///
            virtual void processReply();

            /// 
            /// Parses returned data: authorizations from the server
            ///
            bool parseSuccessBody(QIODevice *);

        private:
            ///
            /// container containing the transfers indexed by requestId/manifestId
            ///
            QList<CloudSyncServiceSignedTransfer> mSignedTransfers;

            ///
            /// Original requests this is responding to
            ///
            QList<CloudSyncServiceAuthorizationRequest> mAuthReqList;
        };

        static const char* const SyncLockTtlHeader		= "X-Origin-Sync-Lock-TTL";
        static const char* const SyncLockNameHeader		= "X-Origin-Sync-Lock";
        static const char* const SyncAuthtoken			= "X-Origin-AuthToken";
        static const char* const SyncLockTypeHeader		= "X-Origin-Sync-Lock-Type";

    }
}

#endif