/*************************************************************************************************/
/*!
    \file   statsextendeddataprovider.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
/*************************************************************************************************/
/*!
    \class StatsExtendedDataProvider
    Manages Stats component extended data providers
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "stats/tdf/stats_server.h"
#include "statscomponentinterface.h"
#include "statsconfig.h"
#include "statsextendeddataprovider.h"
#include "userstats.h"
#include "userranks.h"

#include "EAStdC/EAString.h"

namespace Blaze
{
namespace Stats
{
    StatsExtendedDataProvider::StatsExtendedDataProvider(StatsSlaveImpl& component) 
        : mComponent(component),
        mUserRanks(component),
        mUserStats(component)
    {
    }

    /*************************************************************************************************/
    /*!
        \brief onLoadExtendedData
        Updates user stats in the user extended data map
        \param[in]  data
        \param[in]  extendedData
        \return - none
    */
    /*************************************************************************************************/
    BlazeRpcError StatsExtendedDataProvider::onLoadExtendedData(const UserInfoData &data, UserSessionExtendedData &extendedData)
    {
        BlazeRpcError err = mUserStats.onLoadExtendedData(data, extendedData);
        if (err == ERR_OK)
            err = mUserRanks.onLoadExtendedData(data, extendedData);
        return err;
    }

    /*************************************************************************************************/
    /*!
        \brief parseExtendedData
        Parses user stats and user ranks. Enforces uniqueness of extended data ids.
        \param[in]  config - stats config file input to be parsed
        \param[in]  statsConfig - the translated stats config data 
        \return - true if parsing successful, false otherwise
    */
    /*************************************************************************************************/
    bool StatsExtendedDataProvider::parseExtendedData(const StatsConfig& config, StatsConfigData* statsConfig)
    {
        ConfigureValidationErrors validationErrors;
        UserExtendedDataIdMap uedIdMap;

        TRACE_LOG("[StatsExtendedDataProvider].parseExtendedData(): Processing user extended data.");
        TRACE_LOG("[StatsExtendedDataProvider].parseExtendedData(): Processing user stats.");
        if(!mUserStats.parseUserStats(config, mExtendedDataKeyMap, statsConfig, uedIdMap, validationErrors))
        {
            eastl::string msg;
            msg.sprintf("[StatsExtendedDataProvider].parseExtendedData(): Error processing user stats.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());

            for (ConfigureValidationErrors::StringValidationErrorMessageList::iterator it = validationErrors.getErrorMessages().begin();
                it != validationErrors.getErrorMessages().end(); ++it)
            {
                ERR_LOG(it->c_str());
            }      

            return false;
        }

        TRACE_LOG("[StatsExtendedDataProvider].parseExtendedData(): Processing user ranks.");
        if(!mUserRanks.parseUserRanks(config, mExtendedDataKeyMap, statsConfig, uedIdMap, validationErrors))
        {
            eastl::string msg;
            msg.sprintf("[StatsExtendedDataProvider].parseExtendedData(): Error processing user ranks.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());

            for (ConfigureValidationErrors::StringValidationErrorMessageList::iterator it = validationErrors.getErrorMessages().begin();
                it != validationErrors.getErrorMessages().end(); ++it)
            {
                ERR_LOG(it->c_str());
            }      

            return false;
        }

        // Update the User Session Extended Ids so other components can refer to the stats
        // stats puts in the User Extended Data by name instead of by key.
        BlazeRpcError rc = gUserSessionManager->updateUserSessionExtendedIds(uedIdMap);
        return (rc == Blaze::ERR_OK);
    }

    bool StatsExtendedDataProvider::lookupExtendedData(const UserSessionMaster& session, const char8_t* key, UserExtendedDataValue& value) const
    { 
        //stats will be looked up by id
        int16_t mapKey = (int16_t)EA::StdC::AtoI32(key);
        ExtendedDataKeyMap::const_iterator keyMapIter = mExtendedDataKeyMap.find(mapKey);
        if(keyMapIter != mExtendedDataKeyMap.end())
        {
            switch(keyMapIter->second)
            {
                case USER_STAT:
                    return mUserStats.lookupExtendedData(session, key, value);
                case USER_RANK:
                    return mUserRanks.lookupExtendedData(session, key, value);
                default:
                    return false;
            }
        }

        return false;
    }

    BlazeRpcError StatsExtendedDataProvider::onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData)
    {
        BlazeRpcError err = mUserStats.onRefreshExtendedData(data, extendedData);
        if (err == ERR_OK)
            mUserRanks.onRefreshExtendedData(data, extendedData);
        return err;
    }

}
}
