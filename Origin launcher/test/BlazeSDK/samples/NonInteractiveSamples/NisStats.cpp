
#include "NisStats.h"

#include "EAAssert/eaassert.h"

#include "BlazeSDK/loginmanager/loginmanager.h"


using namespace Blaze::Stats;

namespace NonInteractiveSamples
{

NisStats::NisStats(NisBase &nisBase)
    : NisSample(nisBase)
{
}

NisStats::~NisStats()
{
}

void NisStats::onCreateApis()
{
    // For the sake of convenience, cache a pointer to the API
    mStatsApi = getBlazeHub()->getStatsAPI();
}

void NisStats::onCleanup()
{
}

void NisStats::onRun()
{
    getUiDriver().addDiagnosticFunction();

    // First we need to fetch the keyscopes into the local cache
    Blaze::Stats::GetKeyScopeCb cb(this, &NisStats::GetKeyScopeCb);
    getUiDriver().addDiagnosticCallFunc(mStatsApi->requestKeyScopes(cb));
}

//------------------------------------------------------------------------------
// The result of requestKeyScopes. If it succeeded, we'll request the stats group that interests us.
void NisStats::GetKeyScopeCb(Blaze::BlazeError err, Blaze::JobId id, const KeyScopes* keyScopes)
{
    getUiDriver().addDiagnosticCallback();

    if (err == Blaze::ERR_OK)
    {
        // We now have the keyscopes cached. Now fetch a particular group.
        Blaze::Stats::GetStatsGroupCb cb(this, &NisStats::GetStatsGroupCb);
        const char * GROUPNAME = "SampleStatsGroup";
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "requestStatsGroup \"%s\"", GROUPNAME);
        mStatsApi->requestStatsGroup(cb, GROUPNAME);
    }
    else
    {
        reportBlazeError(err);
    }
}

//------------------------------------------------------------------------------
// The result of requestStatsGroup. If it succeeded, we'll create a view for the local user
void NisStats::GetStatsGroupCb(Blaze::BlazeError err, Blaze::JobId, StatsGroup* group)
{
    getUiDriver().addDiagnosticCallback();

    if (err == Blaze::ERR_OK)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  group: %s (%s)", group->getName(), group->getDescription());

        // Create a simple view for the local user only
        const int entityIdCount = 1;
        Blaze::EntityId entityIdList[entityIdCount];
        entityIdList[0] = getBlazeHub()->getLoginManager(0)->getBlazeId();
        StatsGroup::CreateViewCb cb(this, &NisStats::CreateViewCb);
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "createStatsView for entity %u", entityIdList[0]);
        group->createStatsView(cb, entityIdList, entityIdCount, STAT_PERIOD_ALL_TIME, 0, nullptr);
    }
    else
    {
        reportBlazeError(err);
    }
}

//------------------------------------------------------------------------------
// The result of createStatsView. If it succeeded, we'll dig out the data
void NisStats::CreateViewCb(Blaze::BlazeError err, Blaze::JobId, StatsView* view)
{
    getUiDriver().addDiagnosticCallback();

    if (err == Blaze::ERR_OK)
    {
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  view: %s (%s)", view->getName(), view->getDescription());

        // List the stat names. Get this from the StatsGroup that this StatsView was created from.
        const StatGroupResponse::StatDescSummaryList * SDSList = view->getStatsGroup()->getStatKeyColumnsList();
        StatGroupResponse::StatDescSummaryList::const_iterator SDSListItr       = SDSList->begin();
        StatGroupResponse::StatDescSummaryList::const_iterator SDSListItrEnd    = SDSList->end();
        int row = 1;
        for(; SDSListItr != SDSListItrEnd; ++SDSListItr, row++)
        {
            const StatDescSummary * statDesc = *SDSListItr;
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  descriptions for stat %d: %s/%s", row, statDesc->getShortDesc(), statDesc->getLongDesc());
        }

        // List the stat values for each keyscope
        // If Stats are found without any keyscope, the scopeString will be "No_Scope_Defined".
        const KeyScopeStatsValueMap * KSSVMap = view->getKeyScopeStatsValueMap();
        if (KSSVMap->empty())
        {
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    No stats found. Please use SampleGameReporting to populate some stats.");
        }
        else
        {
            KeyScopeStatsValueMap::const_iterator KSSVMapIter       = KSSVMap->begin();
            KeyScopeStatsValueMap::const_iterator KSSVMapIterEnd    = KSSVMap->end();
            for (;KSSVMapIter != KSSVMapIterEnd; ++KSSVMapIter)
            {
                const ScopeString & scopeString = KSSVMapIter->first;
                getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "  scopeString: %s", scopeString.c_str());

                const StatValues * statValues = KSSVMapIter->second;
                DescribeStatValues(statValues);
            }
        }

        done();
    }
    else
    {
        reportBlazeError(err);
    }
}

//------------------------------------------------------------------------------
// If statValues is non-nullptr, log the stat values for each entity
void NisStats::DescribeStatValues(const StatValues * statValues)
{
    const StatValues::EntityStatsList & entityStatsList = statValues->getEntityStatsList();

    StatValues::EntityStatsList::const_iterator ESLItr      = entityStatsList.begin();
    StatValues::EntityStatsList::const_iterator ESLItrEnd   = entityStatsList.end();

    // Traverse the entity list.
    // In this case our entity list should contain 1 set of data since we are only requesting stats for the local player
    for (; ESLItr != ESLItrEnd; ++ESLItr)
    {
        const EntityStats * entity = *ESLItr;
        getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "    Entity %d:", entity->getEntityId());

        // We'll iterate over the Value list to get the values this entity has for these stats
        const EntityStats::StringStatValueList &SSVList = entity->getStatValues();
        EntityStats::StringStatValueList::const_iterator SSVListItr     = SSVList.begin();
        EntityStats::StringStatValueList::const_iterator SSVListItrEnd  = SSVList.end();

        int row = 1;
        for (; SSVListItr != SSVListItrEnd; ++SSVListItr, ++row)
        {
            StatValue stat = *SSVListItr;
            getUiDriver().addLog_va(Pyro::ErrorLevel::NONE, "      value for stat %d: %s", row, stat.c_str());
        }
    }
}

}
