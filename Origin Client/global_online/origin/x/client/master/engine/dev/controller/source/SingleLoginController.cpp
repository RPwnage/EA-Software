/////////////////////////////////////////////////////////////////////////////
// SingleLoginController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "engine/login/SingleLoginController.h"
#include "services/rest/ChatServiceClient.h"
#include "services/rest/CloudSyncServiceClient.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

#include "chat/OriginConnection.h"

#include "engine/login/LoginController.h"
#include "engine/social/SocialController.h"


// legacy 
#include "TelemetryAPIDLL.h"

// qt
#include <QTimer>

using namespace Origin::Services;

namespace Origin
{
    namespace Engine
    {
        static SingleLoginController* sInstance = NULL;

        void SingleLoginController::init()
        {
            if (!sInstance)
            {
                sInstance = new SingleLoginController();
            }
        }

        void SingleLoginController::release()
        {
            delete sInstance;
            sInstance = NULL;
        }

        SingleLoginController* SingleLoginController::instance()
        {
            return sInstance;
        }

        void SingleLoginController::postInitialize()
        {
            // can't hook up to chat at engine::init() time
            // TBD: this needs to be fixed when OriginApplication startup is refactored
            // this function is called repeatedly, don't use ORIGIN_VERIFY_CONNECT
            UserRef user = LoginController::currentUser();

            if (user)
            {
                Chat::Connection *chatConn = user->socialControllerInstance()->chatConnection();

                QObject::connect(chatConn, SIGNAL(disconnected(Origin::Chat::Connection::DisconnectReason)),
                        this, SLOT(onChatDisconnect(Origin::Chat::Connection::DisconnectReason)), Qt::UniqueConnection);
            }
        }

        void SingleLoginController::checkUserOnline(Session::SessionRef session, const bool isSingleLoginEnabled)
        {
            mIsSingleLoginEnabled = isSingleLoginEnabled;

            if ( mUserOnlineResponse )
            {
                ORIGIN_VERIFY_DISCONNECT(mUserOnlineResponse, SIGNAL(finished()), this, SLOT(onCheckUserOnlineFinished()));
                mUserOnlineResponse->deleteLater();
            }

            // make the server call even if EACore.ini contains SingleLogin=false, EBIBUGS-21802
            mUserOnlineResponse = ChatServiceClient::isUserOnline(session);
            ORIGIN_VERIFY_CONNECT(mUserOnlineResponse, SIGNAL(finished()), this, SLOT(onCheckUserOnlineFinished()));
            
            // Start the time-out timer.
            mUserOnlineTimeout->start();
        }

        void SingleLoginController::checkUserInCloudSaves(Session::SessionRef session)
        {
            CloudSyncLockTypeResponse* resp = CloudSyncServiceClient::getLockType(session);
            ORIGIN_VERIFY_CONNECT(resp, SIGNAL(finished()), this, SLOT(onCheckUserInCloudSavesFinished()));
        }

        void SingleLoginController::onCheckUserInCloudSavesFinished()
        {
            CloudSyncLockTypeResponse* resp = static_cast<CloudSyncLockTypeResponse*>(sender());
            emit (checkUserInCloudSavesFinished(resp->isWritableState()));
            resp->deleteLater();
        }

        void SingleLoginController::onCheckUserOnlineFinished()
        {
            mUserOnlineTimeout->stop();
            
            // If mUserOnlineResponse is NULL, the user either logged out already or the 
            // timeout triggered and the response got cleaned up, so we can ignore this result
            if( mUserOnlineResponse != NULL) 
            {       
                UserOnlineResponse* resp = static_cast<UserOnlineResponse*>(sender());       
                ORIGIN_VERIFY_DISCONNECT(mUserOnlineResponse, SIGNAL(finished()), this, SLOT(onCheckUserOnlineFinished()));
                mUserOnlineResponse->deleteLater();
                mUserOnlineResponse = NULL;

                // If we lost our weak reference to the user's session, the user has logged out
                // already and there's no point in continuing here.
                if(!resp->userSession().isNull())
                {
                    bool online;
                    if (mIsSingleLoginEnabled)
                        online = resp->userOnline();
                    else
                        online = false; // single login check disabled in EACore.ini, ignore response

                    emit (checkUserOnlineFinished(online));
                }
            }
            else
            {
                ORIGIN_LOG_WARNING << "Received user online response after timing out or logging out!";
            }
        }
        
        void SingleLoginController::onCheckUserOnlineTimeout()
        {
            mUserOnlineTimeout->stop();
            if ( mUserOnlineResponse )
            {
                ORIGIN_VERIFY_DISCONNECT(mUserOnlineResponse, SIGNAL(finished()), this, SLOT(onCheckUserOnlineFinished()));
                mUserOnlineResponse->deleteLater();
                mUserOnlineResponse = NULL;
            }

            ORIGIN_LOG_ERROR << "checkUserOnline timed out!";

            // Can't connect to the chat servers. Continue logging in as if no user is online so that we aren't preventing
            // the user from accessing/downloading his games.  We do a separate "SocialLogIn" check after logging in that 
            // will take care of disabling our friends list.
            emit (checkUserOnlineFinished(false));
        }

		void SingleLoginController::onUserLoggedOut(Origin::Engine::UserRef user)
		{
			mUserOnlineTimeout->stop();

			if(mUserOnlineResponse)
			{
                ORIGIN_LOG_WARNING<< "logged out while waiting for check user online response";
                ORIGIN_VERIFY_DISCONNECT(mUserOnlineResponse, SIGNAL(finished()), this, SLOT(onCheckUserOnlineFinished()));
				mUserOnlineResponse->deleteLater();
				mUserOnlineResponse = NULL;
			}
		}

        SingleLoginController::SingleLoginController() :
			mUserOnlineResponse(NULL),
            mUserOnlineTimeout(new QTimer(this)),
            mTimeout(30000)
        {
            mUserOnlineTimeout->setInterval(mTimeout);
            ORIGIN_VERIFY_CONNECT(mUserOnlineTimeout, SIGNAL(timeout()), this, SLOT(onCheckUserOnlineTimeout()));
			ORIGIN_VERIFY_CONNECT(Engine::LoginController::instance(), SIGNAL(userLoggedOut(Origin::Engine::UserRef)), this, SLOT(onUserLoggedOut(Origin::Engine::UserRef)));
        }

        void SingleLoginController::onChatDisconnect(Origin::Chat::Connection::DisconnectReason reason)
        {
            if (reason == Chat::Connection::DisconnectStreamConflict)
            {
                emit firstUserConflict();
            }
        }
    }
}
