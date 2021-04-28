/*H*************************************************************************************************/
/*!

    \File    MasterMetrics.h

    \Description
        Stores health check / monitoring statistics about the EASFC component (master)

    \Notes
        None

    \Copyright
        Copyright (c) Electronic Arts 2012.
        ALL RIGHTS RESERVED.


    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/easfc/mastermetrics.h#1 $

*/
/*************************************************************************************************H*/

#ifndef BLAZE_EASFC_MASTERMETRICS_H
#define BLAZE_EASFC_MASTERMETRICS_H

/*** Include files *******************************************************************************/

#include "framework/util/counter.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class ComponentStatus;

namespace EASFC
{

class MasterMetrics
{
    NON_COPYABLE(MasterMetrics);

public: // Functions used by the Easfc slave impl.

    //! Constructor. Allocates per-type counters and sets all data to default values.
    MasterMetrics();

    //! Destructor. Cleans up allocated memory.
    ~MasterMetrics();

    //! Reports all metrics to the specified status object.
    void report(ComponentStatus* status) const;

public: // Metric variables.

    //! Number of game wins purchased
    Counter     mTotalPurchaseGameWin;

	//! Number of game draw purchased
	Counter     mTotalPurchaseGameDraw;

	//! Number of game loss purchased
	Counter     mTotalPurchaseGameLoss;

	//! Number of game match purchased
	Counter     mTotalPurchaseGameMatch;

private:

    typedef eastl::vector<Counter*> CounterList;
    //! List of all metrics.
    CounterList mCounterList;

};

} // namespace Easfc
} // namespace Blaze

#endif // BLAZE_EASFC_MASTERMETRICS_H
