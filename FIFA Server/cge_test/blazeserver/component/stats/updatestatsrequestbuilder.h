/*************************************************************************************************/
/*!
\file   updatestatsrequestbuilder.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_UPDATESTATSREQUESTBUILDER_H
#define BLAZE_STATS_UPDATESTATSREQUESTBUILDER_H

#include "stats/tdf/stats_server.h"
#include "stats/transformstats.h"

namespace Blaze
{
namespace Stats
{

class UpdateStatsRequestBuilder : public UpdateStatsRequest
{

public:
    UpdateStatsRequestBuilder();
    ~UpdateStatsRequestBuilder() override;

    /* \brief This method will either create a new row, or open an existing row for editing */
    void startStatRow(const char8_t* category, EntityId playerId, 
        const ScopeNameValueMap* scopeNameValueMap = nullptr);
    void makeStat(const char8_t* name, const char8_t* value, int32_t updateType);
    void assignStat(const char8_t* name, int64_t value);
    void assignStat(const char8_t* name, const char8_t* value);
    void selectStat(const char8_t* name);
    void clearStat(const char8_t* name);
    void incrementStat(const char8_t* name, int64_t value);
    void incrementStat(const char8_t* name, const char8_t* value);
    void decrementStat(const char8_t* name, int64_t value);
    void decrementStat(const char8_t* name, const char8_t* value);
    void completeStatRow();
    void discardStats();

    /* \brief This method will return the first key that matches the category/entity, if multiple keys exist with different keyscopes.
              Do not modify the returned key. */
    UpdateRowKey* getUpdateRowKey(const char8_t* category, EntityId entityId);

    /* \brief This method requires that all keyscopes match the desired key. Passing nullptr for the keyscopes is not the same as the above function. */
    const UpdateRowKey* getUpdateRowKey(const char8_t* category, EntityId entityId, const ScopeNameValueMap* scopeNameValueMap);

    bool empty() { return getStatUpdates().empty(); }

private:
    /*\brief Currently open stat row object being edited; closed by invoking completeStatRow()  */
    StatRowUpdate* mOpenRow;
    bool mOpenNewRow;
    /*\brief StatRowUpdate* map keyed by (category, entityid, scope index map) */
    StatRowUpdateMap mUpdateStatsMap;

    typedef eastl::map<eastl::string, UpdateRowKey> StatUpdateRowKeyMap;
    StatUpdateRowKeyMap mStatUpdateRowKeyMap;
};

} //namespace Stats
} //namespace Blaze

#endif

