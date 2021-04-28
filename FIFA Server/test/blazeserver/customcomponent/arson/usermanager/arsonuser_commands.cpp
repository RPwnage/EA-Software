#include "framework/blaze.h"
#include "framework/util/locales.h"
#include "arson/rpc/arsonslave/getuserinfobyblazeid_stub.h"
#include "arson/rpc/arsonslave/getuserinfobyblazeidgos12576_stub.h"
#include "arson/rpc/arsonslave/lookupidentity_stub.h"
#include "arson/rpc/arsonslave/arsonloginusersession_stub.h"
#include "authentication/util/updateuserinfoutil.h" // for UpdateUserInfoUtil::updateLocaleAndErrorByPersonaName in ArsonLoginUserSessionCommand::execute()
#include "framework/usersessions/usersessionmanager.h"
#include "framework/connection/connection.h"

#include "arsonslaveimpl.h"
#include "authentication/authenticationimpl.h"

namespace Blaze
{

namespace Arson
{

class GetUserInfoByBlazeIdCommand : public GetUserInfoByBlazeIdCommandStub
{
public:
    GetUserInfoByBlazeIdCommand(Message* message, GetUserRequest *req, ArsonSlaveImpl* componentImp)
        : GetUserInfoByBlazeIdCommandStub(message, req)
    {
    }

    ~GetUserInfoByBlazeIdCommand() override { }

    GetUserInfoByBlazeIdCommandStub::Errors execute() override
    {
        UserInfoPtr info;
        BlazeRpcError err = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getId(), info);
        if (err == Blaze::ERR_OK)
        {
            mResponse.setAccountId(info->getPlatformInfo().getEaIds().getNucleusAccountId());
            mResponse.setPersonaName(info->getPersonaName());
        }
        return static_cast<Errors>(err);
    }

};

DEFINE_GETUSERINFOBYBLAZEID_CREATE()


class GetUserInfoByBlazeIdGOS12576Command : public GetUserInfoByBlazeIdGOS12576CommandStub
{
public:
    GetUserInfoByBlazeIdGOS12576Command(Message* message, GetUserInfoByBlazeIdGOS12576Request *req, ArsonSlaveImpl* componentImp)
        : GetUserInfoByBlazeIdGOS12576CommandStub(message, req)
    {
    }

    ~GetUserInfoByBlazeIdGOS12576Command() override { }

    GetUserInfoByBlazeIdGOS12576CommandStub::Errors execute() override
    {
        UserInfoPtr info1;
        UserInfoPtr info2;
        UserInfoPtr info3;

        //Call from user0 gUserSessionManager->lookupUserByBlazeId for user1, user2, user3 sequentially (with a cache of 2, you need to do at least 3 
        //lookups to push the first guy you looked up out of the cache, so at least lookup user1, then 2, then 3)       
        Blaze::BlazeRpcError err1 = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getId1(), info1);
        Blaze::BlazeRpcError err2 = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getId2(), info2);
        Blaze::BlazeRpcError err3 = gUserSessionManager->lookupUserInfoByBlazeId(mRequest.getId3(), info3);

        Blaze::BlazeRpcError overallErr = Blaze::ERR_SYSTEM;
        if (err1 == Blaze::ERR_OK && err2 == Blaze::ERR_OK && err3 == Blaze::ERR_OK)
        {
            overallErr = Blaze::ERR_OK;
        }
        else        
        {
            //bail out for non-ERR_OK overallErr instead of dangeriously proceed further
            return ERR_SYSTEM;
        }

        //User1 userinfo
        //Call methods on the first user that you looked up (eg. user1)
        const Blaze::AccountId accountId1 = info1->getPlatformInfo().getEaIds().getNucleusAccountId();
        const char8_t* persona1 = info1->getPersonaName();
        const Blaze::BlazeId blazeObjId = info1->getBlazeObjectId().id;

        mResponse.setAccountId(accountId1);
        mResponse.setPersonaName(persona1);
        mResponse.setId(blazeObjId);
        
        return static_cast<Errors>(overallErr);
    }

};

DEFINE_GETUSERINFOBYBLAZEIDGOS12576_CREATE()

class LookupIdentityCommand : public LookupIdentityCommandStub
{
public:
    LookupIdentityCommand(Message* message, IdentityRequest *req, ArsonSlaveImpl* componentImp)
        : LookupIdentityCommandStub(message, req)
    {
    }

