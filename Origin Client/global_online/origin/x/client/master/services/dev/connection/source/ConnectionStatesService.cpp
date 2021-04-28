///////////////////////////////////////////////////////////////////////////////
// \brief  Service::ConnectionStatesService.cpp
//
// central location used to query the current connection state, both global and session
// also emits signals when connection state changes
// does not contain any functions to set/change the connection state; those should be done directly 
// via those owning the state variables (GlobalConnectionStateMonitor, SessionService)
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////////

#include "services/connection/ConnectionStatesService.h"
#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "TelemetryAPIDLL.h"
#include <QTimer.h>

namespace Origin
{

namespace Services
{

namespace Connection
{

QString const ConnectionStatesService::SERVICE_DATA_KEY = "ConnectionStatesService";

QPointer<ConnectionStatesService> sInstance;

void ConnectionStatesService::init()
{
    if (sInstance.isNull())
        sInstance = new ConnectionStatesService;
}

void ConnectionStatesService::release()
{
    if (sInstance.isNull())
        return;

    delete sInstance.data();
    sInstance = NULL;
}

ConnectionStatesService* ConnectionStatesService::instance()
{
    ORIGIN_ASSERT(sInstance.data());
    return sInstance.data();
}

ConnectionStatesService::ConnectionStatesService() 
    : isRecoveryInProgress(false),
    RecoveryIntervalMilliseconds(5000U)
{
    //listen for global connection state changes
    if (Network::GlobalConnectionStateMonitor::instance())
    {
        ORIGIN_LOG_EVENT << "connecting global connectionStateChanged";
        ORIGIN_VERIFY_CONNECT (Network::GlobalConnectionStateMonitor::instance(), SIGNAL(connectionStateChanged (Origin::Services::Connection::ConnectionStateField)), 
        this, SLOT(onGlobalConnectionStateChanged (Origin::Services::Connection::ConnectionStateField)));
    }

    if (Services::Session::SessionService::instance())
    {

        //hook up to listen for authentication state changes; when state changes occur (e.g. manual offline), LoginRegistrationSession will
        //set the authentication state and will emit authenticationStateChanged
        ORIGIN_VERIFY_CONNECT (Origin::Services::Session::SessionService::instance(), 
                       SIGNAL (authenticationStateChanged(Origin::Services::Session::AuthenticationState, Origin::Services::Session::SessionRef)),
                       this, 
                       SLOT (onAuthenticationStateChanged (Origin::Services::Session::AuthenticationState, Origin::Services::Session::SessionRef)));

        ORIGIN_VERIFY_CONNECT (Origin::Services::Session::SessionService::instance(), 
                       SIGNAL (userOnlineAllChecksCompleted(bool, Origin::Services::Session::SessionRef)),
                       this, 
                       SLOT (onUserOnlineAllChecksCompleted (bool, Origin::Services::Session::SessionRef)));

        //listen for when session ends so we can send signal that user is offline
        ORIGIN_VERIFY_CONNECT(
            Session::SessionService::instance(), SIGNAL(endSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef)),
            this, SLOT(onEndSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef)));
    }
}


//SLOTS
void ConnectionStatesService::onBeginSessionComplete(Origin::Services::Session::SessionError error, Origin::Services::Session::SessionRef session)
{
}

void ConnectionStatesService::onEndSessionComplete(Origin::Services::Session::SessionError, Origin::Services::Session::SessionRef session)
{
    isRecoveryInProgress = false;
    //user has logged out so essentially, user is "offline"
    emit (userConnectionStateChanged (false, session));
}

/// \brief if newAuthState =
/// SESSIONAUTHENTICATION_NONE:         will emit authenticationRequired (to initiate a new login flow requesting credentials)
/// SESSIONAUTHENTICATION_ENCRYPTED:    will emit userConnectionChanged (false), nowOfflineAuth()
/// SESSIONAUTHENTICATION_AUTH:         will emit reAuthenticated, which will initiate single login check (if singlelogincheckcompleted is false)
///
void ConnectionStatesService::onAuthenticationStateChanged (Origin::Services::Session::AuthenticationState newAuthState, Origin::Services::Session::SessionRef session)
{
    if (!session.isNull())
    {
        ORIGIN_LOG_EVENT << "onAuthenticationStateChanged: " << (int)newAuthState;
        switch (newAuthState)
        {
            case Services::Session::SESSIONAUTHENTICATION_NONE: //failed with existing tokens, initiate re authenticaton login flow
                emit authenticationRequired ();
                break;

            case Services::Session::SESSIONAUTHENTICATION_AUTH: //authentication success, initiate single login check
                emit reAuthenticated ();
                if(connectionField(USR_ADM_IS_USER_MANUAL_OFFLINE, session) == false)
                {
                    // start recovery
                    isRecoveryInProgress = true;
                    // Start the recovery timer
                    QTimer::singleShot(RecoveryIntervalMilliseconds, this, SLOT(onRecoveryPeriodEnded()));
                    // print the message that we are trying to recover
                    emit (nowRecovering());
                }
                break;

            case Services::Session::SESSIONAUTHENTICATION_ENCRYPTED:    //has a token but hasn't been authenticated, so place user offline
            default:
                //MY: TODO: checking against previous state causes one issue where the store page clears to white
                //it's somehow related to calling show() on another dialog (it used to be login window, but now it seems to be something else), 
                //not really sure why but for now, choose the lesser of the two evils
                //and allow the signal to be emitted even if we're already offline.  emitting the signal will force the redraw of the error page on the store, instead of
                //leaving it white with no error message

                //The downside of this is that social calls logout multiple times but for now, that doesn't seem to cause any issues
                //NEED TO INVESTIGATE but turn it off for now
//                if (connectionStateActuallyChanged (session, false))
                {
                    Origin::Services::Session::SessionService::setPreviouslyOnline (session, false);
                        emit (userConnectionStateChanged (false, session));
                        emit (nowOfflineUser());
                }
                break;
        }
    }
}

void ConnectionStatesService::onGlobalConnectionStateChanged (Origin::Services::Connection::ConnectionStateField field)
{
    //for our purposes we don't really care which field changed, we just want to know if global state is online or not
    bool isOnline = Network::GlobalConnectionStateMonitor::instance()->isOnline();

    ORIGIN_LOG_EVENT << "onGlobalConnectionStateChanged: " << isOnline;

    bool changed = true;

    if (!isOnline)
    {
        //make sure we don't send multiple offline connection changes
        bool isPendingRequiredUpdate = isRequiredUpdatePending ();
        bool isComputerAsleep = !(connectionField (GLOB_OPR_IS_COMPUTER_AWAKE) == CS_TRUE);
        bool isInternetDown = !(connectionField (GLOB_OPR_IS_INTERNET_REACHABLE) == CS_TRUE);
    
        switch (field)
        {
            case GLOB_ADM_IS_REQUIRED_UPDATE_PENDING:
                if (isComputerAsleep || isInternetDown) //already offline
                {
                    changed = false;
                }
                break;

            case GLOB_OPR_IS_COMPUTER_AWAKE:
                if (isPendingRequiredUpdate || isInternetDown) //already offline
                {
                    changed = false;
                }
                break;

            case GLOB_OPR_IS_INTERNET_REACHABLE:
                if (isPendingRequiredUpdate || isComputerAsleep) //already offline
                {
                    changed = false;
                }
                break;
                
            default:
                break;
        }
    }
    //for online, there really isn't a case where multiple things could make it be "online"" so we don't really need to check individual cases as we did for offline


    if (changed)
    {
        // When PC goes out of sleep mode and the client resumes running
        // at one point the local isOnline flag is becoming true and the
        // globalConnectionStateChnged is sent. Although the argument sent is true,
        // this signal triggers the function PageErrorHandler::updateConnectionState()
        // which calls ConnectionStatesService::isUserOnline(session)
        // which returns that the user is offline and as a result 
        // the offline error message is presented to the user.
        if(!isRecoveryInProgress)
            emit globalConnectionStateChanged(isOnline);
        
        if (!isOnline)
        {
            emit nowOfflineGlobal();
        }
    }

    //unlike going offline where multiple things can set offline to be true,
    //going online means everything has to be on so we shouldn't need to check for actual state change
    if (isOnline)
    {
        emit nowOnlineGlobal();
    }
}

void ConnectionStatesService::onUserOnlineAllChecksCompleted (bool isOnline, Origin::Services::Session::SessionRef session)
{
    if (!session.isNull())
    {
        ORIGIN_LOG_EVENT << "onUserOnlineAllChecksCompleted: " << isOnline;
        //check and see if there is an actual state change
        if (connectionStateActuallyChanged (session, isOnline))
        {
            Origin::Services::Session::SessionService::setPreviouslyOnline (session, isOnline);
            emit (userConnectionStateChanged (isOnline, session));
            if (isOnline)
                emit (nowOnlineUser());
            else
                emit (nowOfflineUser());
        }
    }
    else
    {
        ORIGIN_LOG_EVENT << "onUserOnlineAllChecksCompleted: session is NULL";
    }
}

void ConnectionStatesService::onRecoveryPeriodEnded()
{
    isRecoveryInProgress = false;
}


// ***************** QUERY FUNCTIONS

///
///  \brief returns false if !global online
/// returns true if global online && manualoffline=true
///  also returns true if global online && manualoffline = false (already initiated the go back online) + single login check is false because it means we're probably in the middle of reauthentication
///  before going back online
bool ConnectionStatesService::canGoOnline (Session::SessionRef session)
{
    if (!isGlobalStateOnline())
        return false;

    if (session.isNull())
        return false;

    if (connectionField (USR_ADM_IS_USER_MANUAL_OFFLINE, session) == CS_TRUE)
        return true;

    //we could be here where we're in the middle of trying to go back online so manualoffline is already set to false, but we're still in the reauthentication or single login check
    //in that case, return canGoOnline=true so as to not create confusion as to when you really can't go back online (and things base error messages off this query function, e.g. friends)
    if (connectionField (USR_OPR_IS_SINGLE_LOGIN_CHECK_COMPLETE, session) == CS_FALSE)
        return true;

    return false;
}

///
/// \brief Return true if we global state + session state are both online
///
bool ConnectionStatesService::isUserOnline(Session::SessionRef session)
{
    return (Network::GlobalConnectionStateMonitor::instance()->isOnline() &&
            !session.isNull() &&
            Session::SessionService::isOnline (session));
}

///
/// \brief Checks only against the GlobalState and returns true if online
///
bool ConnectionStatesService::isGlobalStateOnline()
{
    return Network::GlobalConnectionStateMonitor::instance()->isOnline();
}

Connection::ConnectionStatus ConnectionStatesService::connectionField(ConnectionStateField field, Session::SessionRef session)
{
    switch (field)
    {
        //global connection states
        case GLOB_ADM_IS_REQUIRED_UPDATE_PENDING:
        case GLOB_OPR_IS_COMPUTER_AWAKE:
        case GLOB_OPR_IS_INTERNET_REACHABLE:
        case GLOB_ADM_SIMULATE_IS_INTERNET_REACHABLE:
            return (Network::GlobalConnectionStateMonitor::connectionField (field));
            break;

        case USR_ADM_IS_USER_MANUAL_OFFLINE:
        case USR_OPR_IS_SINGLE_LOGIN_CHECK_COMPLETE:
            if (!session.isNull())  //no session
            {
                return Session::SessionService::connectionField (session, field);
            }
            else
            {
                return Connection::CS_UNDEFINED;
            }
            
        default:
            break;
    }
    return Connection::CS_UNDEFINED;
}

///
/// \brief checks and compares against previous online state to see if newState is actually different from the old state
/// 
bool ConnectionStatesService::connectionStateActuallyChanged (Origin::Services::Session::SessionRef session, bool newState)
{
    int prevOnline = Origin::Services::Session::SessionService::previouslyOnline (session);
    if (prevOnline == -1)   //uninitialized state
        return true;
    else
        return (prevOnline !=  static_cast<int>(newState));
}



}

}

}
