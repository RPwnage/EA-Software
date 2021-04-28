/*************************************************************************************************/
/*!
    \file   dynamicinetfilterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DynamicInetFilterImpl

    Implements per-group-sorted InetFilters
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/dynamicinetfilter/tdf/dynamicinetfilter.h"

#include "framework/dynamicinetfilter/dynamicinetfilterimpl.h"

namespace Blaze
{
namespace DynamicInetFilter
{
/*** Static methods **************************************************************************/

bool DynamicInetFilterImpl::generateFilter(const CidrBlock& subNet, InetFilter::Filter& filter)
{
    uint32_t prefixLength = subNet.getPrefixLength();
    uint32_t ip;
    if (!InetAddress::parseDottedDecimal(subNet.getIp(), ip, InetAddress::NET))
        return false;

    uint32_t mask = htonl(prefixLength ? (0xffffffffU << (32 - prefixLength)) : 0);
    filter.setContent(ip, mask);
    return true;
}

bool DynamicInetFilterImpl::validateString(const char8_t* str, bool allowEmptyString)
{
    if (!allowEmptyString && !str[0])
        return false;

    while (*str)
    {
        char c = *str++;
        if (c < 32 || (unsigned char) c > 127)
            return false;
    }

    return true;
}

bool DynamicInetFilterImpl::validateSubNet(const CidrBlock& subNet, bool allowEmptySubNet)
{
    if (allowEmptySubNet && subNet.getPrefixLength() == INVALID_PREFIX_LENGTH)
        return true;
    else
    {
        uint32_t dummy;
        return InetAddress::parseDottedDecimal(subNet.getIp(), dummy)
            && subNet.getPrefixLength() <= MAX_PREFIX_LENGTH
            && subNet.getPrefixLength() != INVALID_PREFIX_LENGTH;
    }
}

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief insertFilterGroupEntry

    Add an entry to the set of filter groups; create entry if it doesn't already exist

    \param[in] entry - entry to insert into set of groups

*/
/*************************************************************************************************/
void DynamicInetFilterImpl::insertFilterGroupEntry(EntryPtr entry)
{
    ObjectId objectId = entry->getRowId();

    InetFilterGroup* group = nullptr;
    bool foundGroup = false;

    // Locate an existing group, which this entry should be inserted into
    for (InetFilterGroups::iterator groupIt = mInetFilterGroups.begin(),
        end = mInetFilterGroups.end();
        groupIt != end;
        ++groupIt)
    {
        if (!strcmp(groupIt->mGroup.c_str(), entry->getGroup()))
        {
            group = &*groupIt;
            foundGroup = true;
            break;
        }
    }

    // If there is no existing group, create a new (empty) group
    if (!group)
    {
        mInetFilterGroups.push_back();
        group = &mInetFilterGroups.back();
        group->mGroup = entry->getGroup();
    }

    // Insert item into group
    uint32_t index = group->mFilters.size();
    bool replacingRowIdEntry = (group->mObjectIdToIndexMap.find(objectId) != group->mObjectIdToIndexMap.end());
    group->mObjectIdToIndexMap[objectId] = index;
    group->mEntries.push_back(entry);

    InetFilter::Filter filter;
    generateFilter(entry->getSubNet(), filter);
    group->mFilters.push_back(filter);

    BLAZE_TRACE_LOG(Log::DYNAMIC_INET, "[DynamicInetFilterImpl].insertFilterGroupEntry: Added entry to" << 
        (foundGroup ? "" : " new") << " group (" << entry->getGroup() << ") -" <<
        (replacingRowIdEntry ? "REPLACING" : "") << " Row Id (" << entry->getRowId() <<") with entry. " <<
        "SubNet: " << entry->getSubNet().getIp() << ", " <<
        "Owner: " << entry->getOwner() << ", " <<
        "Comment: " << entry->getComment() << ".");
}

