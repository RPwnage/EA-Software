/*H*************************************************************************************************/
/*!

    \File    MasterMetrics.h

    \Description
        Stores health check / monitoring statistics about the OSDKSeasonalplay component (master)

    \Notes
        None

    \Copyright
        Copyright (c) Electronic Arts 2010.
        ALL RIGHTS RESERVED.


    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/mastermetrics.h#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

*/
/*************************************************************************************************H*/

#ifndef BLAZE_FIFACUPS_MASTERMETRICS_H
#define BLAZE_FIFACUPS_MASTERMETRICS_H

/*** Include files *******************************************************************************/

#include "framework/util/counter.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class ComponentStatus;

namespace FifaCups
{

class MasterMetrics
{
    NON_COPYABLE(MasterMetrics);

public: // Functions used by the FifaCups slave impl.

    //! Constructor. Allocates per-type counters and sets all data to default values.
    MasterMetrics();

    //! Destructor. Cleans up allocated memory.
    ~MasterMetrics();

    //! Reports all metrics to the specified status object.
    void report(ComponentStatus* status) const;

public: // Metric variables.

    //! Number of seasons
    Counter     mTotalSeasons;

    //! Number of configured awards
    Counter     mTotalAwards;

    //! Amount of time in ms that the last end of season processing took
    Counter     mLastEndSeasonProcessingTimeMs;

private:

    typedef eastl::vector<Counter*> CounterList;
    //! List of all metrics.
    CounterList mCounterList;

};

} // namespace FifaCups
} // namespace Blaze

#endif // BLAZE_FIFACUPS_MASTERMETRICS_H
