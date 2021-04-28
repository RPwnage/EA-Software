
#include "NisLeaderboards.h"

#include "EAAssert/eaassert.h"

using namespace Blaze::Stats;

namespace NonInteractiveSamples
{

NisLeaderboards::NisLeaderboards(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisLeaderboards::~NisLeaderboards()
{
}

void NisLeaderboards::onCreateApis()
{
    getUiDriver().addDiagnosticFunction();

    // Create the leaderboard API
    LeaderboardAPI::createAPI(*getBlazeHub());

    // For the sake of convenience, cache a pointer to the API
    mApi = getBlazeHub()->getLeaderboardAPI();
}

void NisLeaderboards::onCleanup()
{
}

//------------------------------------------------------------------------------
// Base class calls this.
void NisLeaderboards::onRun()
{
    getUiDriver().addDiagnosticFunction();

    // Lets fetch the "IS_LB_allmodes" leaderboard (defined in leaderboards.cfg on the Blaze Server)
    // This may not be populated if you are using your own Blaze Server; use the Integrated Sample to populate it
    Blaze::Stats::GetLeaderboardCb cb(this, &NisLeaderboards::GetLeaderboardCb);
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "requestLeaderboard \"IS_LB_allmodes\"");
    mApi->requestLeaderboard(cb, "IS_LB_allmodes");
}

//------------------------------------------------------------------------------
// The result of requestLeaderboard. If it succeeded, we'll request a particular view of the data
void NisLeaderboards::GetLeaderboardCb(Blaze::BlazeError err, Blaze::JobId, Leaderboard* lb)
{
    getUiDriver().addDiagnosticCallback();

    if (err == Blaze::ERR_OK)
    {
        // Create a view with a specified rank range
        const int startRank = 0;
        const int endRank = 99;
        Leaderboard::CreateGlobalViewCb cb(this, &NisLeaderboards::CreateGlobalViewCb);
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "createGlobalLeaderboardView (%d - %d)", startRank, endRank);
        lb->createGlobalLeaderboardView(cb, startRank, endRank);
    }
    else
    {
        reportBlazeError(err);
        EA_FAIL(); 
    }
}

//------------------------------------------------------------------------------
// The result of createGlobalLeaderboardView. If it succeeded, we'll render the leaderboard data
void NisLeaderboards::CreateGlobalViewCb(Blaze::BlazeError err, Blaze::JobId, GlobalLeaderboardView* view)
{
    getUiDriver().addDiagnosticCallback();

    if (err == Blaze::ERR_OK)
    {
        // Describe the view
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "%s (%s)", view->getName(), view->getDescription());

        // Show the column headings
        
        // The first two column headings are fixed
        char strLine[1024];
        ds_snzprintf(strLine, sizeof(strLine), "%-12s%-15s", "Rank", "Entity");

        // The rest are leaderboard-specific, obtained from the leaderboard that this view is based on
        const StatDescSummaryList* cols = view->getLeaderboard()->getStatKeyColumnsList();
        StatDescSummaryList::const_iterator colsItr = cols->begin();
        StatDescSummaryList::const_iterator colsItrEnd = cols->end();
        for (; colsItr != colsItrEnd; ++colsItr)
        {
            char strTmp[32];

            const StatDescSummary * column = *colsItr;

            ds_snzprintf(strTmp, sizeof(strTmp), "%-12s", column->getName());
            ds_strnzcat(strLine, strTmp, sizeof(strLine));          
        }

        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, strLine);

        // Now show the leaderboard data, obtained via getStatValues()
        const LeaderboardStatValues* lbStats = view->getStatValues();
        const LeaderboardStatValues::RowList& rowsList = lbStats->getRows();
        LeaderboardStatValues::RowList::const_iterator rowsItr = rowsList.begin();
        LeaderboardStatValues::RowList::const_iterator rowsItrEnd = rowsList.end();

        // We'll count rank from 1, pandering to convention
        for (int rank = 1; rowsItr != rowsItrEnd; ++rowsItr, ++rank)
        {
            const LeaderboardStatValuesRow* rowVal = (*rowsItr);

            char strData[1024];
            ds_snzprintf(strData, sizeof(strData), "%-12d%-15s", rank, rowVal->getUser().getName());
            
            const LeaderboardStatValuesRow::StringStatValueList& othersList = rowVal->getOtherStats();
            LeaderboardStatValuesRow::StringStatValueList::const_iterator othersItr = othersList.begin();
            LeaderboardStatValuesRow::StringStatValueList::const_iterator othersItrEnd = othersList.end();

            for (; othersItr != othersItrEnd; ++othersItr)
            {
                char strTmp[32];

                ds_snzprintf(strTmp, sizeof(strTmp), "%-12s", (*othersItr).c_str());
                ds_strnzcat(strData, strTmp, sizeof(strLine));  
            }
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, strData);
        }

        // We're required to release the view now that we're finished with it
        view->release();
        done();
    }
    else
    {
        //EA_FAIL(); 
        if (err == Blaze::ERR_SYSTEM)
        {
            // SAMPLES_TODO - if no data is available you just get ERR_SYSTEM here - would be nice to have something more specific
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "No data is available for the leaderboard; use the Integrated Sample to create some data.");
        }
        else
        {
            EA_FAIL(); 
        }

        reportBlazeError(err);
    }
}

}

