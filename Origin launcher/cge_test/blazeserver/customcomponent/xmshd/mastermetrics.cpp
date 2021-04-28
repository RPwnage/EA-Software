/*H*************************************************************************************************/
/*!

    \File    MasterMetrics.cpp

    \Description
        Health metrics helper for XmsHd master

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2013.
        ALL RIGHTS RESERVED.

    $Header: //gosdev/games/FIFA/2014/Gen3/DEV/blazeserver/13.x/customcomponent/xmshd/mastermetrics.cpp#1 $


*/
/*************************************************************************************************H*/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "xmshd/mastermetrics.h"

#include "framework/tdf/controllertypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace XmsHd
{

/*** Static C-Style Methods **********************************************************************/

/*** Public Methods ******************************************************************************/

//! Constructor. Allocates per-type counters and sets all data to default values.
MasterMetrics::MasterMetrics() :
    mTotalPublishData("TotalPublishData"),
    mCounterList()
{
    mCounterList.push_back(&mTotalPublishData);
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

} // namespace XmsHd
} // namespace Blaze