/*************************************************************************************************/
/*!
    \brief eraseFilterGroupEntry

    Remove an entry to the set of filter groups; also remove any group that becomes empty

    \param[in] objectId - unique ID for the object to erase (same as the entry's rowId)
*/
/*************************************************************************************************/
void DynamicInetFilterImpl::eraseFilterGroupEntry(ObjectId objectId)
{
    // Iterate through all groups, looking for the entry with appropriate objectId, and erase it when found
    for (InetFilterGroups::iterator groupIt = mInetFilterGroups.begin(),
        end = mInetFilterGroups.end();
        groupIt != end;
        ++groupIt)
    {
        // Look for objectId within current group
        InetFilterGroup::ObjectIdToIndexMap::iterator it = groupIt->mObjectIdToIndexMap.find(objectId);
        if (it != groupIt->mObjectIdToIndexMap.end())
        {
            // Is this the last entry in the group?
            if (groupIt->mFilters.size() == 1)
            {
                // Erase both entry and group
                mInetFilterGroups.erase(groupIt);
                return;
            }
            else
            {
                uint32_t index = it->second;
                uint32_t lastIndex = groupIt->mFilters.size() - 1;

                // If entry isn't at the last position in group, then swap it with the last entry
                if (index != lastIndex)
                {
                    InetFilterGroup::ObjectIdToIndexMap::iterator lastIt = groupIt->mObjectIdToIndexMap.find(groupIt->mEntries[lastIndex]->getRowId());
                    groupIt->mFilters[index] = groupIt->mFilters[lastIndex];
                    groupIt->mEntries[index] = groupIt->mEntries[lastIndex];
                    lastIt->second = it->second;
                }

                // Entry is now guaranteed to be at the last position in the group

                // Remove entry from in-memory datastructures
                groupIt->mFilters.pop_back();
                groupIt->mEntries.pop_back();
                groupIt->mObjectIdToIndexMap.erase(it);
                return;
            }
        }
    }

    BLAZE_WARN_LOG(Log::SYSTEM, "Object with id " << objectId << " doesn't exist in any FilterGroup map - replicated data and local data are probably inconsistent");
}

/*************************************************************************************************/
/*!
    \brief clearFilterGroups

    Erase all filtergroups
*/
/*************************************************************************************************/
void DynamicInetFilterImpl::clearFilterGroups()
{
    mInetFilterGroups.clear();
}

/*************************************************************************************************/
/*!
    \brief getFilterGroupEntry

    Get the specified entry by objectId 

    \param[in] objectId - unique ID for the object(same as the entry's rowId)
*/
/*************************************************************************************************/
EntryPtr DynamicInetFilterImpl::getFilterGroupEntry(ObjectId objectId)
{
    EntryPtr entry = nullptr;

    // Iterate through all groups, looking for the entry with appropriate objectId
    for (InetFilterGroups::iterator groupIt = mInetFilterGroups.begin(),
        end = mInetFilterGroups.end();
        groupIt != end;
        ++groupIt)
    {
        // Look for objectId within current group
        InetFilterGroup::ObjectIdToIndexMap::iterator it = groupIt->mObjectIdToIndexMap.find(objectId);
        if (it != groupIt->mObjectIdToIndexMap.end())
        {
            uint32_t index = it->second;
            entry = groupIt->mEntries[index];
            break;
        }
    }

    return entry;
}

/*************************************************************************************************/
/*!
    \brief match

    Matches a single IP address against an InetFilterGroup

    \param[in] group - name of group which should be matched against the IP
    \param[in] ip - IP address to match, in network byte order

    \result - true if any subnet in the group matches the IP, false if not
*/
/*************************************************************************************************/
bool DynamicInetFilterImpl::match(const char8_t* group, uint32_t ip) const
{
    // Iterate through all groups
    for (InetFilterGroups::const_iterator groupIt = mInetFilterGroups.begin(),
        end = mInetFilterGroups.end();
        groupIt != end;
        ++groupIt)
    {
        // Is this the group we're looking for?
        if (!strcmp(groupIt->mGroup.c_str(), group))
        {
            // Yes, so iterate through all IP filters in group
            for (InetFilterGroup::Filters::const_iterator filter = groupIt->mFilters.begin(),
                filterEnd = groupIt->mFilters.end();
                filter != filterEnd;
                ++filter)
            {
                // Is the supplied IP within the range of the IP filter?
                if ((ip & filter->mask) == filter->ip)
                    return true;    // Yes
            }
        }
    }

    // No matching IP filter found
    return false;
}

/*************************************************************************************************/
/*!
    \brief getCount

    Returns number of subnets that exist in the specified group

    \param[in] group - name of group to look for

    \result - number of subnets with that specific group
*/
/*************************************************************************************************/
size_t DynamicInetFilterImpl::getCount(const char8_t* group) const
{
    // Iterate through all groups
    for (InetFilterGroups::const_iterator groupIt = mInetFilterGroups.begin(),
        end = mInetFilterGroups.end();
        groupIt != end;
        ++groupIt)
    {
        // Is this the group we're looking for?
        if (!strcmp(groupIt->mGroup.c_str(), group))
        {
            // Yes, return number of entries in group
            return groupIt->mFilters.size();
        }
    }

    // Group not found, thus zero matching entries available
    return 0;
}

const InetFilter::FilterList* DynamicInetFilterImpl::getGroupFilter(const char8_t* group) const
{
    // Iterate through all groups
    for (InetFilterGroups::const_iterator groupIt = mInetFilterGroups.begin(),
        end = mInetFilterGroups.end();
        groupIt != end;
    ++groupIt)
    {
        // Is this the group we're looking for?
        if (!strcmp(groupIt->mGroup.c_str(), group))
        {
            return &groupIt->mFilters;
        }
    }

    return nullptr;
}

}
} // Blaze

