/*************************************************************************************************/
/*!
    \file   dynamicinetfilterslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_DYNAMICINETFILTER_DYNAMICINETFILTERSLAVEIMPL_H
#define BLAZE_DYNAMICINETFILTER_DYNAMICINETFILTERSLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/connection/inetfilter.h"

#include "framework/dynamicinetfilter/tdf/dynamicinetfilter.h"
#include "framework/dynamicinetfilter/tdf/dynamicinetfilter_server.h"
#include "framework/dynamicinetfilter/rpc/dynamicinetfilterslave_stub.h"
#include "framework/dynamicinetfilter/dynamicinetfilterimpl.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace DynamicInetFilter
{

class DynamicInetFilterSlaveImpl: public DynamicInetFilterSlaveStub,
    public DynamicInetFilterImpl
{
public:
    DynamicInetFilterSlaveImpl();
    ~DynamicInetFilterSlaveImpl() override;

    typedef eastl::list<EntryPtr> EntryList;

    // Inherited from Component
    bool onConfigure() override;
    bool onReconfigure() override;
    bool onPrepareForReconfigure(const DynamicinetfilterConfig& config) override;
    bool onValidateConfig(DynamicinetfilterConfig& config, const DynamicinetfilterConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    void onShutdown() override;
    const DbNameByPlatformTypeMap* getDbNameByPlatformTypeMap() const override { return &getConfig().getDbNamesByPlatform(); }
    uint16_t getDbSchemaVersion() const override{ return 2; }

private:
    void onAddOrUpdateNotification(const Entry &request, Blaze::UserSession *) override;
    void onRemoveNotification(const Blaze::DynamicInetFilter::RemoveRequest &request, Blaze::UserSession *) override;

    void insertFilters(EntryList& entryList);
    bool loadFiltersFromDb();

    EntryList mEntryList;
};

}
} // Blaze

#endif // BLAZE_DYNAMICINETFILTER_DYNAMICINETFILTERSLAVEIMPL_H