    ~LookupIdentityCommand() override { }

    LookupIdentityCommandStub::Errors execute() override
    {
        size_t size = mRequest.getIds().size();
        if (size == 0)
        {
            return ARSON_ERR_INVALID_PARAMETER;
        }
        const EA::TDF::ObjectType bobjType = mRequest.getBlazeObjectType();
        if (size == 1)
        {
            Blaze::IdentityInfo result;
            BlazeRpcError err;
            if ((err = gIdentityManager->getIdentity(bobjType, mRequest.getIds().front(), result)) == Blaze::ERR_OK)
            {
                IdentityResponse::NamesByIdMap& names = mResponse.getNamesById();
                names[result.getBlazeObjectId().id] = result.getIdentityName();
                return ERR_OK;
            }
            else
            {
                return commandErrorFromBlazeError(err);
            }
        }

        EntityIdList keys;
        IdentityRequest::IdsList::iterator i = mRequest.getIds().begin();
        IdentityRequest::IdsList::iterator e = mRequest.getIds().end();
        for(; i != e; ++i)
            keys.push_back(*i);

        Blaze::IdentityInfoByEntityIdMap result;
        BlazeRpcError err = gIdentityManager->getIdentities(bobjType, keys, result);
        if (err == Blaze::ERR_OK)
        {
            IdentityResponse::NamesByIdMap& names = mResponse.getNamesById();
            Blaze::IdentityInfoByEntityIdMap::const_iterator it = result.begin();
            Blaze::IdentityInfoByEntityIdMap::const_iterator itEnd = result.end();
            for(; it != itEnd; ++it)
                names[it->first] = it->second->getIdentityName();
            return ERR_OK;
        }
        else
        {
            return commandErrorFromBlazeError(err);
        }
    }

};

DEFINE_LOOKUPIDENTITY_CREATE()

/* \brief Backdoor arson helper. login the session simulating any specified LoginUserSessionRequest params (e.g. use this to fake external ids, external system id etc). */
class ArsonLoginUserSessionCommand : public ArsonLoginUserSessionCommandStub
{
public:
    ArsonLoginUserSessionCommand(Message* message, ArsonLoginSessionRequest *req, ArsonSlaveImpl* componentImp)
        : ArsonLoginUserSessionCommandStub(message, req)
    {}
    ~ArsonLoginUserSessionCommand() override {}

