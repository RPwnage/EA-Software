/**************************************************************************************************/
/*! 
    \file loginmanager/loginmanagerlistener.h
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef LOGINMANAGERLISTENER_H
#define LOGINMANAGERLISTENER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

// Include files
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blazeerrortype.h"
#include "BlazeSDK/component/authentication/tdf/authentication.h"

namespace Blaze
{
    typedef uint64_t XUID;

namespace LoginManager
{


/*! ***********************************************************************************************/
/*! \class LoginManagerListener
    
    \brief Interface for objects that receive LoginManager notifications.
***************************************************************************************************/
class BLAZESDK_API LoginManagerListener
{
public:

    /*! \brief The attempt to login to the Blaze servers failed.
               
        \param errorCode The error code specifying why the connection could not be made. The error
               code is one of the following:
        <table>        
        <tr>
        <td>AUTH_ERR_TOS_REQUIRED</td>
        <td>The user needs to accept the latest terms of service.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_INVALID_USER,</td>
        <td>The user does not exist or is invalid.</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_INVALID_NAMESPACE,</td>
        <td>The supplied namespace was invalid..</td>
        </tr>

        <tr>
        <td>AUTH_ERR_USER_INACTIVE</td>
        <td>User's status is inactive (disabled, banned, etc.)</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_INV_MASTER</td>
        <td>Invalid master account.</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_DEACTIVATED</td>
        <td>Account pending activation.</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_BANNED</td>
        <td>Account banned from online play.</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_DISABLED</td>
        <td>Account has been disabled.</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_INVALID_EMAIL</td>
        <td>Invalid email.</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_TOO_YOUNG,</td>
        <td>User is too young, cannot create underage account.</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_EXPIRED_TOKEN</td>
        <td>The supplied auth token was expired.</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_INVALID_TOKEN</td>
        <td>The supplied token was invalid.
        <td>For Nucleus 2.0 logins, the supplied AccessToken/AuthCode was invalid. Should launch WebKit with portalUrl to present user with correct Nucleus Portal flow to proceed. i.e. account creation</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_PERSONA_NOT_FOUND</td>
        <td>The requested persona name was not found.  Should launch WebKit with portalUrl to present user with correct Nucleus Portal flow to proceed. i.e. account creation</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_PERSONA_INACTIVE</td>
        <td>The requested persona is not active.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_PERSONA_BANNED</td>
        <td>The requested persona is banned.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_FIELD_MISSING</td>
        <td>The value is missing.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_CODE_ALREADY_USED</td>
        <td>Key code is alreay used.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_INVALID_CODE</td>
        <td>Invalid key.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_NO_SUCH_GROUP</td>
        <td>No such group.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_NO_SUCH_GROUP_NAME</td>
        <td>Group name not found.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_NO_ASSOCIATED_PRODUCT</td>
        <td>No Associated product for this key.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_CODE_ALREADY_DISABLED</td>
        <td>Code Already Disabled.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_GROUP_NAME_DOES_NOT_MATCH</td>
        <td>Group name not matching asscociated code.</td>
        </tr>
        
        <tr>
        <td>AUTH_ERR_INV_MASTER</td>
        <td>The requested persona is banned.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_NAME_MISMATCH</td>
        <td>User's display name doesn't match gamer tag.</td>
        </tr>

        <tr>
        <td>UTIL_TICKER_NO_SERVERS_AVAILABLE</td>
        <td>There are no ticker servers available.</td>
        </tr>

        <tr>
        <td>UTIL_TELEMETRY_NO_SERVERS_AVAILABLE</td>
        <td>There are no telemetry servers available.</td>
        </tr>

        <tr>
        <td>UTIL_TICKER_KEY_TOO_LONG</td>
        <td>Ticker key is too long to create.</td>
        </tr>

        <tr>
        <td>UTIL_TELEMETRY_OUT_OF_MEMORY</td>
        <td>The requested persona is banned.</td>
        </tr>

        <tr>
        <td>UTIL_TELEMETRY_KEY_TOO_LONG</td>
        <td>Telemetry key is longer then it should be.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_NO_SUCH_ENTITLEMENT</td>
        <td>Indicates that the user does not have a valid ONLINE_ACCESS entitlement. This should only occur during testing prior to integration with OriginSDK.</td>
        </tr>

        <tr>
        <td>AUTH_ERR_TRIAL_PERIOD_CLOSED</td>
        <td>Indicates that an attempt to grant a trial entitlement failed due to the entitlement either not started or already over. For managed lifecycle entitlements, this can mean that an entitlement wasn't granted with the correct status (PENDING for before the trial starts, ACTIVE during, and DISABLED when over).</td>
        </tr>

        <tr>
        <td>AUTH_ERR_EXCEEDS_PSU_LIMIT</td>
        <td>PSU cutoff for entitlement type is less than the current connected user count</td>
        </tr>

        <tr>
        <td>AUTH_ERR_BANNED</td>
        <td>The user is banned from logging in for online play</td>
        </tr>

        <tr>
        <td>AUTH_ERR_INVALID_SANDBOX_ID</td>
        <td>The Sandbox associated with this console (Xbox) does not match the primary sandbox, or any alternative sandboxes, specified on the Blaze Server.</td>
        </tr>

        <tr>
        <td>SDK_ERR_NUCLEUS_RESPONSE</td>
        <td>A generic error occurred while communicating with Nucleus while attempting to authenticate the user</td>
        </tr>

        <tr>
        <td>SDK_ERR_INVALID_XBL_TICKET</td>
        <td>A valid XBL (STS) token could not be retrieved from Xbox Live</td>
        </tr>

        <tr>
        <td>SDK_ERR_INVALID_NP_TICKET</td>
        <td>A valid NP ticket could not be retrieved from Sony</td>
        </tr>

        </table>
    ***********************************************************************************************/
    virtual void onLoginFailure(BlazeError errorCode, const char8_t* portalUrl) = 0;

    /*! *******************************************************************************************/
    /*! \name General error callback
     */
    /*! *******************************************************************************************/
    /*! \brief A general error has occurred, such as an attempt to use the LoginManager before the
               BlazeHub is in the CONNECTED state.
        
        \param errorCode The error code returned.
    ***********************************************************************************************/
    virtual void onSdkError(BlazeError errorCode) = 0;
    
    /*! *******************************************************************************************/
    /*! \name Destructor
    ***********************************************************************************************/
    virtual ~LoginManagerListener() {}
};

} // LoginManager
} // Blaze

#endif // LOGINMANAGERLISTENER_H
