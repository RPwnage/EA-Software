/*************************************************************************************************/
/*!
    \file   getseasonconfiguration_command.cpp

    $Header: //gosdev/games/FIFA/2022/GenX/irt/blazeserver/customcomponent/osdkseasonalplay/getseasonconfiguration_command.cpp#1 $
    $Change: 1646537 $
    $DateTime: 2021/02/01 04:49:23 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetSeasonConfigurationCommand

    Retrieves the seasonal play instance configuration

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/getseasonconfiguration_stub.h"

// global includes

// framework includes
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        class GetSeasonConfigurationCommand : public GetSeasonConfigurationCommandStub  
        {
        public:
            GetSeasonConfigurationCommand(Message* message, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : GetSeasonConfigurationCommandStub(message), mComponent(componentImpl)
            { }

            virtual ~GetSeasonConfigurationCommand() { }

        /* Private methods *******************************************************************************/
        private:

            GetSeasonConfigurationCommandStub::Errors execute()
            {
                TRACE_LOG("[GetSeasonConfigurationCommand:" << this << "].execute()");

                Blaze::BlazeRpcError error = Blaze::ERR_OK;

                const OSDKSeasonalPlaySlaveImpl::SeasonConfigurationServerMap *pConfigMap = &mComponent->getSeasonConfigurationMap();
                
                if (pConfigMap->size() > 0)
                {
                    OSDKSeasonalPlaySlaveImpl::SeasonConfigurationServerMap::const_iterator itr = pConfigMap->begin();
                    OSDKSeasonalPlaySlaveImpl::SeasonConfigurationServerMap::const_iterator itr_end = pConfigMap->end();

                    for( ; itr != itr_end; ++itr)
                    {
                        const SeasonConfigurationServer *pSeason = itr->second;

                        SeasonConfiguration *pResponseSeason = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "SeasonConfiguration") SeasonConfiguration();

                        pResponseSeason->setSeasonId(pSeason->getSeasonId());
                        pResponseSeason->setMemberType(pSeason->getMemberType());
                        pResponseSeason->setLeagueId(pSeason->getLeagueId());
                        pResponseSeason->setTournamentId(pSeason->getTournamentId());
                        pResponseSeason->setLeaderboardName(pSeason->getLeaderboardName());
                        pResponseSeason->setStatPeriodtype(pSeason->getStatPeriodtype());
                        pResponseSeason->setStatCategoryName(pSeason->getStatCategoryName());
                        pSeason->getDivisions().copyInto(pResponseSeason->getDivisions());

                        mResponse.getInstanceConfigList().push_back(pResponseSeason);
                    }
                }
                else
                {
                    error = Blaze::OSDKSEASONALPLAY_ERR_CONFIGURATION_ERROR;
                }

                return GetSeasonConfigurationCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        GetSeasonConfigurationCommandStub* GetSeasonConfigurationCommandStub::create(Message* message, OSDKSeasonalPlaySlave* componentImpl)
        {
            return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "GetSeasonConfigurationCommand") GetSeasonConfigurationCommand(message, static_cast<OSDKSeasonalPlaySlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
