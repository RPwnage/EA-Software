/*************************************************************************************************/
/*!
    \file   osdkclubshelperslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class OSDKClubsHelperSlaveImpl

    OSDKClubsHelper Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkclubshelperslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// osdkclubshelper includes
#include "osdkclubshelper/tdf/osdkclubshelpertypes.h"

#include "framework/config/config_sequence.h"
 
#include "framework/util/locales.h"  
#include "clubs/rpc/clubsslave.h"


namespace Blaze
{
namespace OSDKClubsHelper
{

// static
OSDKClubsHelperSlave* OSDKClubsHelperSlave::createImpl()
{
    return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "OSDKClubsHelperSlaveImpl") OSDKClubsHelperSlaveImpl();
}

/*** Public Methods ******************************************************************************/
OSDKClubsHelperSlaveImpl::OSDKClubsHelperSlaveImpl():
    mConfigMap(NULL)
{
}

OSDKClubsHelperSlaveImpl::~OSDKClubsHelperSlaveImpl()
{
}


/*************************************************************************************************/
/*!
    \brief onConfigure

    Load and parses configuration file.

*/
/*************************************************************************************************/
bool OSDKClubsHelperSlaveImpl::onConfigure()
{
    bool success = configureHelper();

    //Register our callback hook for the updateClubSettings command, with the clubs slave.
    BlazeRpcError waitResult = ERR_OK;
    Blaze::Clubs::ClubsSlave* clubsSlave = static_cast<Blaze::Clubs::ClubsSlave*>(gController->getComponent(Clubs::ClubsSlave::COMPONENT_ID, false, true, &waitResult));
    
    if (ERR_OK != waitResult)
    {
        ERR_LOG("[OSDKClubsHelperSlaveImpl:" << this << "].onConfigure - Clubs component not found.");
        success = false;
    }
    else if (NULL != clubsSlave)
    {
        Blaze::Clubs::ClubsSlave::UpdateClubSettingsRequestHookCb hookCb(
            this, &Blaze::OSDKClubsHelper::OSDKClubsHelperSlaveImpl::updateClubSettingsRequestHookCallback);
        clubsSlave->setupdateClubSettingsRequestHook(hookCb);
    }
    else
    {
        ERR_LOG("[OSDKClubsHelperSlaveImpl:" << this << "].onConfigure - Clubs component does not exist.");
        success = false;
    }

    TRACE_LOG("[OSDKClubsHelperSlaveImpl:" << this << "].onConfigure: component configuration " << (success ? "successful." : "failed."));

    return success;
}

/*************************************************************************************************/
/*!
    \brief onReconfigure

    Called when the server is being reloaded and the component needs to re-read it's configuration

*/
/*************************************************************************************************/
bool OSDKClubsHelperSlaveImpl::onReconfigure()
{
    mValidationConfig.mValidTeamsSet.clear();
    mValidationConfig.mValidRegions.clear();
    mValidationConfig.mValidLanguages.clear();

    bool success = configureHelper();
 
    TRACE_LOG("[OSDKClubsHelperSlaveImpl:" << this << "].onReconfigure: reconfiguration " << (success ? "successful." : "failed."));
    return success;
}

