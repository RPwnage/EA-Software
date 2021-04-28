/**************************************************************************************************/
/*! 
    \file stresslogin_command.cpp
    
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

/*** Include Files ********************************************************************************/
#include "framework/blaze.h"
#include "framework/util/locales.h"
#include "authentication/rpc/authenticationslave/stresslogin_stub.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "authentication/authenticationimpl.h"

#include "framework/usersessions/usersessionmanager.h"
#include "framework/controller/controller.h"
#include "authentication/util/updateuserinfoutil.h"

namespace Blaze
{
namespace Authentication
{

/*** Public Methods *******************************************************************************/


/*! ***********************************************************************************************/
/*! \class StressLoginCommand

\brief Uses encrypted authentication token and persona ID to login a user.

\note StressLoginCommand follows these steps:
\li Authenticate token to get user ID, email, and password.
\li Add user session.
***************************************************************************************************/
class StressLoginCommand : public StressLoginCommandStub
{
public:
    StressLoginCommand(Message* message, StressLoginRequest* request,
        AuthenticationSlaveImpl* componentImpl)
        : StressLoginCommandStub(message, request),
        mComponent(componentImpl),
        mUpdateUserInfoUtil(*componentImpl)
    {
    }

/*** Private Methods ******************************************************************************/
private:
    StressLoginCommandStub::Errors onLoginFailure(BlazeRpcError loginErr, ClientPlatformType clientPlatform, ClientType clientType, const char8_t* productName, bool rateLimited = false)
    {
        WARN_LOG("[StressLoginCommand].onLoginFailure: loginUserSession for nucleus id[" << mRequest.getAccountId() 
            << "], persona[" << mRequest.getPersonaName() << "], failed with error[" << ErrorHelp::getErrorName(loginErr) << "]");
        
        mComponent->getMetricsInfoObj()->mLoginFailures.increment(1, productName, clientType);

        if (!rateLimited && *(mRequest.getPersonaName()) != '\0' )
        {
            const char8_t* namespaceName = gController->getNamespaceFromPlatform(clientPlatform);
            mUpdateUserInfoUtil.updateLocaleAndErrorByPersonaName(clientPlatform, namespaceName, mRequest.getPersonaName(), getPeerInfo()->getLocale(), getPeerInfo()->getCountry(), loginErr);
        }
        
        return commandErrorFromBlazeError(loginErr);
    }

    // Not owned memory.
    AuthenticationSlaveImpl* mComponent;
    UpdateUserInfoUtil mUpdateUserInfoUtil;

