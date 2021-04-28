/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_INTERVALMAP_H
#define BLAZE_INTERVALMAP_H

/*** Include files *******************************************************************************/

#include "EASTL/vector.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class IntervalMap
{
    NON_COPYABLE(IntervalMap);

public:
    IntervalMap(int32_t start = 0, int32_t end = INT32_MAX);
    ~IntervalMap();

    int32_t allocate();
    bool release(int32_t id);

    char8_t* print(char8_t* buf, uint32_t len);

    struct IntervalData
    {
        int32_t start;
        int32_t size;
    };
    typedef eastl::vector<IntervalData> IntervalDataList;

    struct IntervalMapData
    {
        IntervalDataList mDataList;
        uint32_t mIndexOfNext;
    };

    void exportIntervals(IntervalMapData& mapData);
    void importIntervals(const IntervalMapData& mapData);

private:
    void init();

    struct Interval : public IntervalData
    {
        Interval *next;
    };

    Interval* mHead;
    Interval* mNext;
    int32_t mStart;
    int32_t mEnd;
};

} // Blaze

#endif // BLAZE_INTERVALMAP_H

