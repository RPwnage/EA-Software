/*************************************************************************************************/
/*!
\file


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/shared/collections.h"

namespace Blaze
{

namespace Collections
{
    /*** Defines/Macros/Constants/Typedefs ***********************************************************/

    /*** Public Methods ******************************************************************************/
    void upsertAttributeMap(AttributeMap& dest, const AttributeMap& source)
    {
        AttributeMap::const_iterator fromItr = source.begin();
        for (;fromItr != source.end(); fromItr++)
        {
            dest[fromItr->first] = fromItr->second;
        }
    }

    /*** Protected Methods ***************************************************************************/

    /*** Private Methods *****************************************************************************/

} // namespace Collections

} // namespace Blaze

