/*************************************************************************************************/
/*!
\file   statstransform.h
\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_STATSTRANSFORM
#define BLAZE_GAMEREPORTING_STATSTRANSFORM

#include "framework/tdf/attributes.h"
#include "transformstats.h"

namespace Blaze
{
    
namespace GameReporting
{

/* \brief Interface that defines a mechanism whereby each StatRowUpdate created via UpdateStatsUtil
    will be forwarded to the implementor of StatTransform in order to build up a data structure
    that will be used to apply a transformation to a subset of stats retrieved and locked in the DB
    via a call to StatsSlaveImpl::updateStats() */
class StatsTransform
{
public:
    virtual ~StatsTransform() {}
    
    /* \brief Initialize new per/row transform object and prepare to add transformable stats to it.
        Each transform row may contain additional parameters stored in the @arguments attribute map.
        NOTE: StatsTransform will often cache the Stats::StatRowUpdate object pointer for performance,
        so be sure to call ::clear() on the StatsTransform before the Stats::StatRowUpdate object is
        deleted by its owner! */
    virtual bool addTransformRow(const Stats::StatRowUpdate* update, const Collections::AttributeMap* arguments) = 0;
    
    /* \brief Not all stats need to be transformed, add the one we want to the currently open per/row transform object */
    virtual bool addTransformStat(const char8_t* name) = 0;
    
    /* \brief Finalize the currently open per/row transform object and prepare for the ::transformStats() call */
    virtual bool completeTransformRow() const = 0;
    
    /* \brief Iterate the per/row transform objects and lookup the corresponding stats
        fetched from the DB by the UpdateRowMap, then apply the desired transformation 
        to those stats before they are written back to the DB */
    virtual void transformStats(Stats::UpdateRowMap* map) = 0;
    
    /* \brief Clear any per/row tranform objects and any other cached data. In order to prepare for receiving
        a new batch of Stats::StatRowUpdate* objects via ::addTransformRow().
        NOTE: Typically ::clear() will be invoked immediately after ::transformStats() */
    virtual void clear() = 0;
};

} //namespace GameReporting
} //namespace Blaze

#endif
