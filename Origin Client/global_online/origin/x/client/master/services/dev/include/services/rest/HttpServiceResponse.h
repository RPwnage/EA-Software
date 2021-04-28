#ifndef HTTPSERVICERESPONSE_H
#define HTTPSERVICERESPONSE_H


#include <QObject>
#include <QNetworkReply>

#if defined(_DEBUG)
#include <QSet>
#include <QMutex>
#include <QMutexLocker>
#endif

#include "services/session/SessionService.h" //needed for SessionRef declaration
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        ///
        /// Base class for HTTP service responses
        ///
        /// By convention subclasses should have success() and failure() signals
        /// in addition to any accessors for information the response may contain
        ///
        class ORIGIN_PLUGIN_API HttpServiceResponse : public QObject
        {
            Q_OBJECT
        public:
            ///
            /// Returns true if the request is still running
            ///
            virtual bool isRunning() const { return m_reply->isRunning(); }

            ///
            /// Returns true if the request is finished
            ///
            virtual bool isFinished() const { return m_reply->isFinished(); }

            ///
            /// Returns the underlying network reply 
            ///
            QNetworkReply *reply() const { return m_reply; }

            ///
            /// Sets if we should delete ourselves after we finish
            ///
            /// Defaults to false
            ///
            /// \param deleteOnFinish True if we should delete ourselves once we've finished
            ///
            void setDeleteOnFinish(bool deleteOnFinish) { m_deleteOnFinish = deleteOnFinish; }

            ///
            /// Returns if we will delete ourselves after we finish
            ///
            bool deleteOnFinish() { return m_deleteOnFinish; }

            QVariant const& clientData() { return m_clientData; }
            void setClientData(QVariant const& data) { m_clientData = data; }

#if defined(_DEBUG)
            static QMutex mInstanceDebugMapMutex;
            static QSet<HttpServiceResponse*> mInstanceDebugMap;
            static void checkForMemoryLeak();
            void insertMe();
            void removeMe();
#else
            static void checkForMemoryLeak(){}
            void insertMe(){}
            void removeMe(){}
#endif
        public slots:

            ///
            /// Aborts the current request
            ///
            virtual void abort() { m_reply->abort(); }

        signals:

            ///
            /// Emitted when the request has finished processing
            ///
            void finished();

        protected slots:

            ///
            /// Our network reply has finished
            ///
            virtual void onReplyFinished();


            ///
            /// called when global connection state changes
            ///
			void onGlobalConnectionStateChanged(bool isOnline);

			///
			/// Called on connection state changed
			///
			void onConnectionStateChanged(bool online, Origin::Services::Session::SessionRef session);


        protected:

            ///
            /// Builds a concrete HTTP service response
            ///
            /// Intended to be called by concrete subclass
            ///
            /// \param reply  Backing QNetworkReply. HttpServiceResponse takes ownership of this instance
            ///
            HttpServiceResponse(QNetworkReply *reply);
            virtual ~HttpServiceResponse();

            // Subclasses override to process a completed network reply
            virtual void processReply() = 0;

        private:
            QNetworkReply *m_reply;
            bool m_deleteOnFinish;
            QVariant m_clientData;
        };
    }
}

#endif