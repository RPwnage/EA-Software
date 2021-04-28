#include "framework/blaze.h"
#include "framework/util/locales.h"
#include "framework/component/censusdata.h"
#include "framework/util/shared/blazestring.h"
#include "framework/connection/connection.h"


namespace Blaze
{

const char8_t* REGION_TOTAL = "TOTAL";

CensusDataManager::CensusDataManager() :
    mProviders(BlazeStlAllocator("CensusDataManager::mProviders"))
{
    mNumOfUsersByRegionMap[REGION_TOTAL] = 0;
}

//////////////////////////////////////////////////////////////////////////////
// registration methods
//
bool CensusDataManager::registerProvider(ComponentId compId, CensusDataProvider& provider)
{
    if(mProviders.find(compId) != mProviders.end())
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[CensusDataManager].registerProvider: CensusDataProvider already registered with component " << compId);
        return false;
    }

    BLAZE_TRACE_LOG(Log::SYSTEM, "[CensusDataManager].registerProvider: added CensusDataProvider for component(" << compId << "): " << &provider);

    mProviders[compId] = &provider;

    return true;
}

bool CensusDataManager::unregisterProvider(ComponentId compId)
{
    ProvidersByComponent::iterator it = mProviders.find(compId);
    if(it == mProviders.end())
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[CensusDataManager].deregisterProvider: CensusDataProvider not registered with for component " << compId);
        return false;
    }

    mProviders.erase(it);

    return true;
}

//////////////////////////////////////////////////////////////////////////////
// interface function
//
BlazeRpcError CensusDataManager::populateCensusData(CensusDataByComponent& censusDataByComponent)
{
    CensusDataComponentIds componentIds;
    getCensusDataProviderComponentIds(componentIds);

    return populateCensusDataForComponents(componentIds, censusDataByComponent);
}


BlazeRpcError CensusDataManager::populateCensusDataForComponents(const CensusDataComponentIds &componentIds, CensusDataByComponent& censusDataByComponent)
{
    for (ComponentId componentId : componentIds)
    {
        ProvidersByComponent::const_iterator iter = mProviders.find(componentId);
        if (iter == mProviders.end())
            continue;

        censusDataByComponent[componentId] = censusDataByComponent.allocate_element();
        if (iter->second->getCensusData(censusDataByComponent) != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[CensusDataManager].populateCensusDataForComponents Census Data Error: Failed when trying to retrieve the server census data from component id [" << componentId << "].\n");
            censusDataByComponent.erase(componentId);
        }
    }
    return ERR_OK;
}


void CensusDataManager::addUserRegion(BlazeId blazeId, uint32_t country)
{
    char8_t region[MAX_REGIONNAME_LEN] = "";
    LocaleTokenCreateCountryString(region, country); // country of residence is not locale-related, but re-using locale macro for convenience

    BLAZE_TRACE_LOG(Log::SYSTEM, "Census Region Count: Log in user (" << blazeId << ") region (" << region << ").");
    if (mNumOfUsersByRegionMap.find(region) == mNumOfUsersByRegionMap.end())
    {
        mNumOfUsersByRegionMap[region] = 1;
    }
    else
    {
        mNumOfUsersByRegionMap[region] += 1;
    }

    mNumOfUsersByRegionMap[REGION_TOTAL] += 1;
};

void CensusDataManager::removeUserRegion(BlazeId blazeId, uint32_t country)
{
    char8_t region[MAX_REGIONNAME_LEN] = "";
    LocaleTokenCreateCountryString(region, country);  // country of residence is not locale-related, but re-using locale macro for convenience
    
    BLAZE_TRACE_LOG(Log::SYSTEM, "Census Region Count: Log out user (" << blazeId << ") region (" << region << ").");
    if (mNumOfUsersByRegionMap.find(region) != mNumOfUsersByRegionMap.end())
    {
        mNumOfUsersByRegionMap[region] -= 1;

        if (mNumOfUsersByRegionMap[region] == 0)
            mNumOfUsersByRegionMap.erase(region);

        if (mNumOfUsersByRegionMap[REGION_TOTAL] > 0)
            mNumOfUsersByRegionMap[REGION_TOTAL] -= 1;
    }
    else
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "Census Region Count Error: the region (" << region << ") could not be found during logout of a user");
    }
};

void CensusDataManager::getCensusDataProviderComponentIds(CensusDataComponentIds &componentIds)
{
    componentIds.clear();
    componentIds.reserve(mProviders.size());
    for (ProvidersByComponent::const_iterator it = mProviders.begin(), itEnd = mProviders.end();
         it != itEnd;
         it++)
    {
        componentIds.push_back(it->first);
    }
}


} // namespace Blaze

