/*!
 * @file        thirdpartyoptinslaveimpl.h
 * @brief       This is the implementation class for the thirdpartyoptin component's slave.
 * @copyright   Electronic Arts Inc.
 */

#ifndef BLAZE_THIRDPARTYOPTIN_SLAVEIMPL_H
#define BLAZE_THIRDPARTYOPTIN_SLAVEIMPL_H

 // Blaze includes
#include "authentication/tdf/accountdefines.h"
#include "preferencecenter/tdf/preferencecenter.h"

// Component includes
#include "thirdpartyoptin/rpc/thirdpartyoptinslave_stub.h"

namespace Blaze
{
namespace ThirdPartyOptIn
{
	typedef eastl::string PreferenceName;
	typedef eastl::map<PreferenceName, PreferenceCenter::PreferenceId> PreferenceIdMap;

    //! This is the implementation class for the thirdpartyoptin component's slave.
    class ThirdPartyOptInSlaveImpl : public ThirdPartyOptInSlaveStub
    {
    public:
        //! Constructor
        ThirdPartyOptInSlaveImpl();

        //! Destructor
        virtual ~ThirdPartyOptInSlaveImpl();

        //! Returns the server's Nucleus access token
        BlazeRpcError getServerNucleusAccessToken(eastl::string& accessToken);

		//! Returns the current user's IP Address
		BlazeRpcError getCurrentUserIpAddress(const Command* command, eastl::string& userIpAddress);

		//! Prepare the auth data for HTTP requests
		BlazeRpcError getAuthenticationCredentials(const Command* command, PreferenceCenter::AuthenticationCredentials& authenticationCredentials);

		//! Prepares the Double Opt In data for HTTP requests
		BlazeRpcError getDoubleOptInData(TeamId teamId, LeagueId leagueId, bool& doubleOptInRequired);

		//! Returns the PreferenceId
		BlazeRpcError getPreferenceId(const Command* command, TeamId teamId, LeagueId leagueId, PreferenceCenter::PreferenceId& preferenceId);
	
		//! Returns the PreferenceId, downloaded from the Preference Center
		BlazeRpcError downloadPreferenceId(const Command* command, const PreferenceName& preferenceName, PreferenceCenter::PreferenceId& preferenceId);

		//! Prepares the HTTP data for the Preference Center component and then requests the HTTP operation
		BlazeRpcError uploadAddedPreference(const Command* command, TeamId teamId, LeagueId leagueId);

	private:

        //! ComponentStub method. Called when the component is configured.
        bool onConfigure();

		//! Converts the json response object data into integers
		static int parsePreferenceUriData(const eastl::string& uri, uint64_t& scannedValue);

		//! Downloaded PreferenceIds.
		PreferenceIdMap	mPreferenceIdMap;
    };

} // namespace ThirdPartyOptIn
} // namespace Blaze

#endif // BLAZE_THIRDPARTYOPTIN_SLAVEIMPL_H
