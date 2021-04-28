#include "services/rest/HttpServiceResponse.h"
#include "services/network/NetworkAccessManager.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/session/SessionService.h"

#if defined(_DEBUG)
#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#endif

namespace Origin
{
    namespace Services
    {
#if defined(_DEBUG)
        QMutex HttpServiceResponse::mInstanceDebugMapMutex;
        QSet<HttpServiceResponse*> HttpServiceResponse::mInstanceDebugMap;
        void HttpServiceResponse::checkForMemoryLeak()
        {
            QMutexLocker locker(&mInstanceDebugMapMutex);
            int count = mInstanceDebugMap.size();

            // If this debug fires, any responses found in the map are leaked HttpResponses. 
            if (count > 0) 
            {
                // Only trigger an annoying assert if we are leaking 5 or more.  Let's find and fix our leaks!
                ORIGIN_ASSERT_MESSAGE((count < 5), "We are leaking several http response objects.  Please help fix this memory leak.");
                ORIGIN_LOG_ERROR << "We are leaking HttpServiceResponse instances [" << count << " total]";
            }

            for ( QSet<HttpServiceResponse*>::const_iterator i = mInstanceDebugMap.begin(); i != mInstanceDebugMap.end(); ++i )
            {
                HttpServiceResponse* curr = *i;
                QNetworkReply* reply = curr->reply();
                Q_UNUSED(reply);
            }
        }
        void HttpServiceResponse::insertMe()
        {
            QMutexLocker locker(&mInstanceDebugMapMutex);
            mInstanceDebugMap.insert(this);
        }
        void HttpServiceResponse::removeMe()
        {
            QMutexLocker locker(&mInstanceDebugMapMutex);
            mInstanceDebugMap.remove(this);
        }
#endif
        HttpServiceResponse::HttpServiceResponse(QNetworkReply *reply)
            : m_reply(reply)
            , m_deleteOnFinish(false)
            , m_clientData()
        {
            connect(reply, SIGNAL(finished()), this, SLOT(onReplyFinished()));

			//listen for any connection changes, both from  the user and global
			connect(
				Origin::Services::Connection::ConnectionStatesService::instance(), 
				SIGNAL(userConnectionStateChanged(bool, Origin::Services::Session::SessionRef)),
                this,
				SLOT(onConnectionStateChanged(bool, Origin::Services::Session::SessionRef))
				);

			connect(
				Origin::Services::Connection::ConnectionStatesService::instance(), 
				SIGNAL(globalConnectionStateChanged(bool)),
                this,
				SLOT(onGlobalConnectionStateChanged(bool))
				);
        }


		void HttpServiceResponse::onGlobalConnectionStateChanged(bool isOnline)
        {
            //pass in an empty session
            onConnectionStateChanged (isOnline, Origin::Services::Session::SessionRef());
        }

		void HttpServiceResponse::onConnectionStateChanged(bool online, Origin::Services::Session::SessionRef session)
		{
            //we can't just rely on bool passed from the signal because for the session case, auth or single login check could be off but for our purposes
            //we only need to know about manual offline, so just check manually

            //first check if global state is online
            bool isOnline = Connection::ConnectionStatesService::isGlobalStateOnline();
            //now check the session, but we only need to know about manual offline, authentication could be off and single login still not done but
            //we don't want to abort in case this is requesting a refresh of the token
            if (isOnline)
            {
                //otherwise just rely on global connection state?
                if (!session.isNull())  
                {
                    isOnline = !(static_cast<bool>(Connection::ConnectionStatesService::isUserManualOfflineFlagSet (session)));
                }
            }

			//if we are not online abort the request
			if( m_reply && !isOnline )
			{
				m_reply->abort();
			}
		}

        HttpServiceResponse::~HttpServiceResponse()
        {
            removeMe();
            delete m_reply;
        }

        void HttpServiceResponse::onReplyFinished()
        {
            // Track if we're not cleaned up
            insertMe();

            processReply();
            emit(finished());

            if (deleteOnFinish())
            {
                // We can't delete inline in a slot without crashing
                deleteLater();
            }
        }
    }
}
