/*************************************************************************************************/
/*!
    \file   dynamicinetfilter.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_DYNAMICINETFILTER_DYNAMICINETFILTER_H
#define BLAZE_DYNAMICINETFILTER_DYNAMICINETFILTER_H

/*** Include files *******************************************************************************/

#include "framework/connection/inetfilter.h"

#include "framework/dynamicinetfilter/tdf/dynamicinetfilter.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace DynamicInetFilter
{

class Filter
{
public:
    Filter(){}
    virtual ~Filter(){}

    virtual bool match(const char8_t* group, uint32_t ip) const = 0;
    virtual size_t getCount(const char8_t* group) const = 0;
    virtual const InetFilter::FilterList* getGroupFilter(const char8_t* group) const = 0;
};


} // DynamicInetFilter
extern EA_THREAD_LOCAL DynamicInetFilter::Filter* gDynamicInetFilter;

} // Blaze

#endif // BLAZE_DYNAMICINETFILTER_DYNAMICINETFILTER_H

