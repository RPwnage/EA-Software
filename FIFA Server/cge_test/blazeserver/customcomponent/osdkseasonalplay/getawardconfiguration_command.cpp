/*************************************************************************************************/
/*!
    \file   getawardconfiguration_command.cpp

    $Header$
    $Change$
    $DateTime$

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetAwardConfigurationCommand

    Retrieves the seasonal play award configuration

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/rpc/osdkseasonalplayslave/getawardconfiguration_stub.h"

// global includes

// framework includes
#include "framework/connection/selector.h"
#include "framework/identity/identity.h"
#include "framework/util/localization.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        class GetAwardConfigurationCommand : public GetAwardConfigurationCommandStub  
        {
        public:
            GetAwardConfigurationCommand(Message* message, OSDKSeasonalPlaySlaveImpl* componentImpl)
                : GetAwardConfigurationCommandStub(message), mComponent(componentImpl)
            { }

            virtual ~GetAwardConfigurationCommand() { }

        /* Private methods *******************************************************************************/
        private:

            GetAwardConfigurationCommandStub::Errors execute()
            {
                TRACE_LOG("[GetAwardConfigurationCommand:" << this << "].execute()");

                Blaze::BlazeRpcError error = Blaze::ERR_OK;

                uint32_t uLocale = getPeerInfo()->getLocale();

                const OSDKSeasonalPlaySlaveImpl::AwardConfigurationMap *pConfigMap = &mComponent->getAwardConfigurationMap();
                
                if (pConfigMap->size() > 0)
                {
                    OSDKSeasonalPlaySlaveImpl::AwardConfigurationMap::const_iterator itr = pConfigMap->begin();
                    OSDKSeasonalPlaySlaveImpl::AwardConfigurationMap::const_iterator itr_end = pConfigMap->end();

                    for( ; itr != itr_end; ++itr)
                    {
                        const AwardConfiguration *pAward = itr->second;

                        AwardConfiguration *pResponseAward = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "AwardConfiguration") AwardConfiguration();

                        pResponseAward->setAwardId(pAward->getAwardId());
                        pResponseAward->setName(gLocalization->localize(pAward->getName(), uLocale));
                        pResponseAward->setDescription(gLocalization->localize(pAward->getDescription(), uLocale));
                        pResponseAward->setAssetPath(pAward->getAssetPath());

                        mResponse.getAwards().push_back(pResponseAward);
                    }
                }
                else
                {
                    error = Blaze::OSDKSEASONALPLAY_ERR_CONFIGURATION_ERROR;
                }

                return GetAwardConfigurationCommand::commandErrorFromBlazeError(error);
            }

        private:
            // Not owned memory.
            OSDKSeasonalPlaySlaveImpl* mComponent;
        };


        // static factory method impl
        GetAwardConfigurationCommandStub* GetAwardConfigurationCommandStub::create(Message* message, OSDKSeasonalPlaySlave* componentImpl)
        {
            return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "GetAwardConfigurationCommand") GetAwardConfigurationCommand(message, static_cast<OSDKSeasonalPlaySlaveImpl*>(componentImpl));
        }

    } // OSDKSeasonalPlay
} // Blaze
