/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_INETFILTER_H
#define BLAZE_INETFILTER_H

/*** Include files *******************************************************************************/

#include "EASTL/vector.h"
#include "EASTL/intrusive_ptr.h"
#include "framework/connection/inetaddress.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class InetFilter
{
    NON_COPYABLE(InetFilter);

public:
    struct Filter
    {
        uint32_t ip;
        uint32_t mask;

        void setContent(const uint32_t aIP, const uint32_t aMask)
        {
            ip = (aIP & aMask);
            mask = aMask;
        }
    };

    typedef eastl::vector<Filter> FilterList;
    typedef eastl::list<eastl::string> DynamicFilterGroupList;
    
    InetFilter() : mAllowIfEmpty(false), mRefCount(0) { }

    /*! \brief Initialize this filter from the list of tdf strings.
     */
    bool initialize(bool allowIfEmpty, const NetworkFilterConfig& config);

    /*! \brief Test if the given address is a match to the filter. */
    bool match(uint32_t addr) const;

    /*! \brief Test if the given address is a match to the filter. */
    bool match(const InetAddress& addr) const { return match(addr.getIp()); }

    /*! \brief Return number of entries in the filter. */
    size_t getCount() const;

    /*! \brief Return list of static filter entries in the filter. */
    const FilterList& getFilters() const { return mFilters; }

    /*! \brief Return list of names of the dynamic group entries in the filter. */
    const DynamicFilterGroupList& getDynamicFilterGroups() const { return mDynamicFilterGroups; };

private:
    FilterList mFilters;
    DynamicFilterGroupList mDynamicFilterGroups;
    bool mAllowIfEmpty;
    
    bool parseFilter(const char8_t* filter, uint32_t& ip, uint32_t& mask) const;

    uint32_t mRefCount;

    friend void intrusive_ptr_add_ref(InetFilter* ptr);
    friend void intrusive_ptr_release(InetFilter* ptr);
};

typedef eastl::intrusive_ptr<InetFilter> InetFilterPtr;

inline void intrusive_ptr_add_ref(InetFilter* ptr)
{
    ptr->mRefCount++;
}

inline void intrusive_ptr_release(InetFilter* ptr)
{
    if (--ptr->mRefCount == 0)
        delete ptr;
}

} // Blaze

#endif // BLAZE_INETFILTER_H

