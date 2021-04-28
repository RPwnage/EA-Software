/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class InetFilter

    This class provides a means to filter IP addresses based on subnets.  Configuration is
    obtained from a config file sequence. It can contain network/mask pairs and references to
    groups within the dynamic InetFilter database.  Example syntax is:

        filter = [ 192.168.0.0/16, 10.0.0.0/8, DynamicInetFilterGroup:MyGroup, 127.0.0.1 ]

    The filter can be setup to default to either allow or deny functionality when no filters are
    provided in the config map/database using the allowIfEmpty parameter to the initialize function.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/inetfilter.h"
#include "framework/connection/platformsocket.h"
#include "framework/controller/controller.h"
#include "framework/dynamicinetfilter/dynamicinetfilter.h"
#include "framework/dynamicinetfilter/tdf/dynamicinetfilter_server.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
     \brief initialize
 
     Initialize this instance using the given config sequence.  See the class description for the
     syntax of the sequence elements.
  
     \param[in] allowIfEmpty - If the sequence is empty - or the sequence only references dynamic
                               InetFilter groups and those groups are empty in the database - then
                               this setting controls whether the filter will provide
                               a positive match for any given IP.
     \param[in] config - List of network/netmask pairs and references to dynamic InetFilter groups as strings.
 
     \return - true on successful parsing of the sequence; false otherwise

*/
/*************************************************************************************************/
bool InetFilter::initialize(bool allowIfEmpty, const NetworkFilterConfig& config)
{
    mAllowIfEmpty = allowIfEmpty;
    mFilters.clear();
    mDynamicFilterGroups.clear();

    const char8_t* dynamicFilterPrefix = "DynamicInetFilterGroup:";
    unsigned dynamicFilterPrefixLen = strlen(dynamicFilterPrefix);

    // Load subnet descriptions and dynamic InetFilter group references from config file
    for (NetworkFilterConfig::const_iterator itr = config.begin(), end = config.end(); itr != end; ++itr)
    {
        if (!memcmp(itr->c_str(), dynamicFilterPrefix, dynamicFilterPrefixLen))
        {
            // This is a reference to a group in the DynamicInetFilter

            mDynamicFilterGroups.push_back(itr->c_str() + dynamicFilterPrefixLen);
        }
        else
        {
            // This is a straight IP filter specification

            uint32_t ip;
            uint32_t mask;

            if (!parseFilter(itr->c_str(), ip, mask))
                return false;

            mFilters.push_back();
            Filter& f = mFilters.back();
            f.setContent(ip, mask);
        }
    }

    return true;
}

/*************************************************************************************************/
/*!
     \brief match
 
     Test if the given IP address is a match to this filter.
  
     \param[in] addr - Address to check match for (in network byte order).
 
     \return - true if address is a match; false otherwise.

*/
/*************************************************************************************************/
bool InetFilter::match(uint32_t addr) const
{
    // If AllowIfEmpty is set, then zero filters set -> allow anything
    if (mAllowIfEmpty && !getCount())
        return true;

    // Match IP against the static filters
    FilterList::const_iterator i = mFilters.begin();
    FilterList::const_iterator e = mFilters.end();
    for(; i != e; ++i)
    {
        const Filter& filter = *i;
        if ((addr & filter.mask) == filter.ip)
        {
            return true;
        }
    }

    // If no match, run IP against any dynamic filters
    if (!mDynamicFilterGroups.empty() && gDynamicInetFilter)
    {
        for (DynamicFilterGroupList::const_iterator group = mDynamicFilterGroups.begin(),
            groupEnd = mDynamicFilterGroups.end();
            group != groupEnd;
            ++group)
        {
            if (gDynamicInetFilter->match(group->c_str(), addr))
            {
                return true;
            }
        }
    }

    // No match found
    return false;
}

/*************************************************************************************************/
/*!
     \brief getCount
 
     Return number of entries (static + dynamic) in this InetFilter
  
     \return - number of entries

*/
/*************************************************************************************************/
size_t InetFilter::getCount() const
{
    size_t count = mFilters.size();

    if (!mDynamicFilterGroups.empty() && gDynamicInetFilter)
    {
        for (DynamicFilterGroupList::const_iterator group = mDynamicFilterGroups.begin(), end = mDynamicFilterGroups.end();
            group != end;
            ++group)
            count += gDynamicInetFilter->getCount(group->c_str());
    }

    return count;
}

/*** Private Methods *****************************************************************************/

bool InetFilter::parseFilter(const char8_t* filter, uint32_t& ip, uint32_t& mask) const
{
    if (filter == nullptr)
        return false;

    ip = 0;
    mask = 0;

    // Parse IP address
    char8_t* curr = const_cast<char8_t*>(filter);
    char8_t* prefix = blaze_strstr(curr, "/");
    size_t ipLen = 0;
    
    if(prefix != nullptr)  
        ipLen = static_cast<size_t>(prefix - curr);     
    else
        ipLen = strlen(curr);

    char8_t ipAddr[32] = "";
    blaze_strnzcpy(ipAddr, curr, ipLen + 1);
    if (!InetAddress::parseDottedDecimal(ipAddr, ip, InetAddress::NET))
        return false;
    curr = curr + ipLen;

    // Parse mask
    if (*curr != '\0')
    {
        ++curr;
        char8_t* next;
        int32_t val = strtol(curr, &next, 10);
        if (curr == next)
            return false;

        if ((val < 0) || (val > 32))
            return false;
        if (val == 32)
            mask = 0xffffffff;
        else
            mask = ((1 << val) - 1) << (32 - val);
        curr = next;
    }
    else
        mask = 0xffffffff;

    if (*curr != '\0')
        return false;

    mask = htonl(mask);
    return true;
}

} // Blaze

