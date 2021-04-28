/*H*************************************************************************************************/
/*!

    \File    MasterMetrics.cpp

    \Description
        Health metrics helper for Easfc master

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2010.
        ALL RIGHTS RESERVED.

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/easfc/mastermetrics.cpp#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $


*/
/*************************************************************************************************H*/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "easfc/mastermetrics.h"

//#include "framework/tdf/controllertypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace EASFC
{

/*** Static C-Style Methods **********************************************************************/

/*** Public Methods ******************************************************************************/

//! Constructor. Allocates per-type counters and sets all data to default values.
MasterMetrics::MasterMetrics() :
    mTotalPurchaseGameWin("TotalPurchaseGameWin"),
	mTotalPurchaseGameDraw("TotalPurchaseGameDraw"),
	mTotalPurchaseGameLoss("TotalPurchaseGameLoss"),
	mTotalPurchaseGameMatch("TotalPurchaseGameMatch"),
    mCounterList()
{
    mCounterList.push_back(&mTotalPurchaseGameWin);
	mCounterList.push_back(&mTotalPurchaseGameDraw);
	mCounterList.push_back(&mTotalPurchaseGameLoss);
	mCounterList.push_back(&mTotalPurchaseGameMatch);
}

//! Destructor. Cleans up allocated memory.
MasterMetrics::~MasterMetrics()
{
}

//! Reports all metrics to the specified status object.
void MasterMetrics::report(Blaze::ComponentStatus* status) const
{
    Blaze::ComponentStatus::InfoMap &statusMap = status->getInfo();

    CounterList::const_iterator iterator = mCounterList.begin();
    CounterList::const_iterator end = mCounterList.end();
    while (iterator != end)
    {
        Counter *counter = *iterator;
        const char8_t *strKey = counter->getName();
        char8_t tempValueBuffer[64];
        counter->getCountAsString(tempValueBuffer, sizeof(tempValueBuffer));
        statusMap[strKey] = tempValueBuffer;

        ++iterator;
    }
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // namespace EASFC
} // namespace Blaze
