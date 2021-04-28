/*************************************************************************************************/
/*!
    \file

    
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_PMAP_H
#define BLAZE_PMAP_H

/*** Include files *******************************************************************************/
#include "EASTL/map.h"
#include "EASTL/utility.h"  // Needed for make_pair()
#include "framework/util/less_ptr.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

template<class K, class V>
struct pmap
{
    typedef eastl::map< const K,  V, less_ptr<K> > Type;
};

template<typename K, typename V>
struct pmultimap
{
    typedef eastl::multimap< const K,  V, less_ptr<K> > Type;
};

} // Blaze
#endif // BLAZE_PMAP_H
