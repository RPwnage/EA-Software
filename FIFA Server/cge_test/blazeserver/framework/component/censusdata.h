/*************************************************************************************************/
/*!
/file censusdata.h


/attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CENSUSDATA_H
#define BLAZE_CENSUSDATA_H

/*** Include files *******************************************************************************/
#include "blazerpcerrors.h"
#include "framework/tdf/censusdatatype.h"
#include "EASTL/vector.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

/*! **************************************************************************/
/*! /class  CensusDataProvider
    /brief  Abstract class to be implemented by any class wishing to provide census data
    /note   Implementers must register the CensusDataProvider with CensusDataManager in order 
            to receive search requests, implementers must supply a custom Iterator that is
            packaged inside the IteratorPtr returned from the search methods. 
*/
class CensusDataProvider
{
public:
    virtual ~CensusDataProvider() { }

    /*! /brief Create a map (by platform) of custom EA::TDF::Tdfs with component-specific census data. */
    virtual BlazeRpcError DEFINE_ASYNC_RET(getCensusData(CensusDataByComponent& censusDataByPlatform)) = 0;
};

/*! **************************************************************************/
/*! /class  CensusDataManager
    /brief  Manager class that services populate census data requests from CensusData
            component
    /note
    1) Provides CensusDataProvider registration facility
    2) Maintains a lookup table of CensusDataProviders indexed by Component Id
    3) Forwards getCensusData requests to the underlying CensusDataProvider registered with it
*/
class CensusDataManager
{
    NON_COPYABLE(CensusDataManager);

public:
    typedef eastl::vector<EA::TDF::TdfPtr> CensusDataList;

    CensusDataManager();

    //////////////////////////////////////////////////////////////////////////////
    // registration methods
    //
    
    /*! /brief Register a CensusDataProvider for the given component. Memory is owned by the caller. */
    bool registerProvider(ComponentId compId, CensusDataProvider& provider);

    /*! /brief Deregister a CensusDataProvider for the given component. */
    bool unregisterProvider(ComponentId compId);

    //////////////////////////////////////////////////////////////////////////////
    // interface function
    //

    // census data
    BlazeRpcError DEFINE_ASYNC_RET(populateCensusData(CensusDataByComponent& censusDataList));
    BlazeRpcError DEFINE_ASYNC_RET(populateCensusDataForComponents(const CensusDataComponentIds &componentIds, CensusDataByComponent& censusDataList));
    void getCensusDataProviderComponentIds(CensusDataComponentIds &componentIds);

    // region user statistic related
    void  addUserRegion(BlazeId blazeId, uint32_t country);
    void  removeUserRegion(BlazeId blazeId, uint32_t country);
    const NumOfUsersByRegionMap& getNumOfUsersByRegionMap() const { return mNumOfUsersByRegionMap; }
    
private:

    typedef eastl::hash_map<ComponentId, CensusDataProvider*> ProvidersByComponent;
    ProvidersByComponent mProviders;

    NumOfUsersByRegionMap mNumOfUsersByRegionMap;
};

extern EA_THREAD_LOCAL CensusDataManager* gCensusDataManager;

} // namespace Blaze

#endif // BLAZE_CENSUSDATA_H