    StressLoginCommandStub::Errors execute() override
    {
        BlazeRpcError loginErr = Blaze::ERR_OK;

        const char8_t* serviceName = getPeerInfo()->getServiceName();

        const char8_t* productName = mRequest.getProductName();
        if (productName[0] == '\0') // productName is optional. If not specified, use the default of the service. 
            productName = gController->getServiceProductName(serviceName);

        ClientPlatformType servicePlatform = gController->getServicePlatform(serviceName);
        if (servicePlatform == common)
        {
            // If connected to the 'common' default service name (i.e. via gRPC on the EADP Service Mesh),
            // then the product name can be used to specify the service platform
            ClientPlatformType productPlatform = mComponent->getProductPlatform(productName);
            if (productPlatform != INVALID)
                servicePlatform = productPlatform;
        }

        ClientType clientType = mRequest.getClientType();
        if (clientType == CLIENT_TYPE_INVALID)
            clientType = getPeerInfo()->getClientType();

        if (!mComponent->isValidProductName(productName))
        {
            ERR_LOG("[StressLoginCommand].execute: Invalid product name " << productName << " specified in request.");

            // If requested productName is not valid, use the default of the service.  
            productName = gController->getServiceProductName(serviceName);
            return onLoginFailure(Blaze::AUTH_ERR_INVALID_FIELD, servicePlatform, clientType, productName);
        }

        if (!mComponent->isBelowPsuLimitForClientType(clientType))
        {
            INFO_LOG("[StressLoginCommand].execute : PSU limit of " << mComponent->getPsuLimitForClientType(clientType) << " for client type "
                << ClientTypeToString(clientType) << " has been reached.");
            return onLoginFailure(Blaze::AUTH_ERR_EXCEEDS_PSU_LIMIT, servicePlatform, clientType, productName);
        }

        if (!mComponent->shouldBypassRateLimiter(clientType))
        {
            loginErr = mComponent->tickAndCheckLoginRateLimit();
            if (loginErr != Blaze::ERR_OK)
            {
                // an error occured while the fiber was asleep, so we must exit with the error.
                WARN_LOG("[StressLoginCommand].execute: tickAndCheckLoginRateLimit for persona[" 
                    << mRequest.getPersonaName() << "] failed with error[" <<
                    ErrorHelp::getErrorName(loginErr) << "]");
                
                return onLoginFailure(loginErr, servicePlatform, clientType, productName, true);
            }
        }

        if (!mComponent->getAllowStressLogin())
        {
            WARN_LOG("[StressLoginCommand].execute: Stress login disabled for server. Failed with error[ERR_SYSTEM]");
            return onLoginFailure(Blaze::ERR_SYSTEM, servicePlatform, clientType, productName);
        }

        if ( *(mRequest.getPersonaName()) == '\0' )
        {
            WARN_LOG("[StressLoginCommand].execute: No persona name provided. Failed with error[AUTH_ERR_FIELD_MISSING]");
            return onLoginFailure(Blaze::AUTH_ERR_FIELD_MISSING, servicePlatform, clientType, productName);
        }

        if (mRequest.getAccountId() == 0)
        {
            WARN_LOG("[StressLoginCommand].execute: No Nucleus id provided. Failed with error[AUTH_ERR_FIELD_MISSING]");
            return onLoginFailure(Blaze::AUTH_ERR_FIELD_MISSING, servicePlatform, clientType, productName);
        }

        if (loginErr == Blaze::ERR_OK)
        {
            TimeValue curtime = TimeValue::getTimeOfDay();

            Locale accountLocale = LOCALE_EN_US;
            uint32_t accountCountry = COUNTRY_UNITED_STATES;
            // For lack of a better last auth time, we will use "now"
            uint32_t lastAuthenticated = static_cast<uint32_t>(curtime.getSec());

            //Save off the user data
            UserInfoData userData;
            userData.setId(mRequest.getAccountId());
            //w/stress login, mock account info won't be via nucleus, just reuse accountId for all the ids:
            convertToPlatformInfo(userData.getPlatformInfo(), mRequest.getAccountId(), nullptr, mRequest.getAccountId(), mRequest.getAccountId(), mRequest.getPersonaName(), servicePlatform);

            const char8_t* namespaceName = gController->getNamespaceFromPlatform(servicePlatform);

            userData.setPersonaName(mRequest.getPersonaName());
            userData.setPersonaNamespace(namespaceName);
            userData.setAccountLocale(accountLocale);
            userData.setAccountCountry(accountCountry);
            userData.setLastLoginDateTime(TimeValue::getTimeOfDay().getSec());
            userData.setLastLocaleUsed(getPeerInfo()->getLocale());
            userData.setLastCountryUsed(getPeerInfo()->getCountry());
            userData.setLastAuthErrorCode(Blaze::ERR_OK);

            bool geoIpSucceeded = true;
            NucleusIdentity::IpGeoLocation geoLoc; // just any dummy data
            UserInfoDbCalls::UpsertUserInfoResults upsertUserInfoResults(accountCountry);
            loginErr = gUserSessionMaster->createNormalUserSession(userData, *getPeerInfo(), clientType, getConnectionUserIndex(), productName, true, geoLoc, geoIpSucceeded, upsertUserInfoResults, false, false);
            if (loginErr == ERR_MOVED)
            {
                // ERR_MOVED means the client was redirected to another coreSlave because we're not yet ready to own a UserSession, and basically, nothing has really happened.
                return commandErrorFromBlazeError(loginErr);
            }

            if(loginErr == USER_ERR_EXISTS)
            {
                 return onLoginFailure(Blaze::AUTH_ERR_EXISTS, servicePlatform, clientType, productName);
            }

            if (loginErr != Blaze::ERR_OK)
            {
                if (loginErr == ERR_DUPLICATE_LOGIN)
                {
                    loginErr = AUTH_ERR_DUPLICATE_LOGIN;
                }
                else if (loginErr == ERR_NOT_PRIMARY_PERSONA)
                {
                    loginErr = AUTH_ERR_NOT_PRIMARY_PERSONA;
                }
                return onLoginFailure(loginErr, servicePlatform, clientType, productName);
            }
            else
            {
                char8_t sessionKey[MAX_SESSION_KEY_LENGTH];

                mComponent->getMetricsInfoObj()->mLogins.increment(1, productName, clientType);
                 char8_t lastAuth[MAX_DATETIME_LENGTH];
                Blaze::TimeValue curTime = Blaze::TimeValue::getTimeOfDay();
                curTime.toAccountString(lastAuth, sizeof(lastAuth));
                mComponent->getMetricsInfoObj()->setLastAuthenticated(lastAuth);
                mResponse.getUserLoginInfo().setBlazeId(gCurrentLocalUserSession->getUserInfo().getId());
                gCurrentLocalUserSession->getKey(sessionKey);
                mResponse.getUserLoginInfo().setSessionKey(sessionKey);
                userData.getPlatformInfo().copyInto(mResponse.getUserLoginInfo().getPlatformInfo());
                mResponse.getUserLoginInfo().setAccountId(mResponse.getUserLoginInfo().getPlatformInfo().getEaIds().getNucleusAccountId());

                mResponse.getUserLoginInfo().setIsFirstLogin(gCurrentLocalUserSession->getSessionFlags().getFirstLogin());
                mResponse.getUserLoginInfo().setIsFirstConsoleLogin(gCurrentUserSession->getSessionFlags().getFirstConsoleLogin());
                mResponse.getUserLoginInfo().getPersonaDetails().setDisplayName(mRequest.getPersonaName());
                mResponse.getUserLoginInfo().getPersonaDetails().setPersonaId(mRequest.getAccountId());
                mResponse.getUserLoginInfo().getPersonaDetails().setLastAuthenticated(lastAuthenticated);
                mResponse.getUserLoginInfo().setLastLoginDateTime(gCurrentLocalUserSession->getUserInfo().getPreviousLoginDateTime());
                mResponse.getUserLoginInfo().setGeoIpSucceeded(geoIpSucceeded);

                if ( *(mRequest.getPersonaName()) != '\0' )
                {
                    mUpdateUserInfoUtil.updateLocaleAndErrorByPersonaName(servicePlatform, namespaceName, mRequest.getPersonaName(), getPeerInfo()->getLocale(), getPeerInfo()->getCountry(), loginErr);
                }
            }
        }

        return commandErrorFromBlazeError(loginErr);
    }
};
DEFINE_STRESSLOGIN_CREATE()

} // Authentication
} // Blaze
