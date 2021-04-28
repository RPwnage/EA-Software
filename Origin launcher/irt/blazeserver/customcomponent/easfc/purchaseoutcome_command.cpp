/*************************************************************************************************/
/*!
    \file   purchaseoutcome_command.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/easfc/purchaseoutcome_command.cpp#1 $

    \attention
    (c) Electronic Arts Inc. 2012
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"
#include "framework/database/dbscheduler.h"
#include "framework/controller/controller.h"

#include "purchaseoutcome_extension.h"

#include "component/stats/updatestatshelper.h"
#include "component/stats/updatestatsrequestbuilder.h"

// easfc includes
#include "easfcslaveimpl.h"
#include "easfc/tdf/easfctypes.h"
#include "easfc/rpc/easfcslave/purchasegamewin_stub.h"
#include "easfc/rpc/easfcslave/purchasegameloss_stub.h"
#include "easfc/rpc/easfcslave/purchasegamedraw_stub.h"

namespace Blaze
{
    namespace EASFC
    {
		/*===============================================================================================
			PurchaseOutcomeHelper
		===============================================================================================*/
		class PurchaseOutcomeHelper
		{
		public:
			PurchaseOutcomeHelper()
				: mSeasonalPlayExtension(NULL)
			{
			}

			virtual ~PurchaseOutcomeHelper()
			{
				delete mSeasonalPlayExtension;
			}

			Blaze::BlazeRpcError ExecuteOutcomePurchase(MemberId memberId, MemberType memberType, Blaze::GameReporting::IFifaSeasonalPlayExtension::GameResult gameResult)
			{
				Blaze::BlazeRpcError error = Blaze::ERR_OK;

				if(NULL != mSeasonalPlayExtension)
				{
					delete mSeasonalPlayExtension;
				}				

				switch(memberType)
				{
				case EASFC_MEMBERTYPE_USER:
					{
						mSeasonalPlayExtension = BLAZE_NEW SeasonPurchaseOutcomeExtension();						
					}
					break;
				case EASFC_MEMBERTYPE_COOP_PAIR:
					{
						mSeasonalPlayExtension = BLAZE_NEW CoopSeasonPurchaseOutcomeExtension();
					}
					break;
				case EASFC_MEMBERTYPE_CLUB:
						mSeasonalPlayExtension = BLAZE_NEW ProClubPurchaseOutcomeExtension();
					break;
				default:
					{
						error = Blaze::EASFC_ERR_INVALID_PARAMS;
						TRACE_LOG("[PurchaseOutcomeHelper].ExecuteOutcomePurchase(): ERROR! invalid MemberType");
					}					
					break;
				}

				if( Blaze::ERR_OK != error )
				{
					return error;
				}

				if( NULL == mSeasonalPlayExtension )
				{
					return Blaze::ERR_SYSTEM;
				}

				
				Blaze::Stats::UpdateStatsRequestBuilder builder;
				Blaze::Stats::UpdateStatsHelper updateStatsHelper;

				//----------------------------------------------------------------------------------
				// Set up the SeasonPlay logic for processing the outcome
				//----------------------------------------------------------------------------------
				SeasonPurchaseOutcomeExtension::ExtensionData data;
				data.mEntityId = memberId;
				data.mGameResult = gameResult;

				mSeasonalPlayExtension->setExtensionData(&data);
				mSeasonalPlay.setExtension(mSeasonalPlayExtension);
				mSeasonalPlay.initialize(&builder, &updateStatsHelper, NULL);

				//----------------------------------------------------------------------------------
				// Set up the initial selects for the stats we want to update
				//----------------------------------------------------------------------------------
				mSeasonalPlay.selectSeasonalPlayStats();

				bool strict = true;
				error = updateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)builder, strict);
				if( Blaze::ERR_OK != error )
				{
					return error;
				}

				//----------------------------------------------------------------------------------
				// Perform the fetching of the select statements
				//----------------------------------------------------------------------------------
				error = updateStatsHelper.fetchStats();
				if( Blaze::ERR_OK != error )
				{
					return error;
				}

				//----------------------------------------------------------------------------------
				// Execute the logic to update the stats and construct update SQL instructions
				//----------------------------------------------------------------------------------				
				mSeasonalPlay.transformSeasonalPlayStats();
				mSeasonalPlay.updateDivCounts();

				if (memberType == EASFC_MEMBERTYPE_USER) //We only want the redeem functionality for seasons
				{
					Stats::UpdateRowKey* key = builder.getUpdateRowKey("SPDivisionalPlayerStats", memberId);
					Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key->period);

					switch (gameResult)
					{
					case Blaze::GameReporting::IFifaSeasonalPlayExtension::WIN:
						updateStatsHelper.setValueInt(key, STATNAME_WINREDEEMED, periodType, EasfcSlaveImpl::SeasonsItemState::ITEM_REDEEMED);
						break;
					case Blaze::GameReporting::IFifaSeasonalPlayExtension::TIE:
						updateStatsHelper.setValueInt(key, STATNAME_DRAWREDEEMED, periodType, EasfcSlaveImpl::SeasonsItemState::ITEM_REDEEMED);
						break;
					default:
						break;
					}
				}

				//----------------------------------------------------------------------------------
				// Perform all update SQL instructions
				//----------------------------------------------------------------------------------
				error = updateStatsHelper.commitStats();
				
				return error;
			}

		private:
			Blaze::GameReporting::FifaSeasonalPlay				mSeasonalPlay;
			Blaze::GameReporting::IFifaSeasonalPlayExtension*	mSeasonalPlayExtension;
		};

		/*===============================================================================================
			PurchaseGameWinCommand
		===============================================================================================*/
        class PurchaseGameWinCommand : public PurchaseGameWinCommandStub  
        {
        public:
            PurchaseGameWinCommand(Message* message, PurchaseGameRequest* request, EasfcSlaveImpl* componentImpl)
                : PurchaseGameWinCommandStub(message, request), mComponent(componentImpl)				
            { }

            virtual ~PurchaseGameWinCommand()
			{ }
        
        private:

            PurchaseGameWinCommand::Errors execute()
            {
				TRACE_LOG("[PurchaseGameWinCommand].execute()");
				Blaze::BlazeRpcError error = mPurchaseOutcomeHelper.ExecuteOutcomePurchase(mRequest.getMemberId(), mRequest.getMemberType(), Blaze::GameReporting::IFifaSeasonalPlayExtension::WIN);
                return PurchaseGameWinCommand::commandErrorFromBlazeError(error);
            }

        private:            
            EasfcSlaveImpl* mComponent; // Not owned memory.
			PurchaseOutcomeHelper mPurchaseOutcomeHelper;
        };


        // static factory method impl
        PurchaseGameWinCommandStub* PurchaseGameWinCommandStub::create(Message* message, PurchaseGameRequest* request, EasfcSlave* componentImpl)
        {
            return BLAZE_NEW PurchaseGameWinCommand(message, request, static_cast<EasfcSlaveImpl*>(componentImpl));
        }

		/*===============================================================================================
			PurchaseGameLossCommand
		===============================================================================================*/
		class PurchaseGameLossCommand : public PurchaseGameLossCommandStub  
		{
		public:
			PurchaseGameLossCommand(Message* message, PurchaseGameRequest* request, EasfcSlaveImpl* componentImpl)
				: PurchaseGameLossCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~PurchaseGameLossCommand()
			{ }

		private:

			PurchaseGameLossCommand::Errors execute()
			{
				TRACE_LOG("[PurchaseGameLossCommand].execute()");
				Blaze::BlazeRpcError error = mPurchaseOutcomeHelper.ExecuteOutcomePurchase(mRequest.getMemberId(), mRequest.getMemberType(), Blaze::GameReporting::IFifaSeasonalPlayExtension::LOSS);
				return PurchaseGameLossCommand::commandErrorFromBlazeError(error);
			}

		private:			
			EasfcSlaveImpl* mComponent; // Not owned memory.
			PurchaseOutcomeHelper mPurchaseOutcomeHelper;
		};


		// static factory method impl
		PurchaseGameLossCommandStub* PurchaseGameLossCommandStub::create(Message* message, PurchaseGameRequest* request, EasfcSlave* componentImpl)
		{
			return BLAZE_NEW PurchaseGameLossCommand(message, request, static_cast<EasfcSlaveImpl*>(componentImpl));
		}

		/*===============================================================================================
			PurchaseGameDrawCommand
		===============================================================================================*/
		class PurchaseGameDrawCommand : public PurchaseGameDrawCommandStub  
		{
		public:
			PurchaseGameDrawCommand(Message* message, PurchaseGameRequest* request, EasfcSlaveImpl* componentImpl)
				: PurchaseGameDrawCommandStub(message, request), mComponent(componentImpl)
			{ }

			virtual ~PurchaseGameDrawCommand()
			{ }

		private:

			PurchaseGameDrawCommand::Errors execute()
			{
				TRACE_LOG("[PurchaseGameDrawCommand].execute()");
				Blaze::BlazeRpcError error = mPurchaseOutcomeHelper.ExecuteOutcomePurchase(mRequest.getMemberId(), mRequest.getMemberType(), Blaze::GameReporting::IFifaSeasonalPlayExtension::TIE);
				return PurchaseGameDrawCommand::commandErrorFromBlazeError(error);
			}

		private:			
			EasfcSlaveImpl* mComponent; // Not owned memory.
			PurchaseOutcomeHelper mPurchaseOutcomeHelper;
		};


		// static factory method impl
		PurchaseGameDrawCommandStub* PurchaseGameDrawCommandStub::create(Message* message, PurchaseGameRequest* request, EasfcSlave* componentImpl)
		{
			return BLAZE_NEW PurchaseGameDrawCommand(message, request, static_cast<EasfcSlaveImpl*>(componentImpl));
		}

    } // EASFC
} // Blaze
