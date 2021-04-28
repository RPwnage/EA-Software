/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_UNIQUE_ID_MANAGER_H
#define BLAZE_UNIQUE_ID_MANAGER_H

/*** Include files *******************************************************************************/

#include "blazerpcerrors.h"
#include "framework/rpc/uniqueidslave_stub.h"


namespace Blaze
{


class UniqueIdManager : public UniqueIdSlaveStub
{

class ComponentIdInfo 
{
public:
    ComponentIdInfo()        
        : mComponentId(0), mIdType(0), mLastId(0),mCurrentId(0), mUpdatingId(false)
    {
    }

    ComponentIdInfo(uint16_t componentId, uint16_t idType, uint64_t lastId, uint64_t currentId)
        : mComponentId(componentId), mIdType(idType), mLastId(lastId),mCurrentId(currentId), mUpdatingId(false)
    {
    }

    ComponentIdInfo& operator=(const ComponentIdInfo& rhs)
    {
        mComponentId = rhs.mComponentId;
        mIdType = rhs.mIdType;
        mLastId = rhs.mLastId;
        mCurrentId = rhs.mCurrentId;
        mUpdatingId = rhs.mUpdatingId;

        return *this;
    }

    uint16_t mComponentId;
    uint16_t mIdType;
    uint64_t mLastId;
    uint64_t mCurrentId;    
    bool mUpdatingId;
};

public:
    static const uint32_t SCHEMA_VERSION = 1;
    static const uint32_t FRAMEWORK_ID = 0;

    UniqueIdManager();
    ~UniqueIdManager() override;

    BlazeRpcError getId(uint16_t componentId, uint16_t idType, uint64_t &id, uint64_t min = 0);
    BlazeRpcError reserveIdType(uint16_t componentId, uint16_t idType);

    uint16_t getDbSchemaVersion() const override { return SCHEMA_VERSION; }
    const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }

protected:
    bool onValidateConfig(UniqueIdConfig& config, const UniqueIdConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    bool onConfigure() override;

private:
    void dbUpdateLastId(uint16_t componentId, uint16_t idType);
    void getBatchSettings(uint16_t componentId, int32_t& batchSize, int32_t& safeSize);

    typedef eastl::map<uint32_t, ComponentIdInfo> LastIdByComponentMap;
    LastIdByComponentMap mLastIdByComponentMap;

};

extern EA_THREAD_LOCAL UniqueIdManager* gUniqueIdManager;

} // Blaze
#endif
