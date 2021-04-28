/*!
 * @file        thirdpartyoptinimpl.cpp
 * @brief       This is the implementation class for the thirdpartyoptin component's slave.
 * @copyright   Electronic Arts Inc.
 */

// Blaze includes
#include "framework/blaze.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/controller/controller.h"
#include "framework/oauth/accesstokenutil.h"
#include "component/authentication/rpc/authenticationslave.h"

// Component includes
#include "thirdpartyoptin/tdf/thirdpartyoptintypes_server.h"
#include "customcomponent/thirdpartyoptin/thirdpartyoptinslaveimpl.h"

#include "preferencecenter/rpc/preferencecenterslave.h"
#include "preferencecenter/tdf/preferencecenter.h"
#include "preferencecenter/tdf/preferencecenter_base.h"

namespace Blaze
{
namespace ThirdPartyOptIn
{

//=============================================================================
//! Static factory creator
ThirdPartyOptInSlave* ThirdPartyOptInSlave::createImpl()
{
    return BLAZE_NEW_NAMED("ThirdPartyOptInSlaveImpl") ThirdPartyOptInSlaveImpl();
}

//=============================================================================
//! Constructor
ThirdPartyOptInSlaveImpl::ThirdPartyOptInSlaveImpl()
{
}

//=============================================================================
//! Destructor
ThirdPartyOptInSlaveImpl::~ThirdPartyOptInSlaveImpl()
{
}

//=============================================================================
//! Returns the server's Nucleus access token
BlazeRpcError ThirdPartyOptInSlaveImpl::getServerNucleusAccessToken(eastl::string& accessToken)
{
    // Set the super user permission to make use of the scopes defined in authentication.cfg
    UserSession::SuperUserPermissionAutoPtr ptr(true);

    // Retrieve the access token for the server
    OAuth::AccessTokenUtil tokenUtil;

    BlazeRpcError tokErr = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
   
	if (tokErr == ERR_OK)
    {
        accessToken = tokenUtil.getAccessToken();
        TRACE_LOG(THIS_FUNC << "accessToken[" << accessToken.c_str() << "]");
    }
    else
    {
        ERR_LOG(THIS_FUNC << "retrieveServerAccessToken failed, tokErr[" << tokErr << "]");

        // Clear out the accessToken
        accessToken.clear();
    }

    return tokErr;
}

//=============================================================================

BlazeRpcError ThirdPartyOptInSlaveImpl::getCurrentUserIpAddress(const Command* command, eastl::string& userIpAddress)
{
	BlazeRpcError blazeRpcError = ERR_SYSTEM;
	const uint32_t clientIp = (gCurrentUserSession != nullptr) ? gCurrentUserSession->getExistenceData().getClientIp() : 0;

	if (clientIp != 0)
	{
		userIpAddress.sprintf("%d.%d.%d.%d", NIPQUAD(clientIp));
		blazeRpcError = ERR_OK;
	}
	else if (command != nullptr)
	{
		userIpAddress = Authentication::AuthenticationUtil::getRealEndUserAddr(command).getIpAsString();
		blazeRpcError = ERR_OK;
	}

	return blazeRpcError;
}

//=============================================================================

BlazeRpcError ThirdPartyOptInSlaveImpl::getAuthenticationCredentials(const Command* command, PreferenceCenter::AuthenticationCredentials& authenticationCredentials)
{
	BlazeRpcError blazeRpcError = ERR_SYSTEM;

	eastl::string accessToken;
	BlazeRpcError accessTokenBlazeRpcError = getServerNucleusAccessToken(accessToken);

	if (accessTokenBlazeRpcError == ERR_OK)
	{
		eastl::string userIpAddress;
		BlazeRpcError userIpAddressBlazeRpcError = getCurrentUserIpAddress(command, userIpAddress);

		if (userIpAddressBlazeRpcError == ERR_OK)
		{
			authenticationCredentials.setAccessToken(accessToken.c_str());
			authenticationCredentials.setIpAddress(userIpAddress.c_str());
			blazeRpcError = ERR_OK;
		}
		else
		{
			blazeRpcError = userIpAddressBlazeRpcError;
			ERR_LOG(THIS_FUNC << "getCurrentUserIpAddress failed, blazeRpcError[" << userIpAddressBlazeRpcError << "]");
		}
	}
	else
	{
		blazeRpcError = accessTokenBlazeRpcError;
		ERR_LOG(THIS_FUNC << "getServerNucleusAccessToken failed, blazeRpcError[" << accessTokenBlazeRpcError << "]");
	}

	return blazeRpcError;
}

//=============================================================================

BlazeRpcError ThirdPartyOptInSlaveImpl::getDoubleOptInData(TeamId teamId, LeagueId leagueId, bool& doubleOptInRequired)
{
	BlazeRpcError blazeRpcError = ERR_SYSTEM;

	if (gCurrentUserSession != nullptr)
	{
		uint32_t accountCountry = gCurrentUserSession->getAccountCountry();
		const ThirdPartyOptInConfig& config = getConfig();
		const CountryCodeList& countryCodeList = config.getDoubleOptInCountryCodeList();
		const LeagueIdList& leagueIdList = config.getDoubleOptInLeagueIdList();
		const TeamIdList& teamIdList = config.getDoubleOptInTeamIdList();
		doubleOptInRequired = false;

		CountryCodeList::const_iterator countryCodeListIter = countryCodeList.begin();
		CountryCodeList::const_iterator countryCodeListEndIter = countryCodeList.end();

		for (; countryCodeListIter != countryCodeListEndIter; ++countryCodeListIter)
		{
			const PreferenceCenter::CountryCode& countryCode = *countryCodeListIter;

			if (countryCode.length() >= 2)
			{
				uint32_t doubleOptCountry = LocaleTokenGetShortFromString(countryCode.c_str());

				if (doubleOptCountry != 0 && doubleOptCountry == accountCountry)
				{
					doubleOptInRequired = true;
					break;
				}
			}
			else
			{
				ERR_LOG(THIS_FUNC << "invalid countryCode[" << countryCode.c_str() << "]");
			}
		}

		if (!doubleOptInRequired)
		{
			auto leagueIdIter = leagueIdList.findAsSet(leagueId);

			if (leagueIdIter != leagueIdList.end())
			{
				doubleOptInRequired = true;
			}
			else
			{
				auto teamIdIter = teamIdList.findAsSet(teamId);

				if (teamIdIter != teamIdList.end())
				{
					doubleOptInRequired = true;
				}
			}
		}

		blazeRpcError = ERR_OK;
	}
	else
	{
		ERR_LOG(THIS_FUNC << "gCurrentUserSession is nullptr, blazeRpcError[" << blazeRpcError << "]");
	}

	return blazeRpcError;
}

//=============================================================================

BlazeRpcError ThirdPartyOptInSlaveImpl::getPreferenceId(const Command* command, TeamId teamId, LeagueId leagueId, PreferenceCenter::PreferenceId& preferenceId)
{
	BlazeRpcError blazeRpcError = ERR_SYSTEM;

	const ThirdPartyOptInConfig& config = getConfig();
	const EA::TDF::TdfString& preferenceNamePrefix = config.getPreferenceNamePrefixAsTdfString();

	if (!preferenceNamePrefix.empty())
	{
		PreferenceName preferenceName;
		preferenceName.sprintf("%s-%d-%d", preferenceNamePrefix.c_str(), leagueId, teamId);

		auto iter = mPreferenceIdMap.find(preferenceName);

		if (iter != mPreferenceIdMap.end())
		{
			preferenceId = iter->second;
			blazeRpcError = ERR_OK;
		}
		else
		{
			blazeRpcError = downloadPreferenceId(command, preferenceName, preferenceId);

			if (blazeRpcError == ERR_OK)
			{
				mPreferenceIdMap.insert(eastl::make_pair(preferenceName, preferenceId));
			}
			else
			{
				ERR_LOG(THIS_FUNC << "getPreferenceByName failed, blazeRpcError[" << blazeRpcError << "]");
			}
		}
	}

	return blazeRpcError;
}

//=============================================================================

BlazeRpcError ThirdPartyOptInSlaveImpl::downloadPreferenceId(const Command* command, const PreferenceName& preferenceName, PreferenceCenter::PreferenceId& preferenceId)
{
	BlazeRpcError blazeRpcError = ERR_SYSTEM;
	PreferenceCenter::PreferenceCenterError preferenceCenterError;
	PreferenceCenter::GetPreferenceByNameRequest request;
	PreferenceCenter::GetPreferenceByNameResponse response;

	PreferenceCenter::PreferenceCenterSlave* preferenceCenterSlave = (PreferenceCenter::PreferenceCenterSlave*)gOutboundHttpService->getService(PreferenceCenter::PreferenceCenterSlave::COMPONENT_INFO.name);

	if (preferenceCenterSlave != nullptr)
	{
		eastl::string accessToken;
		BlazeRpcError accessTokenBlazeRpcError = getServerNucleusAccessToken(accessToken);

		if (accessTokenBlazeRpcError == ERR_OK)
		{
			request.setAccessToken(accessToken.c_str());
			request.setName(preferenceName.c_str());

			BlazeRpcError preferenceByNameBlazeRpcError = preferenceCenterSlave->getPreferenceByName(request, response, preferenceCenterError);

			if (preferenceByNameBlazeRpcError == ERR_OK)
			{
				PreferenceCenter::PreferenceUriList& preferenceOrPreferenceUriList = response.getPreferenceOrPreferenceUri();

				if (!preferenceOrPreferenceUriList.empty())
				{
					PreferenceCenter::UriString& uriString = preferenceOrPreferenceUriList.front();
					eastl::string uri(uriString.c_str());
					uint64_t scannedValue = 0;

					int numScanned = parsePreferenceUriData(uri, scannedValue);

					if (numScanned == 1)
					{
						preferenceId = scannedValue;
						blazeRpcError = ERR_OK;
						TRACE_LOG(THIS_FUNC << "downloadPreferenceId[" << preferenceName.c_str() << "," << preferenceId << "]");
					}
					else
					{
						ERR_LOG(THIS_FUNC << "parsePreferenceUriData failed, uri " << uri.c_str());
					}
				}
				else
				{
					ERR_LOG(THIS_FUNC << "the list is empty");
				}
			}
			else
			{
				blazeRpcError = preferenceByNameBlazeRpcError;
				ERR_LOG(THIS_FUNC << "getPreferenceByName failed, blazeRpcError[" << blazeRpcError << "]");
			}
		}
		else
		{
			blazeRpcError = accessTokenBlazeRpcError;
			ERR_LOG(THIS_FUNC << "getServerNucleusAccessToken failed, blazeRpcError[" << accessTokenBlazeRpcError << "]");
		}
	}
	else
	{
		blazeRpcError = ERR_COMPONENT_NOT_FOUND;
		ERR_LOG(THIS_FUNC << "unable to get the PreferenceCenter component");
	}

	return blazeRpcError;
}

//=============================================================================

BlazeRpcError ThirdPartyOptInSlaveImpl::uploadAddedPreference(const Command* command, TeamId teamId, LeagueId leagueId)
{
	BlazeRpcError blazeRpcError = ERR_SYSTEM;
	PreferenceCenter::PreferenceCenterError preferenceCenterError;
	PreferenceCenter::PutPreferenceUserByPidIdRequest putPreferenceUserByPidIdRequest;
	PreferenceCenter::PutPreferenceUserByPidIdResponse putPreferenceUserByPidIdResponse;

	PreferenceCenter::PreferenceCenterSlave* preferenceCenterSlave = (PreferenceCenter::PreferenceCenterSlave*)gOutboundHttpService->getService(PreferenceCenter::PreferenceCenterSlave::COMPONENT_INFO.name);

	if (preferenceCenterSlave != nullptr)
	{
		PreferenceCenter::AuthenticationCredentials& authenticationCredentials = putPreferenceUserByPidIdRequest.getAuthenticationCredentials();

		BlazeRpcError authenticationCredentialsBlazeRpcError = getAuthenticationCredentials(command, authenticationCredentials);

		if (authenticationCredentialsBlazeRpcError == ERR_OK)
		{
			bool doubleOptInRequired = false;
			BlazeRpcError doubleOptInDataBlazeRpcError = getDoubleOptInData(teamId, leagueId, doubleOptInRequired);

			if (doubleOptInDataBlazeRpcError == ERR_OK)
			{
				putPreferenceUserByPidIdRequest.setIsDoubleOptInRequired(doubleOptInRequired);

				PreferenceCenter::PreferenceId preferenceId = 0;

				BlazeRpcError preferenceIdBlazeRpcError = getPreferenceId(command, teamId, leagueId, preferenceId);

				if (preferenceIdBlazeRpcError == ERR_OK)
				{
					PreferenceCenter::PutPreferenceUserByPidIdRequestData& requestData = putPreferenceUserByPidIdRequest.getRequestData();
					PreferenceCenter::PreferenceIdListData& preferenceIdListData = requestData.getAddedPreferences();
					PreferenceCenter::PreferenceIdList& preferenceIdList = preferenceIdListData.getPreferenceIds();
					preferenceIdList.push_back(preferenceId);

					if (gCurrentUserSession != nullptr)
					{
						PreferenceCenter::PidId pidId = gCurrentUserSession->getAccountId();
						putPreferenceUserByPidIdRequest.setPidId(pidId);
						bool underageUser = gCurrentUserSession->getSessionFlags().getIsChildAccount();

						if (!underageUser)
						{
							BlazeRpcError putPreferenceBlazeRpcError =
								preferenceCenterSlave->putPreferenceUserByPidId(putPreferenceUserByPidIdRequest, putPreferenceUserByPidIdResponse, preferenceCenterError);

							if (putPreferenceBlazeRpcError == ERR_OK)
							{
								blazeRpcError = ERR_OK;
							}
							else
							{
								blazeRpcError = putPreferenceBlazeRpcError;
								ERR_LOG(THIS_FUNC << "putPreferenceUserByPidId failed, blazeRpcError[" << putPreferenceBlazeRpcError << "]");
							}
						}
						else
						{
							ERR_LOG(THIS_FUNC << "getIsChildAccount check failed, blazeRpcError[" << blazeRpcError << "]");
						}
					}
					else
					{
						blazeRpcError = ERR_SYSTEM;
						ERR_LOG(THIS_FUNC << "gCurrentUserSession is nullptr, blazeRpcError[" << blazeRpcError << "]");
					}
				}
				else
				{
					blazeRpcError = preferenceIdBlazeRpcError;
					ERR_LOG(THIS_FUNC << "getPreferenceId failed, blazeRpcError[" << preferenceIdBlazeRpcError << "]");
				}
			}
			else
			{
				blazeRpcError = doubleOptInDataBlazeRpcError;
				ERR_LOG(THIS_FUNC << "getDoubleOptInData failed, blazeRpcError[" << doubleOptInDataBlazeRpcError << "]");
			}
		}
		else
		{
			blazeRpcError = authenticationCredentialsBlazeRpcError;
			ERR_LOG(THIS_FUNC << "getAuthenticationCredentials failed, blazeRpcError[" << authenticationCredentialsBlazeRpcError << "]");
		}
	}
	else
	{
		blazeRpcError = ERR_COMPONENT_NOT_FOUND;
		ERR_LOG(THIS_FUNC << "unable to get the PreferenceCenter component");
	}
		
	return blazeRpcError;
}

//=============================================================================

int ThirdPartyOptInSlaveImpl::parsePreferenceUriData(const eastl::string& uri, uint64_t& scannedValue)
{
	eastl::string::size_type index = uri.find_last_of("/\\");
	const char* source = nullptr;

	if (index == eastl::string::npos)
	{
		source = uri.c_str();
	}
	else if (index + 1 < uri.size())
	{
		source = &uri.at(index + 1);
	}

	int numScanned = 0;
	
	if (source != nullptr)
	{
		numScanned = EA::StdC::Sscanf(source, "%" SCNu64, &scannedValue);
	}

	return numScanned;
}

//=============================================================================
//! ComponentStub method. Called when the component is configured.
bool ThirdPartyOptInSlaveImpl::onConfigure()
{
    TRACE_LOG(THIS_FUNC << "configuring component");
    return true;
}

} // ThirdPartyOptIn
} // Blaze
