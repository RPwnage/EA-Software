
#include "NisGameReporting.h"

#include "EAAssert/eaassert.h"

#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/component/gamereporting/custom/sample/tdf/samplecustomreport.h"
#include "BlazeSDK/callback.h"


using namespace Blaze::GameReporting;

namespace NonInteractiveSamples
{

NisGameReporting::NisGameReporting(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisGameReporting::~NisGameReporting()
{
}

void NisGameReporting::onCreateApis()
{
    getUiDriver().addDiagnosticFunction();

    GameReportingComponent::createComponent(getBlazeHub());

    // For convenience, cache a pointer to the Game Reporting component
    mGameReportingComponent = getBlazeHub()->getComponentManager(0)->getGameReportingComponent();
}

void NisGameReporting::onCleanup()
{
}

//------------------------------------------------------------------------------
// Base class calls this. Send an offline game report, since it is the simplest case.
void NisGameReporting::onRun()
{
    getUiDriver().addDiagnosticFunction();

    AppSubmitOfflineGameReport();
}

//------------------------------------------------------------------------------
// Create a game report for an offline game, and submit it 
void NisGameReporting::AppSubmitOfflineGameReport(void)
{
    getUiDriver().addDiagnosticFunction();

    // Request notification when the Blaze Server has finished processing the report
    mGameReportingComponent->setResultNotificationHandler(
        Blaze::MakeFunctor(this, &NisGameReporting::ResultNotificationCb));

    // Seed rand
    ::srand((unsigned int)::time(nullptr));

    // Create our variable report
    SampleBase::Report report;   

    // Fill GameAttributes with random values
    SampleBase::GameAttributes & gameAttributes = report.getGameAttrs();
    gameAttributes.setMapId((int32_t)(::rand() % 3 + 1));
    gameAttributes.setMode((int32_t)(::rand() % 2 + 1));     

    // Fill PlayerReportsMap for local player, with random values
    SampleBase::Report::PlayerReportsMap & playerReportsMap = report.getPlayerReports();
    const Blaze::GameManager::PlayerId playerId = getBlazeHub()->getUserManager()->getLocalUser(0)->getId();
    playerReportsMap[playerId] = playerReportsMap.allocate_element();
    SampleBase::PlayerReport * playerReport = playerReportsMap[playerId];
    playerReport->setMoney((uint32_t)(::rand() % 10));
    playerReport->setKills((uint16_t)(::rand() % 3));
    playerReport->setDeaths((uint16_t)(::rand() % 10));
    playerReport->setLongestTimeAlive((uint32_t)(::rand() % 10));

    // Create the request for submitOfflineGameReport
    SubmitGameReportRequest request;
    GameReport & gameReport = request.getGameReport();
    gameReport.setGameReportName("sampleReportBase");
    gameReport.setReport(report);
    
    // Finally, submit the report
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "submitOfflineGameReport (gametype %s, for player %d)", 
        gameReport.getGameReportName(), playerId);
    mGameReportingComponent->submitOfflineGameReport(request, 
        MakeFunctor(this, &NisGameReporting::SubmitOfflineGameReportCb));
}

//------------------------------------------------------------------------------
// This tells us when the blaze server has completed processing our report
void NisGameReporting::ResultNotificationCb(const ResultNotification * notification, uint32_t userIndex)
{
    const Blaze::BlazeError err = notification->getBlazeError();

    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "NisGameReporting::ResultNotificationCb: %d (%s) (game reporting id %d)", 
        err, getBlazeHub()->getErrorName(err), notification->getGameReportingId());

    if (err == Blaze::ERR_OK)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  Success! Now use the SampleStats sample to fetch these stats.");

        AppGetGameReports();
    }
    else
    {
        reportBlazeError(err);
    }
}

//------------------------------------------------------------------------------
// The result of submitOfflineGameReport
void NisGameReporting::SubmitOfflineGameReportCb(Blaze::BlazeError err, Blaze::JobId)
{
    getUiDriver().addDiagnosticCallback();

    if (err != Blaze::ERR_OK)
    {
        reportBlazeError(err);
    }
}

//------------------------------------------------------------------------------
// Execute a query on game reports
void NisGameReporting::AppGetGameReports(void)
{
    getUiDriver().addDiagnosticFunction();

    Blaze::GameReporting::GetGameReports request; 

    request.setQueryName("recent_sample_game"); 

    // Use this instead for a more refined query
    //request.setQueryName("my_recent_sample_game"); 
    //request.getQueryVarValues().push_back("2"); // i.e: player kills = 2

    getUiDriver().addDiagnosticCallFunc(mGameReportingComponent->getGameReports(request, 
        MakeFunctor(this, &NisGameReporting::GetGameReportsCb))); 
}

//------------------------------------------------------------------------------
// The result of getGameReports. Describe any reports found.
void NisGameReporting::GetGameReportsCb(const Blaze::GameReporting::GameReportsList * gameReportsList, Blaze::BlazeError err, Blaze::JobId jobId)
{
    getUiDriver().addDiagnosticCallback();

    if (err == Blaze::ERR_OK)
    {
        const GameReportsList::GameHistoryReportList & gameHistoryReportList = gameReportsList->getGameReportList();
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  gameHistoryReportList size = %d", gameHistoryReportList.size());

        GameReportsList::GameHistoryReportList::const_iterator GHRLitr      = gameHistoryReportList.begin();
        GameReportsList::GameHistoryReportList::const_iterator GHRLitrEnd   = gameHistoryReportList.end();
        for (;GHRLitr != GHRLitrEnd; ++GHRLitr)
        {
            const GameHistoryReport * gameHistoryReport = *GHRLitr;

            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    ");
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    mGameHistoryId  = %I64d", gameHistoryReport->getGameHistoryId());
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    mGameReportName     = %s", gameHistoryReport->getGameReportName());
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    mTableRowMap:");

            const GameHistoryReport::TableRowMap & tableRowMap = gameHistoryReport->getTableRowMap();
            GameHistoryReport::TableRowMap::const_iterator TRMitr       = tableRowMap.begin();
            GameHistoryReport::TableRowMap::const_iterator TRMitrEnd    = tableRowMap.end();
            for (;TRMitr != TRMitrEnd; ++TRMitr)
            {
                const EA::TDF::TdfString & tableName = TRMitr->first;
                getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "      TableName: %s", tableName.c_str());
                const GameHistoryReport::TableRows * tableRows = TRMitr->second;
                const GameHistoryReport::TableRowList & tableRowList = tableRows->getTableRowList();
                getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "      TableRows:");

                GameHistoryReport::TableRowList::const_iterator TRLitr      = tableRowList.begin();
                GameHistoryReport::TableRowList::const_iterator TRLItrEnd   = tableRowList.end();
                for (;TRLitr != TRLItrEnd; ++TRLitr)
                {
                    const GameHistoryReport::TableRow * tableRow = *TRLitr;
                    const Blaze::Collections::AttributeMap & attributeMap = tableRow->getAttributeMap();

                    Blaze::Collections::AttributeMap::const_iterator AMitr      = attributeMap.begin();
                    Blaze::Collections::AttributeMap::const_iterator AMitrEnd   = attributeMap.end();
                    for (; AMitr != AMitrEnd; AMitr++)
                    {
                        const Blaze::Collections::AttributeName & attrName      = AMitr->first;
                        const Blaze::Collections::AttributeValue & attrvalue    = AMitr->second;
                        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "        %s, %s", attrName.c_str(), attrvalue.c_str());
                    }
                }
            }
        }

        done();
    }
    else
    {
        reportBlazeError(err);
    }
}

}
