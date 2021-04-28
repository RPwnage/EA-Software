///////////////////////////////////////////////////////////////////////////////
// Service::ConnectionStates.h
//
// enums used to describe type and value of connection states, both global and session
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef CONNECTION_STATES_H
#define CONNECTION_STATES_H

namespace Origin
{

namespace Services
{
    // 8.6 states that we may want to represent
    //    kOR_NoNetworkConnection,
    //    kOR_LoginServerFailure,
    //    kOR_RequiredUpdatePending,
    //    kOR_IntentionalByUser,
    //    kOR_SingleLoginConflict,
    //    kOR_MissingToken,
    //    kOR_WebServiceFailure,
    
/// \brief The base %Connection namespace contains a function that queries the current connection state and listens for the changes in the
/// global state and the session state and emits signals.
///
/// <span style="font-size:14pt;color:#404040;"><b>See the relevant wiki docs:</b></span> <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/Connection+States">Connection States</a>
namespace Connection
{
    ///
    ///bit flags to indicate both connection states -- global and user
    ///used to pass into LoginRegistrationService to indicate what connection has changed
    ///
    enum ConnectionStateField
    {
        // note that some fields need to be true for internet access to be 
        // possible, and others need to be false, the fields are named for 
        // readability rather than consistency.

        // There is a required update pending, and the client should 
        // not access the internet. Needs to be false for internet to 
        // be reachable.
        GLOB_ADM_IS_REQUIRED_UPDATE_PENDING,
        // Used to represent the computer being asleep.
        GLOB_OPR_IS_COMPUTER_AWAKE,
        // Used for the check which accessed an EA heartbeat host to verify that
        // the internet can in fact be accessed. Is true when the internet is 
        // reachable.
        GLOB_OPR_IS_INTERNET_REACHABLE,
        // Used for debugging to make the client pretend the network is unreachable, 
        // see the -Origin_Offline and -Origin_Online command line arguments and 
        // UserArea::toggleInternetConnection(). Needs to be true for internet to 
        // be reachable.
        GLOB_ADM_SIMULATE_IS_INTERNET_REACHABLE,
        // Used for when the user selects to be offline. Needs to be false for
        // the internet to be reachable.
        USR_ADM_IS_USER_MANUAL_OFFLINE,
        // Flag used to represent single login check has been completed
        USR_OPR_IS_SINGLE_LOGIN_CHECK_COMPLETE,
        // Used when the user has lost authentication, i.e. the auth token in the
        // client does not exist, or is not accepted by the servers. Needs to be
        // true for authenticated services to be reachable.
        USER_OPR_IS_USER_AUTHENTICATED
    } ;

    enum ConnectionStatus
    {
        CS_UNDEFINED = -1,
        CS_FALSE = 0,
        CS_TRUE
    };

}

}

}

#endif // CONNECTION_STATES_H