/*************************************************************************************************/
/*!
    \brief configureHelper

    Helper function which performs tasks common to onConfigure and onReconfigure

*/
/*************************************************************************************************/
bool OSDKClubsHelperSlaveImpl::configureHelper()
{
    const OSDKClubsHelperConfig& config = getConfig();

    // validation
    ConfigValidTeamIdList::const_iterator teamIdIt = config.getValidation().getValidTeamIds().begin();
    ConfigValidTeamIdList::const_iterator teamIdItEnd = config.getValidation().getValidTeamIds().end();
    for (; teamIdIt != teamIdItEnd; ++teamIdIt)
    {
        uint32_t teamId = *teamIdIt;
        INFO_LOG("[OSDKClubsHelperSlaveImpl:" << this << "].configureHelper: adding valid team id: " << teamId << "");
        mValidationConfig.mValidTeamsSet.insert(teamId);
    }

    ConfigValidLanguageList::const_iterator languageIt = config.getValidation().getValidLanguages().begin();
    ConfigValidLanguageList::const_iterator languageItEnd = config.getValidation().getValidLanguages().end();
    if (languageIt == languageItEnd)
    {
        WARN_LOG("[OSDKClubsHelperSlaveImpl:" << this << "].configureHelper: no valid languages specified.");
    }
    for (; languageIt != languageItEnd; ++languageIt)
    {
        const char8_t* strLanguage = *languageIt;
        if ('\0' == (*strLanguage))
        {
            WARN_LOG("[OSDKClubsHelperSlaveImpl:" << this << "].configureHelper: empty valid language.");
        }
        else
        {
            INFO_LOG("[OSDKClubsHelperSlaveImpl:" << this << "].configureHelper: adding valid language: " << strLanguage << "");
            uint16_t langAsInt = LocaleTokenGetShortFromString(strLanguage);
            mValidationConfig.mValidLanguages.push_back(langAsInt);
        }
    }

    ConfigValidRegionList::const_iterator regionIt = config.getValidation().getValidRegions().begin();
    ConfigValidRegionList::const_iterator regionItEnd = config.getValidation().getValidRegions().end();
    if (regionIt == regionItEnd)
    {
        WARN_LOG("[OSDKClubsHelperSlaveImpl:" << this << "].configureHelper: no valid regions specified.");
    }
    for (; regionIt != regionItEnd; ++regionIt)
    {
        uint32_t region = *regionIt;
        INFO_LOG("[OSDKClubsHelperSlaveImpl:" << this << "].configureHelper: adding valid region: " << region << "");
        mValidationConfig.mValidRegions.push_back(region);
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief updateClubSettingsRequestHookCallback

    Callback for validation of the club updateClubSettings Command. 
    Will check for valid teams, region and language, as read in from config files.
*/
/*************************************************************************************************/
void OSDKClubsHelperSlaveImpl::updateClubSettingsRequestHookCallback(Blaze::Clubs::UpdateClubSettingsRequest& request, BlazeRpcError& blazeError, bool& executeRpc)
{
    executeRpc = true; //by default, all is good, so carry on..
    blazeError = ERR_OK;
    TRACE_LOG("[OSDKClubsHelperSlaveImpl:" << this << "].updateClubSettingsRequestHookCallback() called.");
    if (!isValidTeam(request.getClubSettings().getTeamId()))
    {
        ERR_LOG("[OSDKClubsHelperSlaveImpl::UpdateClubSettingsCommand:" << this << "] got bad TeamId: " << request.getClubSettings().getTeamId());
        blazeError = CLUBS_ERR_INVALID_ARGUMENT;
        executeRpc = false;
        return;
    }

    uint16_t langAsInt = LocaleTokenGetShortFromString(request.getClubSettings().getLanguage());

    const OSDKClubsHelper::ValidSettingsContainer& langs = mValidationConfig.mValidLanguages;
    if (!isValidSetting(langs, langAsInt))
    {
        ERR_LOG("[OSDKClubsHelperSlaveImpl::UpdateClubSettingsCommand:" << this << "] got bad Language: ["
                << request.getClubSettings().getLanguage() << "] aka [" << langAsInt << "]");
        blazeError = CLUBS_ERR_INVALID_ARGUMENT;
        executeRpc = false;
        return;
    }

    if (!isValidSetting(mValidationConfig.mValidRegions, request.getClubSettings().getRegion()))
    {
        ERR_LOG("[OSDKClubsHelperSlaveImpl::UpdateClubSettingsCommand:" << this << "] got bad Region: " << request.getClubSettings().getRegion());
        blazeError = CLUBS_ERR_INVALID_ARGUMENT;
        executeRpc = false;
        return;
    }
}

/*************************************************************************************************/
/*!
    \brief isValidTeam

     Check for a valid Team Id, based on values read from config.
*/
/*************************************************************************************************/
bool OSDKClubsHelperSlaveImpl::isValidTeam(uint32_t teamId)
{
    const OSDKClubsHelper::ValidTeamsSet& teams = mValidationConfig.mValidTeamsSet;
    if (teams.empty())
    { 
        //<assuming no validation>
        TRACE_LOG("[OSDKClubsHelperSlaveImpl:isValidTeam] teams config is empty, assuming no validation");
        return true;
    }

    if (teams.find(teamId) != teams.end())
    {
        return true;
    }

    TRACE_LOG("[OSDKClubsHelperSlaveImpl:isValidTeam] bad team received: " << teamId << "");
    return false;
}

/*************************************************************************************************/
/*!
    \brief isValidSetting

     Checks that a setting value is contained in the map.
*/
/*************************************************************************************************/
bool OSDKClubsHelperSlaveImpl::isValidSetting(const OSDKClubsHelper::ValidSettingsContainer& settings, uint32_t value)
{
    if (settings.empty())
    { 
        TRACE_LOG("[OSDKClubsHelperSlaveImpl:isValidSetting] setting config is empty, assuming no validation, value=[" << value << "]");
        return true;
    }

    OSDKClubsHelper::ValidSettingsContainer::const_iterator itr = settings.begin();
    OSDKClubsHelper::ValidSettingsContainer::const_iterator end = settings.end();

    for (; itr != end; ++itr)
    {
        if (*itr == value)
        {
            return true;
        }
    }

    TRACE_LOG("[OSDKClubsHelperSlaveImpl:isValidSetting] bad setting value received: " << value << "");
    return false;
}


} // OSDKClubsHelper
} // Blaze
