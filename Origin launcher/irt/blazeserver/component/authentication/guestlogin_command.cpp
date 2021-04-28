/*************************************************************************************************/
/*!
    \file   guestlogin_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GuestLoginCommand

    Allows an MLU guest user to login and associate with the current parent session.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "authentication/authenticationimpl.h"
#include "authentication/rpc/authenticationslave/guestlogin_stub.h"

#include "framework/tdf/userevents_server.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"

namespace Blaze
{
namespace Authentication
{

class GuestLoginCommand : public GuestLoginCommandStub
{
public:
    GuestLoginCommand(Message* message, AuthenticationSlaveImpl* componentImpl)
        : GuestLoginCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~GuestLoginCommand() override {};

private:
    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;
    // States
    BlazeRpcError doLogin(UserSessionMasterPtr &parentSession, UserSessionMasterPtr &guestSession)
    {
        if (getPeerInfo() == nullptr || !getPeerInfo()->isClient())
        {
            WARN_LOG("[GuestLoginCommand].execute(): Guest login attempted on a non-persistent connection.  This is not supported.");
            return AUTH_ERR_NO_PARENT_SESSION;
        }

        UserSessionIdVectorSet possibleParentUserSessionIds = getPeerInfo()->getUserSessionIds();
        for (UserSessionIdVectorSet::iterator it = possibleParentUserSessionIds.begin(), end = possibleParentUserSessionIds.end(); it != end; ++it)
        {
            parentSession = gUserSessionMaster->getLocalSession(*it);
            if ((parentSession != nullptr) && parentSession->canBeParentToGuestSession())
            {
                break;
            }

            parentSession = nullptr;
        }

        if (parentSession == nullptr)
        {
            WARN_LOG("[GuestLoginCommand].execute(): Unable to find parent session for connection.");
            return AUTH_ERR_NO_PARENT_SESSION;
        }
        BlazeRpcError err = gUserSessionMaster->createGuestUserSession(*parentSession, guestSession, *getPeerInfo(), getConnectionUserIndex());

        return err;
    }

    GuestLoginCommandStub::Errors execute() override
    {
        UserSessionMasterPtr guestSession;
        UserSessionMasterPtr parentSession;
        BlazeRpcError err = doLogin(parentSession, guestSession);

        const char8_t* serviceName = getPeerInfo()->getServiceName();
        const char8_t* productName = parentSession != nullptr ? parentSession->getProductName() : gController->getServiceProductName(serviceName);

        if (err != Blaze::ERR_OK)
        {
            mComponent->getMetricsInfoObj()->mLoginFailures.increment(1, productName, getPeerInfo()->getClientType());
            char8_t addrStr[Login::MAX_IPADDRESS_LEN] = {0};
            getPeerInfo()->getRealPeerAddress().toString(addrStr, sizeof(addrStr));

            bool sendPINErrorEvent = (err == AUTH_ERR_NO_PARENT_SESSION);

            GeoLocationData geoIpData;
            if (parentSession != nullptr)
            {
                geoIpData.setISP(parentSession->getExtendedData().getISP());
                geoIpData.setTimeZone(parentSession->getExtendedData().getTimeZone());
                geoIpData.setCountry(parentSession->getExtendedData().getCountry());
                gUserSessionManager->getGeoIpData(getPeerInfo()->getRealPeerAddress(), geoIpData);
    
                gUserSessionMaster->generateLoginEvent(err, getUserSessionId(), USER_SESSION_GUEST,
                    parentSession->getUserInfo().getPlatformInfo(), parentSession->getUniqueDeviceId(), parentSession->getDeviceLocality(), parentSession->getBlazeId(), parentSession->getUserInfo().getPersonaName(),
                    getPeerInfo()->getLocale(), getPeerInfo()->getCountry(), getPeerInfo()->getClientInfo()->getBlazeSDKVersion(), getPeerInfo()->getClientType(), addrStr, serviceName, geoIpData, mComponent->getProjectId(parentSession->getProductName()),
                    LOCALE_BLANK, 0, parentSession->getPeerInfo()->getClientInfo()->getClientVersion(), sendPINErrorEvent,
                    parentSession->getExistenceData().getSessionFlags().getIsChildAccount());
            }
            else
            {
                gUserSessionManager->getGeoIpData(getPeerInfo()->getRealPeerAddress(), geoIpData);

                ClientPlatformType clientPlatform = NATIVE; // We default to NATIVE as this is the default for ClientInfo
                const char8_t* clientVersion = "";
                const char8_t* sdkVersion = "";
                if (getPeerInfo() != nullptr && getPeerInfo()->getClientInfo() != nullptr)
                {
                    clientPlatform = getPeerInfo()->getClientInfo()->getPlatform();
                    clientVersion = getPeerInfo()->getClientInfo()->getClientVersion();
                    sdkVersion = getPeerInfo()->getClientInfo()->getBlazeSDKVersion();
                }

                PlatformInfo platformInfo;
                convertToPlatformInfo(platformInfo, INVALID_EXTERNAL_ID, nullptr, INVALID_ACCOUNT_ID, clientPlatform);
                gUserSessionMaster->generateLoginEvent(err, getUserSessionId(), USER_SESSION_GUEST,
                    platformInfo, "", DEVICELOCALITY_UNKNOWN, INVALID_BLAZE_ID, nullptr,
                    getPeerInfo()->getLocale(), getPeerInfo()->getCountry(), sdkVersion, getPeerInfo()->getClientType(), addrStr, serviceName, geoIpData, mComponent->getProjectId(productName),
                    LOCALE_BLANK, 0, clientVersion, sendPINErrorEvent);
            }

             return commandErrorFromBlazeError(err);
        }
        else
        {
            mComponent->getMetricsInfoObj()->mGuestLogins.increment(1, productName, getPeerInfo()->getClientType());
            char8_t lastAuth[MAX_DATETIME_LENGTH];
            Blaze::TimeValue curTime = Blaze::TimeValue::getTimeOfDay();
            curTime.toAccountString(lastAuth, sizeof(lastAuth));
            mComponent->getMetricsInfoObj()->setLastAuthenticated(lastAuth);
        }

        char8_t sessionKey[MAX_SESSION_KEY_LENGTH];
        guestSession->getKey(sessionKey);

        // fill out response
        mResponse.setSessionKey(sessionKey);
        mResponse.setBlazeId(guestSession->getBlazeId());
        mResponse.setAccountId(guestSession->getUserInfo().getPlatformInfo().getEaIds().getNucleusAccountId());
        mResponse.setDisplayName(guestSession->getUserInfo().getPersonaName());

        // Return results.
        return ERR_OK;
    }
};
DEFINE_GUESTLOGIN_CREATE();

/* Private methods *******************************************************************************/


} // Authentication
} // Blaze
