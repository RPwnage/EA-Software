
#include "NisLeaderboardList.h"

#include "EAAssert/eaassert.h"

using namespace Blaze;

namespace NonInteractiveSamples
{

NisLeaderboardList::NisLeaderboardList(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisLeaderboardList::~NisLeaderboardList()
{
}

void NisLeaderboardList::onCreateApis()
{
    getUiDriver().addDiagnosticFunction();

    // Create the leaderboard API
    Stats::LeaderboardAPI::createAPI(*getBlazeHub());

    // For the sake of convenience, cache a pointer to the API
    mApi = getBlazeHub()->getLeaderboardAPI();
}

void NisLeaderboardList::onCleanup()
{
}

//------------------------------------------------------------------------------
// Base class calls this.
void NisLeaderboardList::onRun(void)
{
    getUiDriver().addDiagnosticFunction();

    // Get the top most folder of the leaderboard folder hiearchy.  This folder is always called
    // TopFolder and contains all of the folders defined in leaderboard.cfg
    Stats::GetLeaderboardFolderCb cb(this, &NisLeaderboardList::GetLeaderboardFolderCb);
    mApi->requestTopLeaderboardFolder(cb);
}

//------------------------------------------------------------------------------
// Callback for requestTopLeaderboardFolder
void NisLeaderboardList::GetLeaderboardFolderCb(Blaze::BlazeError error, Blaze::JobId jobId, Blaze::Stats::LeaderboardFolder *leaderboardFolder)
{
    getUiDriver().addDiagnosticCallback();

    if (error != Blaze::ERR_OK)
    {
        reportBlazeError(error);
    }

    // Lets output the name of this particular leaderboard folder
    getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Folder: %s", leaderboardFolder->getName());

    // If this folder has child folder, lets go ahead a ask for a list of those - for the sake of sample code
    if (leaderboardFolder->hasChildFolders())
    {
        Stats::LeaderboardFolder::GetLeaderboardFolderListCb cb(this, &NisLeaderboardList::GetLeaderboardFolderListCb);
        leaderboardFolder->requestChildLeaderboardFolderList(cb);
    }

    // If this folder contains any leaderboards, lets ask for a list of them
    if (leaderboardFolder->hasChildLeaderboards())
    {
        Stats::LeaderboardFolder::GetLeaderboardListCb cb(this, &NisLeaderboardList::GetLeaderboardListCb);
        leaderboardFolder->requestChildLeaderboardList(cb);
    }
}

//------------------------------------------------------------------------------
// The callback called called as a result of callling requestChildLeaderboardFolderList
void NisLeaderboardList::GetLeaderboardFolderListCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::Stats::LeaderboardFolderList *leaderboardFolderList)
{
    getUiDriver().addDiagnosticCallback();

    if (error != Blaze::ERR_OK)
    {
        reportBlazeError(error);
    }

    // Here, for the sake of the sample, lets iterate through the list of folders
    // and manually call GetLeaderboardFolderCb, in order to output the folder names
    // and request any of their sub-folders
    for (Stats::LeaderboardFolderList::const_iterator i = leaderboardFolderList->begin();
        i != leaderboardFolderList->end();
        i++)
    {
        // We pass (*i), which is actually an instance of Blaze::Stats::LeaderboardFolder
        GetLeaderboardFolderCb(error, jobId, (*i));
    }
}

//------------------------------------------------------------------------------
// Callback for requestChildLeaderboardList
void NisLeaderboardList::GetLeaderboardListCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::Stats::LeaderboardList *leaderboardList)
{

    if (error != Blaze::ERR_OK)
    {
        reportBlazeError(error);
    }

    // Iterate through the list of returned leaderboards
    for (Stats::LeaderboardList::const_iterator i = leaderboardList->begin();
        i != leaderboardList->end();
        i++)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "Leaderboard: %s", (*i)->getName());
    }
}

}
