/*************************************************************************************************/
/*!
    \file   comparenucleusapiresponses_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/comparenucleusapiresponses_stub.h"

#include "authentication/authenticationimpl.h"
#include "authentication/util/authenticationutil.h"
#include "framework/oauth/oauthslaveimpl.h"
#include "framework/oauth/accesstokenutil.h"
#include "framework/tdf/userdefines.h"
#include "framework/usersessions/usersession.h"

#include "proxycomponent/nucleusconnect/tdf/nucleusconnect.h"
#include "proxycomponent/nucleusidentity/tdf/nucleusidentity.h"

namespace Blaze
{
    namespace Arson
    {
        class CompareNucleusApiResponsesCommand : public CompareNucleusApiResponsesCommandStub
        {
        public:
            CompareNucleusApiResponsesCommand(Message* message, Blaze::Arson::CompareNucleusApiResponsesRequest* request, ArsonSlaveImpl* componentImpl)
                : CompareNucleusApiResponsesCommandStub(message, request),
                mComponent(componentImpl)
            {
                mAuthComponent = static_cast<Blaze::Authentication::AuthenticationSlaveImpl*>(
                    gController->getComponent(Authentication::AuthenticationSlave::COMPONENT_ID));
            }

            ~CompareNucleusApiResponsesCommand() override { }

        private:
            // Not owned memory
            ArsonSlaveImpl* mComponent;
            Authentication::AuthenticationSlaveImpl *mAuthComponent;

            CompareNucleusApiResponsesCommandStub::Errors execute() override
            {
                //this function gets responses of jwt token and opaque token in the same order as LoginCommand::doLogin and compare the responses
                INFO_LOG("[CompareNucleusApiResponsesCommand].execute:" << mRequest);

                if (gCurrentUserSession == nullptr)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].execute: gCurrentUserSession is nullptr");
                    return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
                }

                BlazeRpcError err = Blaze::ERR_SYSTEM;
                OAuth::OAuthSlaveImpl* oAuthSlave = mAuthComponent->oAuthSlaveImpl();
                const char8_t* serviceName = getPeerInfo()->getServiceName();
                ClientPlatformType clientPlatform = gController->getServicePlatform(serviceName);
                InetAddress inetAddr = Authentication::AuthenticationUtil::getRealEndUserAddr(this);
                Blaze::AccountId pidId = gCurrentUserSession->getAccountId();
                Blaze::PersonaId personaId = gCurrentUserSession->getBlazeId();
                const char8_t* authCode = mRequest.getAuthCode();

                NucleusConnect::GetAccessTokenResponse opaqueResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getAccessTokenResponse("", authCode, "arsontestproductname", inetAddr, opaqueResponse));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].execute: getAccessTokenResponse failed to get opaqueResponse with error(" << ErrorHelp::getErrorName(err) << ")");
                    return commandErrorFromBlazeError(err);
                }

                const char8_t* opaqueToken = opaqueResponse.getAccessToken();
                const char8_t* jwtToken = mRequest.getAccessToken();

                NucleusConnect::GetAccessTokenResponse jwtResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getAccessTokenResponse(jwtToken, "", "arsontestproductname", inetAddr, jwtResponse));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].execute: getAccessTokenResponse failed to get jwtResponse with error(" << ErrorHelp::getErrorName(err) << ")");
                    return commandErrorFromBlazeError(err);
                }
                
                int mismatchCount = 0;
                mismatchCount += compareField("AccessToken", jwtResponse.getAccessToken(), jwtToken);
                mismatchCount += compareField("RefreshToken", jwtResponse.getRefreshToken(), "");
                mismatchCount += compareField("RefreshExpiresIn", jwtResponse.getRefreshExpiresIn(), 0);
                mismatchCount += compareField("TokenType", jwtResponse.getTokenType(), "Bearer");

                if (mismatchCount != 0)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].execute: " << mismatchCount << " fields of JWT GetAccessTokenResponse do not match expectation");
                    return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
                }

                NucleusConnect::JwtPayloadInfo jwtPayloadInfo;
                NucleusConnect::GetTokenInfoResponse tokenInfoResponse;
                err = compareGetTokenInfoResponses(jwtToken, opaqueToken, tokenInfoResponse, jwtPayloadInfo);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].execute: compareGetTokenInfoResponses failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return commandErrorFromBlazeError(err);
                }

                err = compareGetAccountResponses(jwtToken, opaqueToken, jwtPayloadInfo);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].execute: compareGetAccountResponses failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return commandErrorFromBlazeError(err);
                }

                err = compareGetPersonaListResponse(jwtToken, opaqueToken, jwtPayloadInfo, pidId, NAMESPACE_ORIGIN);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].execute: compareGetPersonaListResponse failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return commandErrorFromBlazeError(err);
                }

                const char8_t* platformNamespaceName = gController->getNamespaceFromPlatform(clientPlatform);
                err = compareGetPersonaListResponse(jwtToken, opaqueToken, jwtPayloadInfo, pidId, platformNamespaceName);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].execute: compareGetPersonaListResponse failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return commandErrorFromBlazeError(err);
                }

                if (clientPlatform == Blaze::steam)
                {
                    err = compareGetExternalRefResponse(jwtToken, opaqueToken, jwtPayloadInfo, personaId);
                    if (err != Blaze::ERR_OK)
                    {
                        ERR_LOG("[CompareNucleusApiResponsesCommand].execute: compareGetExternalRefResponse failed with error(" << ErrorHelp::getErrorName(err) << ")");
                        return commandErrorFromBlazeError(err);
                    }
                }

                if (clientPlatform != Blaze::nx)
                {
                    err = compareGetPersonaInfoResponse(jwtToken, opaqueToken, jwtPayloadInfo, pidId, personaId);
                    if (err != Blaze::ERR_OK)
                    {
                        ERR_LOG("[CompareNucleusApiResponsesCommand].execute: compareGetPersonaInfoResponse failed with error(" << ErrorHelp::getErrorName(err) << ")");
                        return commandErrorFromBlazeError(err);
                    }
                }

                mResponse.setErrorMessage("ERR_OK");
                return commandErrorFromBlazeError(err);
            }

            BlazeRpcError compareGetTokenInfoResponses(const char8_t* jwtToken, const char8_t* opaqueToken, NucleusConnect::GetTokenInfoResponse& response, NucleusConnect::JwtPayloadInfo& payloadInfo)
            {
                BlazeRpcError err = Blaze::ERR_SYSTEM;
                OAuth::OAuthSlaveImpl* oAuthSlave = mAuthComponent->oAuthSlaveImpl();
                const char8_t* serviceName = getPeerInfo()->getServiceName();
                ClientPlatformType clientPlatform = gController->getServicePlatform(serviceName);
                InetAddress inetAddr = Authentication::AuthenticationUtil::getRealEndUserAddr(this);
                const char8_t* clientId = mAuthComponent->getBlazeServerNucleusClientId();

                NucleusConnect::GetTokenInfoRequest request;
                request.setAccessToken(jwtToken);
                request.setIpAddress(inetAddr.getIpAsString());
                request.setClientId(clientId);

                NucleusConnect::GetTokenInfoResponse jwtResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getTokenInfo(request, jwtResponse, payloadInfo, clientPlatform));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetTokenInfoResponses: getTokenInfo failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                request.setAccessToken(opaqueToken);
                NucleusConnect::GetTokenInfoResponse opaqueResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getTokenInfo(request, opaqueResponse, payloadInfo, clientPlatform));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetTokenInfoResponses: getTokenInfo failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                int mismatchCount = 0;
                mismatchCount += compareField("AuthSource", jwtResponse.getAuthSource(), opaqueResponse.getAuthSource());
                mismatchCount += compareField("ConnectionType", jwtResponse.getConnectionType(), opaqueResponse.getConnectionType());
                mismatchCount += compareField("DeviceId", jwtResponse.getDeviceId(), opaqueResponse.getDeviceId());
                mismatchCount += compareField("PersonaId", jwtResponse.getPersonaId(), opaqueResponse.getPersonaId());
                mismatchCount += compareField("PidType", jwtResponse.getPidType(), opaqueResponse.getPidType());
                mismatchCount += compareField("PidId", jwtResponse.getPidId(), opaqueResponse.getPidId());
                mismatchCount += compareField("ConsoleEnv", jwtResponse.getConsoleEnv(), opaqueResponse.getConsoleEnv());
                mismatchCount += compareField("StopProcess", jwtResponse.getStopProcess(), opaqueResponse.getStopProcess());
                mismatchCount += compareField("IsUnderage", jwtResponse.getIsUnderage(), opaqueResponse.getIsUnderage());
                mismatchCount += compareField("AccountId", jwtResponse.getAccountId(), opaqueResponse.getAccountId());
                mismatchCount += compareField("ipgeo Country", jwtResponse.getIpGeoLocation().getCountry(), opaqueResponse.getIpGeoLocation().getCountry());
                mismatchCount += compareField("ipgeo City", jwtResponse.getIpGeoLocation().getCity(), opaqueResponse.getIpGeoLocation().getCity());
                mismatchCount += compareField("ipgeo Region", jwtResponse.getIpGeoLocation().getRegion(), opaqueResponse.getIpGeoLocation().getRegion());
                mismatchCount += compareField("ipgeo Latitude", jwtResponse.getIpGeoLocation().getLatitude(), opaqueResponse.getIpGeoLocation().getLatitude());
                mismatchCount += compareField("ipgeo Longitude", jwtResponse.getIpGeoLocation().getLongitude(), opaqueResponse.getIpGeoLocation().getLongitude());
                mismatchCount += compareField("ipgeo TimeZone", jwtResponse.getIpGeoLocation().getTimeZone(), opaqueResponse.getIpGeoLocation().getTimeZone());
                mismatchCount += compareField("ipgeo ISPName", jwtResponse.getIpGeoLocation().getISPName(), opaqueResponse.getIpGeoLocation().getISPName());
                
                if (mismatchCount != 0)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetTokenInfoResponses: " << mismatchCount << " fields of JWT response do not match OPAQUE response");
                    return Blaze::ERR_SYSTEM;
                }

                response = jwtResponse;
                return err;
            }

            BlazeRpcError compareGetAccountResponses(const char8_t* jwtToken, const char8_t* opaqueToken, const NucleusConnect::JwtPayloadInfo& payloadInfo)
            {
                BlazeRpcError err = Blaze::ERR_SYSTEM;
                OAuth::OAuthSlaveImpl* oAuthSlave = mAuthComponent->oAuthSlaveImpl();
                const char8_t* serviceName = getPeerInfo()->getServiceName();
                ClientPlatformType clientPlatform = gController->getServicePlatform(serviceName);
                InetAddress inetAddr = Authentication::AuthenticationUtil::getRealEndUserAddr(this);
                const char8_t* addrStr = inetAddr.getIpAsString();
                const char8_t* clientId = mAuthComponent->getBlazeServerNucleusClientId();

                eastl::string jwtTokenBearer(eastl::string::CtorSprintf(), "Bearer %s", jwtToken);
                eastl::string opaqueTokenBearer(eastl::string::CtorSprintf(), "Bearer %s", opaqueToken);

                NucleusIdentity::GetAccountRequest request;
                request.getAuthCredentials().setAccessToken(jwtTokenBearer.c_str());
                request.getAuthCredentials().setClientId(clientId);
                request.getAuthCredentials().setIpAddress(addrStr);

                NucleusIdentity::GetAccountResponse jwtResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getAccount(request, jwtResponse, &payloadInfo, clientPlatform));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetAccountResponses: getAccount failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                request.getAuthCredentials().setAccessToken(opaqueTokenBearer.c_str());
                NucleusIdentity::GetAccountResponse opaqueResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getAccount(request, opaqueResponse, &payloadInfo, clientPlatform));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetAccountResponses: getAccount failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                int mismatchCount = 0;
                mismatchCount += compareField("UnderagePid", jwtResponse.getPid().getUnderagePid(), opaqueResponse.getPid().getUnderagePid());
                mismatchCount += compareField("AnonymousPid", jwtResponse.getPid().getAnonymousPid(), opaqueResponse.getPid().getAnonymousPid());
                mismatchCount += compareField("Country", jwtResponse.getPid().getCountry(), opaqueResponse.getPid().getCountry());
                mismatchCount += compareField("Language", jwtResponse.getPid().getLanguage(), opaqueResponse.getPid().getLanguage());
                mismatchCount += compareField("Status", jwtResponse.getPid().getStatus(), opaqueResponse.getPid().getStatus());
                mismatchCount += compareField("Locale", jwtResponse.getPid().getLocale(), opaqueResponse.getPid().getLocale());
                mismatchCount += compareField("ExternalRefValue", jwtResponse.getPid().getExternalRefValue(), opaqueResponse.getPid().getExternalRefValue());

                if (mismatchCount != 0)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetAccountResponses: " << mismatchCount << " fields of JWT response do not match OPAQUE response");
                    return Blaze::ERR_SYSTEM;
                }

                return err;
            }

            BlazeRpcError compareGetPersonaListResponse(const char8_t* jwtToken, const char8_t* opaqueToken, const NucleusConnect::JwtPayloadInfo& payloadInfo, const Blaze::AccountId pidId, const char8_t* namespaceName)
            {
                BlazeRpcError err = Blaze::ERR_SYSTEM;
                OAuth::OAuthSlaveImpl* oAuthSlave = mAuthComponent->oAuthSlaveImpl();
                InetAddress inetAddr = Authentication::AuthenticationUtil::getRealEndUserAddr(this);
                const char8_t* addrStr = inetAddr.getIpAsString();
                const char8_t* clientId = mAuthComponent->getBlazeServerNucleusClientId();

                OAuth::AccessTokenUtil tokenUtil;
                err = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetAccountResponses: Failed to retrieve server access token with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                NucleusIdentity::GetPersonaListRequest request;
                request.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
                request.getAuthCredentials().setUserAccessToken(jwtToken);
                request.getAuthCredentials().setClientId(clientId);
                request.getAuthCredentials().setIpAddress(addrStr);
                request.setPid(pidId);
                request.setExpandResults("true");
                request.getFilter().setNamespaceName(namespaceName);
                request.getFilter().setStatus("ACTIVE");

                NucleusIdentity::GetPersonaListResponse jwtResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getPersonaList(request, jwtResponse, &payloadInfo));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetPersonaListResponse: getPersonaList failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                request.getAuthCredentials().setUserAccessToken(opaqueToken);
                NucleusIdentity::GetPersonaListResponse opaqueResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getPersonaList(request, opaqueResponse, &payloadInfo));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetPersonaListResponse: getPersonaList failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                if (jwtResponse.getPersonas().getPersona().empty())
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetPersonaListResponse: jwtResponse.getPersonas().getPersona() is empty");
                    return Blaze::ERR_SYSTEM;
                }
                if (opaqueResponse.getPersonas().getPersona().empty())
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetPersonaListResponse: opaqueResponse.getPersonas().getPersona() is empty");
                    return Blaze::ERR_SYSTEM;
                }

                int mismatchCount = 0;
                mismatchCount += compareField("PersonaId", jwtResponse.getPersonas().getPersona()[0]->getPersonaId(), opaqueResponse.getPersonas().getPersona()[0]->getPersonaId());
                mismatchCount += compareField("NamespaceName", jwtResponse.getPersonas().getPersona()[0]->getNamespaceName(), opaqueResponse.getPersonas().getPersona()[0]->getNamespaceName());
                mismatchCount += compareField("DisplayName", jwtResponse.getPersonas().getPersona()[0]->getDisplayName(), opaqueResponse.getPersonas().getPersona()[0]->getDisplayName());

                if (mismatchCount != 0)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetPersonaListResponse: " << mismatchCount << " fields of JWT response do not match OPAQUE response");
                    return Blaze::ERR_SYSTEM;
                }

                return err;
            }

            BlazeRpcError compareGetExternalRefResponse(const char8_t* jwtToken, const char8_t* opaqueToken, const NucleusConnect::JwtPayloadInfo& payloadInfo, const Blaze::PersonaId personaId)
            {
                BlazeRpcError err = Blaze::ERR_SYSTEM;
                OAuth::OAuthSlaveImpl* oAuthSlave = mAuthComponent->oAuthSlaveImpl();
                InetAddress inetAddr = Authentication::AuthenticationUtil::getRealEndUserAddr(this);
                const char8_t* addrStr = inetAddr.getIpAsString();
                const char8_t* clientId = mAuthComponent->getBlazeServerNucleusClientId();

                OAuth::AccessTokenUtil fullScopeServerTokenUtil;
                {
                    //retrieve full scope server access token in a block to avoid changing function context permission
                    UserSession::SuperUserPermissionAutoPtr autoPtr(true);
                    err = fullScopeServerTokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
                    if (err != Blaze::ERR_OK)
                    {
                        ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetExternalRefResponse: Failed to retrieve full scope server access token with error(" << ErrorHelp::getErrorName(err) << ")");
                        return err;
                    }
                }

                NucleusIdentity::GetExternalRefRequest request;
                request.getAuthCredentials().setAccessToken(fullScopeServerTokenUtil.getAccessToken());
                request.getAuthCredentials().setUserAccessToken(jwtToken);
                request.getAuthCredentials().setClientId(clientId);
                request.getAuthCredentials().setIpAddress(addrStr);
                request.setExpandResults("true");
                request.setPersonaId(personaId);

                NucleusIdentity::GetExternalRefResponse jwtResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getExternalRef(request, jwtResponse, &payloadInfo));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetExternalRefResponse: getExternalRef failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                request.getAuthCredentials().setUserAccessToken(opaqueToken);
                NucleusIdentity::GetExternalRefResponse opaqueResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getExternalRef(request, opaqueResponse, &payloadInfo));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetExternalRefResponse: getExternalRef failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                if (jwtResponse.getPersonaReference().empty())
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetExternalRefResponse: jwtResponse.getPersonaReference().empty() is empty");
                    return Blaze::ERR_SYSTEM;
                }
                if (opaqueResponse.getPersonaReference().empty())
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetExternalRefResponse: opaqueResponse.getPersonaReference().empty() is empty");
                    return Blaze::ERR_SYSTEM;
                }

                int mismatchCount = 0;
                mismatchCount += compareField("ReferenceValue", jwtResponse.getPersonaReference()[0]->getReferenceValue(), opaqueResponse.getPersonaReference()[0]->getReferenceValue());

                if (mismatchCount != 0)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetExternalRefResponse: " << mismatchCount << " fields of JWT response do not match OPAQUE response");
                    return Blaze::ERR_SYSTEM;
                }

                return err;
            }

            BlazeRpcError compareGetPersonaInfoResponse(const char8_t* jwtToken, const char8_t* opaqueToken, const NucleusConnect::JwtPayloadInfo& payloadInfo, const Blaze::AccountId pidId, const Blaze::PersonaId personaId)
            {
                BlazeRpcError err = Blaze::ERR_SYSTEM;
                OAuth::OAuthSlaveImpl* oAuthSlave = mAuthComponent->oAuthSlaveImpl();
                InetAddress inetAddr = Authentication::AuthenticationUtil::getRealEndUserAddr(this);
                const char8_t* addrStr = inetAddr.getIpAsString();
                const char8_t* clientId = mAuthComponent->getBlazeServerNucleusClientId();

                OAuth::AccessTokenUtil tokenUtil;
                err = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetPersonaInfoResponse: Failed to retrieve server access token with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                NucleusIdentity::GetPersonaInfoRequest request;
                request.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
                request.getAuthCredentials().setUserAccessToken(jwtToken);
                request.getAuthCredentials().setClientId(clientId);
                request.getAuthCredentials().setIpAddress(addrStr);
                request.setPid(pidId);
                request.setPersonaId(personaId);

                NucleusIdentity::GetPersonaInfoResponse jwtResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getPersonaInfo(request, jwtResponse, &payloadInfo));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetPersonaInfoResponse: getPersonaInfo failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                request.getAuthCredentials().setUserAccessToken(opaqueToken);
                NucleusIdentity::GetPersonaInfoResponse opaqueResponse;
                err = Authentication::AuthenticationUtil::authFromOAuthError(oAuthSlave->getPersonaInfo(request, opaqueResponse, &payloadInfo));
                if (err != Blaze::ERR_OK)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetPersonaInfoResponse: getPersonaInfo failed with error(" << ErrorHelp::getErrorName(err) << ")");
                    return err;
                }

                int mismatchCount = 0;
                mismatchCount += compareField("PersonaId", jwtResponse.getPersona().getPersonaId(), opaqueResponse.getPersona().getPersonaId());
                mismatchCount += compareField("NamespaceName", jwtResponse.getPersona().getNamespaceName(), opaqueResponse.getPersona().getNamespaceName());
                mismatchCount += compareField("DisplayName", jwtResponse.getPersona().getDisplayName(), opaqueResponse.getPersona().getDisplayName());

                if (mismatchCount != 0)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareGetPersonaInfoResponse: " << mismatchCount << " fields of JWT response do not match OPAQUE response");
                    return Blaze::ERR_SYSTEM;
                }

                return err;
            }

            int compareField(const char8_t* fieldName, const char8_t* fieldValue1, const char8_t* fieldValue2)
            {
                if (blaze_strcmp(fieldValue1, fieldValue2) != 0)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareField: " << fieldName << "(" << fieldValue1 << ") does not match " << fieldName << "(" << fieldValue2 << ")");
                    return 1;
                }
                return 0;
            }

            int compareField(const char8_t* fieldName, const int64_t fieldValue1, const int64_t fieldValue2)
            {
                if (fieldValue1 != fieldValue2)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareField: " << fieldName << "(" << fieldValue1 << ") does not match " << fieldName << "(" << fieldValue2 << ")");
                    return 1;
                }
                return 0;
            }

            int compareField(const char8_t* fieldName, const bool fieldValue1, const bool fieldValue2)
            {
                if (fieldValue1 != fieldValue2)
                {
                    ERR_LOG("[CompareNucleusApiResponsesCommand].compareField: " << fieldName << "(" << fieldValue1 << ") does not match " << fieldName << "(" << fieldValue2 << ")");
                    return 1;
                }
                return 0;
            }

        };

        DEFINE_COMPARENUCLEUSAPIRESPONSES_CREATE()

    } //Arson
} //Blaze
