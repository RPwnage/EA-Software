/*H*************************************************************************************************/
/*!

    \File    MasterMetrics.h

    \Description
        Stores health check / monitoring statistics about the XMSHD component (master)

    \Notes
        None

    \Copyright
        Copyright (c) Electronic Arts 2013.
        ALL RIGHTS RESERVED.


    $Header: //gosdev/games/FIFA/2014/Gen3/DEV/blazeserver/13.x/customcomponent/xmshd/mastermetrics.h#1 $

*/
/*************************************************************************************************H*/

#ifndef BLAZE_XMSHD_MASTERMETRICS_H
#define BLAZE_XMSHD_MASTERMETRICS_H

/*** Include files *******************************************************************************/

#include "framework/util/counter.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class ComponentStatus;

namespace XmsHd
{

class MasterMetrics
{
    NON_COPYABLE(MasterMetrics);

public: // Functions used by the XmsHd slave impl.

    //! Constructor. Allocates per-type counters and sets all data to default values.
    MasterMetrics();

    //! Destructor. Cleans up allocated memory.
    ~MasterMetrics();

    //! Reports all metrics to the specified status object.
    void report(ComponentStatus* status) const;

public: // Metric variables.

    //! Number of game wins purchased
    Counter     mTotalPublishData;

private:

    typedef eastl::vector<Counter*> CounterList;
    //! List of all metrics.
    CounterList mCounterList;

};

} // namespace XmsHd
} // namespace Blaze

#endif // BLAZE_XMSHD_MASTERMETRICS_H
