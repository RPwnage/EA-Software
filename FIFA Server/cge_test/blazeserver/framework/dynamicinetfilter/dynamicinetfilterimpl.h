/*************************************************************************************************/
/*!
    \file   dynamicinetfilterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_DYNAMICINETFILTER_DYNAMICINETFILTERIMPL_H
#define BLAZE_DYNAMICINETFILTER_DYNAMICINETFILTERIMPL_H

/*** Include files *******************************************************************************/
#include "framework/dynamicinetfilter/tdf/dynamicinetfilter.h"
#include "framework/dynamicinetfilter/dynamicinetfilter.h"

#include "EASTL/map.h"
#include "EASTL/vector.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace DynamicInetFilter
{

class DynamicInetFilterImpl : public Filter
{
public:
    DynamicInetFilterImpl(){}
    ~DynamicInetFilterImpl() override{}

    // Inherited from DynamicInetFilter
    bool match(const char8_t* group, uint32_t ip) const override;
    size_t getCount(const char8_t* group) const override;
    const InetFilter::FilterList* getGroupFilter(const char8_t* group) const override;

    // Helper methods
    static bool generateFilter(const CidrBlock& subNet, InetFilter::Filter& filter);
    static bool validateString(const char8_t* str, bool allowEmptyString);
    static bool validateSubNet(const CidrBlock& subNet, bool allowEmptySubNet);

    // Subset of DynamicInetFilter, where all entries share the same group identifier
    struct InetFilterGroup
    {
        typedef eastl::map<ObjectId, uint32_t> ObjectIdToIndexMap;
        typedef eastl::vector<InetFilter::Filter> Filters;
        typedef eastl::vector<EntryPtr> Entries;

        eastl::string      mGroup;                                     // Group identifier for all subnet entries in this object
        ObjectIdToIndexMap mObjectIdToIndexMap;                        // Lookup from ObjectId to an offset into the following eastl::vector<>s
        Filters            mFilters;                                   // IP+mask pairs for the subnet entries
        Entries            mEntries;                                   // EA::TDF::Tdf objects containing full info for the subnet entries
    };

    typedef eastl::list<InetFilterGroup> InetFilterGroups;

    // Retrieve list of all filtergroups
    const InetFilterGroups& getInetFilterGroups() const { return mInetFilterGroups; }

protected:

    // FilterGroupEntry management methods
    void insertFilterGroupEntry(EntryPtr entry);
    void eraseFilterGroupEntry(ObjectId objectId);
    void clearFilterGroups();
    EntryPtr getFilterGroupEntry(ObjectId objectId);

    InetFilterGroups mInetFilterGroups; // Per-group representation of DynamicInetFilter

};

}
} // Blaze

#endif // BLAZE_DYNAMICINETFILTER_DYNAMICINETFILTERIMPL_H

