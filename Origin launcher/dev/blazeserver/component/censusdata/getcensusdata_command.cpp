/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "censusdata/rpc/censusdataslave/getlatestcensusdata_stub.h"
#include "censusdata/censusdataslaveimpl.h"

namespace Blaze
{
namespace CensusData
{

    class GetLatestCensusDataCommand : public GetLatestCensusDataCommandStub
    {
    public:
        GetLatestCensusDataCommand(Message* message, CensusDataSlaveImpl* componentImpl)
            : GetLatestCensusDataCommandStub(message), 
              mComponent(componentImpl)
        {
        }

        ~GetLatestCensusDataCommand() override
        {
        }

        /* Private methods *******************************************************************************/
    private:

        GetLatestCensusDataCommandStub::Errors execute() override
        {          
            ClientPlatformType platform = gCurrentUserSession ? gCurrentUserSession->getClientPlatform() : INVALID;
            if (platform == INVALID)
                return ERR_SYSTEM;

            Blaze::CensusDataByComponent censusDataByComponent;
            mComponent->fetchRemoteCensusData(censusDataByComponent);

            BlazeRpcError err = gCensusDataManager->populateCensusData(censusDataByComponent);
            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[GetLatestCensusDataCommand].execute(): populateCensusData got error " << (ErrorHelp::getErrorName(err)) 
                         << ", line:" << __LINE__ << "!");  
                return commandErrorFromBlazeError(err);
            }
            
            NotifyServerCensusDataList& nsList = mResponse.getCensusDataList();
            nsList.reserve(censusDataByComponent.size());

            for (auto curCensusByComp : censusDataByComponent)
            {
                NotifyServerCensusDataItem* nsCensusData = nsList.pull_back();
                nsCensusData->setTdf(*curCensusByComp.second->get());
            }
            censusDataByComponent.clear();

            return commandErrorFromBlazeError(err);
        }

    private:
        CensusDataSlaveImpl* mComponent;  // memory owned by creator, don't free
    };

    // static factory method impl
    DEFINE_GETLATESTCENSUSDATA_CREATE()
    
} // Association
} // Blaze
