
#include "NisCensus.h"

#include "BlazeSDK/censusdata/censusdataapi.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"

namespace NonInteractiveSamples
{

NisCensus::NisCensus(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisCensus::~NisCensus()
{
}

void NisCensus::onCreateApis()
{
}

void NisCensus::onCleanup()
{
}

//------------------------------------------------------------------------------
// Base class calls this. We'll subscribe to census data here.
void NisCensus::onRun()
{
    getUiDriver().addDiagnosticFunction();

    mNotificationCount = 0;

    getUiDriver().addDiagnosticCallFunc(getBlazeHub()->getCensusDataAPI()->addListener(this));

    // Subscribe to census data
    getUiDriver().addDiagnosticCallFunc(
        getBlazeHub()->getCensusDataAPI()->subscribeToCensusData(
            Blaze::MakeFunctor(this, &NisCensus::SubscribeToCensusDataCb)));
}

//------------------------------------------------------------------------------
// The result of the subscribeToCensusData call
void NisCensus::SubscribeToCensusDataCb(Blaze::BlazeError error)
{
    getUiDriver().addDiagnosticCallback();
    if (error != Blaze::ERR_OK)
    {
        reportBlazeError(error);
    }
}

//------------------------------------------------------------------------------
// This listener callback gives us the census data
void NisCensus::onNotifyCensusData()
{
    getUiDriver().addDiagnosticCallback();

    mNotificationCount += 1;

    if (mNotificationCount < 5)
    {
        const Blaze::GameManager::GameManagerCensusData* gameManagerCensusData = 
            getBlazeHub()->getGameManagerAPI()->getCensusData();
        if (gameManagerCensusData != nullptr)
        {
            char buf[2048] = "";
            gameManagerCensusData->print(buf, sizeof(buf), 1);
        }
    }
    else
    {
        // Unsubscribe to census data
        getUiDriver().addDiagnosticCallFunc(
            getBlazeHub()->getCensusDataAPI()->unsubscribeFromCensusData(
                Blaze::MakeFunctor(this, &NisCensus::UnsubscribeFromCensusDataCb)));

        getUiDriver().addDiagnosticCallFunc(
            getUiDriver().addDiagnosticCallFunc(getBlazeHub()->getCensusDataAPI()->removeListener(this)));
    }
}

//------------------------------------------------------------------------------
// The result of the unsubscribeFromCensusData call
void NisCensus::UnsubscribeFromCensusDataCb(Blaze::BlazeError error)
{
    getUiDriver().addDiagnosticCallback();

    if (error == Blaze::ERR_OK)
    {
        done();
    }
    else
    {
        reportBlazeError(error);
    }
}

}