    ArsonLoginUserSessionCommandStub::Errors execute() override
    {
        BlazeRpcError loginErr = Blaze::ERR_OK;

        // ensure can get auth component
        Authentication::AuthenticationSlaveImpl* authComponent = 
            static_cast<Authentication::AuthenticationSlaveImpl*>(gController->getComponent(Authentication::AuthenticationSlave::COMPONENT_ID));    
        if (authComponent == nullptr)
        {
            WARN_LOG("[ArsonLoginUserSessionCommand].execute() - authentication component is not available");
            return ERR_SYSTEM;
        }

        // setup and call the loginUserSession
        if ( *(mRequest.getPersonaName()) == '\0' )
        {
            ERR_LOG("[ArsonLoginUserSessionCommand].execute(): missing mRequest.getPersonaName()! failed with error[AUTH_ERR_FIELD_MISSING]");
            loginErr = Blaze::ARSON_ERR_AUTH_FIELD_MISSING;
        }
        if (mRequest.getAccountId() == 0)
        {
            ERR_LOG("[ArsonLoginUserSessionCommand].execute(): missing mRequest.getAccountId()! failed with error[AUTH_ERR_FIELD_MISSING]");
            loginErr = Blaze::ARSON_ERR_AUTH_FIELD_MISSING;
        }
        if (mRequest.getBlazeId() == 0)
        {
            ERR_LOG("[ArsonLoginUserSessionCommand].execute(): missing mRequest.getBlazeId()! failed with error[AUTH_ERR_FIELD_MISSING]");
            loginErr = Blaze::ARSON_ERR_AUTH_FIELD_MISSING;
        }
        if ( *(mRequest.getPersonaName()) != '\0' )
        {
            Blaze::Authentication::UpdateUserInfoUtil updateUserInfoUtil(*authComponent);
            updateUserInfoUtil.updateLocaleAndErrorByPersonaName(mRequest.getClientPlatformType(), gController->getNamespaceFromPlatform(mRequest.getClientPlatformType()), mRequest.getPersonaName(), getPeerInfo()->getLocale(), getPeerInfo()->getCountry(), loginErr);
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
            userData.setId(mRequest.getBlazeId());
            // CROSSPLAY_TODO: This user's blazeId is the same as his OriginPersonaId, and his personaName is the same as his OriginPersonaName.
            // This would normally be true only for PC clients (but that might be a non-issue, since this RPC is only used for running Arson tests with Nucleus bypass on).
            if ( *(mRequest.getExternalString()) != '\0' )
            {
                convertToPlatformInfo(userData.getPlatformInfo(), 0, mRequest.getExternalString(), mRequest.getAccountId(), mRequest.getOriginPersonaId(), mRequest.getOriginPersonaName(), mRequest.getClientPlatformType());
            }
            else
            {
                convertToPlatformInfo(userData.getPlatformInfo(), mRequest.getExternalId(), nullptr, mRequest.getAccountId(), mRequest.getOriginPersonaId(), mRequest.getOriginPersonaName(), mRequest.getClientPlatformType());
            }

            userData.setPersonaName(mRequest.getPersonaName());
            userData.setPersonaNamespace(gController->getNamespaceFromPlatform(mRequest.getClientPlatformType()));
            userData.setAccountLocale(accountLocale);
            userData.setAccountCountry(accountCountry);
            userData.setCrossPlatformOptIn(mRequest.getCrossPlatformOptIn());

            bool geoIpSucceeded = true;
            NucleusIdentity::IpGeoLocation geoLoc; // just any dummy data
            UserInfoDbCalls::UpsertUserInfoResults upsertUserInfoResults(accountCountry);

            // productName is not specified in the request; use the default of the service. 
            const char8_t* productName = gController->getServiceProductName(getPeerInfo()->getServiceName());

            loginErr = gUserSessionMaster->createNormalUserSession(userData, *getPeerInfo(), mRequest.getClientType(), getConnectionUserIndex(),
                productName, true, geoLoc, geoIpSucceeded, upsertUserInfoResults, false, true);

            if(loginErr == USER_ERR_EXISTS)
            {
                loginErr = Blaze::ARSON_ERR_AUTH_EXISTS;
            }
            if (loginErr != Blaze::ERR_OK)
            {
                ERR_LOG("[ArsonLoginUserSessionCommand].execute(): failed with error[" << ErrorHelp::getErrorName(loginErr) << "]");
                authComponent->getMetricsInfoObj()->mLoginFailures.increment(1, productName, getPeerInfo()->getClientType());
            }
            else
            {
                char8_t sessionKey[MAX_SESSION_KEY_LENGTH];

                authComponent->getMetricsInfoObj()->mLogins.increment(1, productName, getPeerInfo()->getClientType());
                char8_t lastAuth[Blaze::Authentication::MAX_DATETIME_LENGTH];
                Blaze::TimeValue curTime = Blaze::TimeValue::getTimeOfDay();
                curTime.toAccountString(lastAuth, sizeof(lastAuth));
                authComponent->getMetricsInfoObj()->setLastAuthenticated(lastAuth);
                mResponse.getUserLoginInfo().setBlazeId(gCurrentLocalUserSession->getUserInfo().getId());
                gCurrentLocalUserSession->getKey(sessionKey);
                mResponse.getUserLoginInfo().setSessionKey(sessionKey);
                mResponse.getUserLoginInfo().setAccountId(userData.getPlatformInfo().getEaIds().getNucleusAccountId());
                userData.getPlatformInfo().copyInto(mResponse.getUserLoginInfo().getPlatformInfo());  
                mResponse.getUserLoginInfo().getPersonaDetails().setDisplayName(mRequest.getPersonaName());
                mResponse.getUserLoginInfo().getPersonaDetails().setPersonaId(mRequest.getAccountId());
                mResponse.getUserLoginInfo().getPersonaDetails().setLastAuthenticated(lastAuthenticated);
                mResponse.getUserLoginInfo().setLastLoginDateTime(gCurrentLocalUserSession->getUserInfo().getPreviousLoginDateTime());
                mResponse.getUserLoginInfo().setGeoIpSucceeded(geoIpSucceeded);
            }
        }
        return static_cast<Errors>(loginErr);
    }//execute()

};
DEFINE_ARSONLOGINUSERSESSION_CREATE()

} // namespace Arson
} // namespace Blaze

