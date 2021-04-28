/*************************************************************************************************/
/*!
    \file   statsserviceutil.h

    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_STATSSERVICEUTIL
#define BLAZE_GAMEREPORTING_STATSSERVICEUTIL

#include "eadp/stats/stats_core.grpc.pb.h"
#include "framework/grpc/outboundgrpcjobhandlers.h"

namespace Blaze
{
namespace GameReporting
{

class GameReportingSlaveImpl;

namespace Utilities
{

struct UpdateStatsServiceKey
{
public:
    UpdateStatsServiceKey();
    UpdateStatsServiceKey(const char8_t* context, const char8_t* category, const char8_t* entityId);
    bool operator < (const UpdateStatsServiceKey& other) const;

    const char8_t* context;
    const char8_t* category;
    const char8_t* entityId;
};

struct UpdateStatsServiceInfo
{
    eadp::stats::UpdateEntityStatsRequest* request;
    Grpc::ResponsePtr responsePtr;
};

typedef eastl::vector_map<UpdateStatsServiceKey, UpdateStatsServiceInfo> UpdateStatsServiceMap;

class StatsServiceUtil;
typedef eastl::intrusive_ptr<StatsServiceUtil> StatsServiceUtilPtr;

class StatsServiceUtil
{
public:
    StatsServiceUtil(GameReportingSlaveImpl& component, ReportType reportType, const char8_t* reportName);
    ~StatsServiceUtil();

    void startStatRequest(const char8_t* context, const char8_t* category, const char8_t* entityId);
    void makeStat(const char8_t* name, double value, int32_t updateType);
    void addDimensionListToStat(const char8_t* statName, double value, int32_t updateType);
    void addDimensionToStatList(const char8_t* statName, int listIndex, const char8_t* dimName, const char8_t* dimValue);
    void completeStatRequest();
    void discardStats();

    void addStatResponse(const char8_t* context, const char8_t* category, const char8_t* entityId, Grpc::ResponsePtr& responsePtr);

    // accessors for custom report processing
    UpdateStatsServiceMap& getUpdateStatsServiceMap()
    {
        return mUpdateStatsServiceMap;
    }
    const UpdateStatsServiceKey* getUpdateStatsServiceKey(const char8_t* context, const char8_t* category, const char8_t* entityId);
    eadp::stats::UpdateEntityStatsRequest* getStatRequest(const char8_t* context, const char8_t* category, const char8_t* entityId);
    eadp::stats::UpdateEntityStatsRequest* getStatRequest(const UpdateStatsServiceKey& key);
    eadp::stats::UpdateEntityStatsResponse* getStatResponse(const char8_t* context, const char8_t* category, const char8_t* entityId);
    eadp::stats::UpdateEntityStatsResponse* getStatResponse(const UpdateStatsServiceKey& key);

    GameReportingSlaveImpl& getComponent()
    {
        return mComponent;
    }
    ReportType getReportType()
    {
        return mReportType;
    }
    const char8_t* getReportName()
    {
        return mReportName.c_str();
    }

    void sendUpdates();
    BlazeRpcError sendUpdatesAndWait();

private:
    static BlazeRpcError addCustomHeaders(grpc::ClientContext& context);

    static void sendUpdateEntityStats(StatsServiceUtilPtr util, const eadp::stats::UpdateEntityStatsRequest& req);

private:
    GameReportingSlaveImpl& mComponent;
    ReportType mReportType;
    eastl::string mReportName;

    eadp::stats::UpdateEntityStatsRequest* mOpenRequest; // the currently open entity stat request object being edited by the parser; closed by invoking completeStatRow()
    UpdateStatsServiceMap mUpdateStatsServiceMap; // UpdateEntityStatsInfo map keyed by (context, category, entityid)

    FiberJobQueue mStatsServiceRpcFiberJobQueue; // queue that completes RPCs sent to the Stats Service

    uint32_t mRefCount;

    friend void intrusive_ptr_add_ref(StatsServiceUtil* ptr);
    friend void intrusive_ptr_release(StatsServiceUtil* ptr);

    NON_COPYABLE(StatsServiceUtil);
};

inline void intrusive_ptr_add_ref(StatsServiceUtil* ptr)
{
    ptr->mRefCount++;
}

inline void intrusive_ptr_release(StatsServiceUtil* ptr)
{
    if (--ptr->mRefCount == 0)
        delete ptr;
}

} // namespace Utilities
} // namespace GameReporting
} // namespace Blaze

#endif  // BLAZE_GAMEREPORTING_STATSSERVICEUTIL
