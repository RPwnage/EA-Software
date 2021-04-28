/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include "framework/blaze.h"
#include "framework/protocol/shared/heat2util.h"
#include "framework/util/shared/rawbuffer.h"

#include "EATDF/printencoder.h"
#include "EASTL/algorithm.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/shared/framework/util/blazestring.h"
#else
#include "framework/util/shared/blazestring.h"
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

// Static


Heat2Util::HeatType Heat2Util::getHeatTypeFromTdfType(EA::TDF::TdfType type)
{
    switch (type)
    {
        case EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE:       return HEAT_TYPE_GENERIC_TYPE;
        case EA::TDF::TDF_ACTUAL_TYPE_TDF:                return HEAT_TYPE_TDF; 
        case EA::TDF::TDF_ACTUAL_TYPE_UNION:              return HEAT_TYPE_UNION; 
        case EA::TDF::TDF_ACTUAL_TYPE_MAP:                return HEAT_TYPE_MAP; 
        case EA::TDF::TDF_ACTUAL_TYPE_LIST:               return HEAT_TYPE_LIST;
        case EA::TDF::TDF_ACTUAL_TYPE_BLOB:               return HEAT_TYPE_BLOB; 
        case EA::TDF::TDF_ACTUAL_TYPE_VARIABLE:           return HEAT_TYPE_VARIABLE;
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_TYPE:        return HEAT_TYPE_OBJECT_TYPE; 
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_ID:          return HEAT_TYPE_OBJECT_ID;
        case EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE:          return HEAT_TYPE_TIMEVALUE;
        case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:              return HEAT_TYPE_FLOAT;
        case EA::TDF::TDF_ACTUAL_TYPE_STRING:             return HEAT_TYPE_STRING;         
        case EA::TDF::TDF_ACTUAL_TYPE_ENUM:               return HEAT_TYPE_INTEGER;
        case EA::TDF::TDF_ACTUAL_TYPE_BITFIELD:           return HEAT_TYPE_INTEGER; 
        case EA::TDF::TDF_ACTUAL_TYPE_BOOL:               
        case EA::TDF::TDF_ACTUAL_TYPE_INT8:               
        case EA::TDF::TDF_ACTUAL_TYPE_UINT8:              
        case EA::TDF::TDF_ACTUAL_TYPE_INT16:              
        case EA::TDF::TDF_ACTUAL_TYPE_UINT16:             
        case EA::TDF::TDF_ACTUAL_TYPE_INT32:              
        case EA::TDF::TDF_ACTUAL_TYPE_UINT32:             
        case EA::TDF::TDF_ACTUAL_TYPE_INT64:              
        case EA::TDF::TDF_ACTUAL_TYPE_UINT64:             
            return HEAT_TYPE_INTEGER;
        case EA::TDF::TDF_ACTUAL_TYPE_UNKNOWN:    
        default:
            return HEAT_TYPE_INVALID;
    }
}

Heat2Util::HeatType Heat2Util::getCollectionHeatTypeFromTdfType(EA::TDF::TdfType type, bool heat1BackCompat)
{
    switch (type)
    {
        // Heat1 had a bug where struct vectors misencode the data types of unions, maps, and lists as TDF.
        // We  optionally maintain this bug until all clients have updated to a post-Urraca version, where we can remove this check. 
        case EA::TDF::TDF_ACTUAL_TYPE_TDF:                return HEAT_TYPE_TDF;
        case EA::TDF::TDF_ACTUAL_TYPE_UNION:              return heat1BackCompat ? HEAT_TYPE_TDF : HEAT_TYPE_UNION;
        case EA::TDF::TDF_ACTUAL_TYPE_MAP:                return heat1BackCompat ? HEAT_TYPE_TDF : HEAT_TYPE_MAP;
        case EA::TDF::TDF_ACTUAL_TYPE_LIST:               return heat1BackCompat ? HEAT_TYPE_TDF : HEAT_TYPE_LIST;
        case EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE:       return HEAT_TYPE_GENERIC_TYPE;

        case EA::TDF::TDF_ACTUAL_TYPE_BLOB:               return HEAT_TYPE_BLOB; 
        case EA::TDF::TDF_ACTUAL_TYPE_VARIABLE:           return HEAT_TYPE_VARIABLE;
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_TYPE:        return HEAT_TYPE_OBJECT_TYPE; 
        case EA::TDF::TDF_ACTUAL_TYPE_OBJECT_ID:          return HEAT_TYPE_OBJECT_ID;
        case EA::TDF::TDF_ACTUAL_TYPE_TIMEVALUE:          return HEAT_TYPE_TIMEVALUE;
        case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:              return HEAT_TYPE_FLOAT;
        case EA::TDF::TDF_ACTUAL_TYPE_STRING:             return HEAT_TYPE_STRING;         
        case EA::TDF::TDF_ACTUAL_TYPE_ENUM:               return HEAT_TYPE_INTEGER;
        case EA::TDF::TDF_ACTUAL_TYPE_BITFIELD:           return HEAT_TYPE_INTEGER; 
        case EA::TDF::TDF_ACTUAL_TYPE_BOOL:               
        case EA::TDF::TDF_ACTUAL_TYPE_INT8:               
        case EA::TDF::TDF_ACTUAL_TYPE_UINT8:              
        case EA::TDF::TDF_ACTUAL_TYPE_INT16:              
        case EA::TDF::TDF_ACTUAL_TYPE_UINT16:             
        case EA::TDF::TDF_ACTUAL_TYPE_INT32:              
        case EA::TDF::TDF_ACTUAL_TYPE_UINT32:             
        case EA::TDF::TDF_ACTUAL_TYPE_INT64:              
        case EA::TDF::TDF_ACTUAL_TYPE_UINT64:             
            return HEAT_TYPE_INTEGER;
        case EA::TDF::TDF_ACTUAL_TYPE_UNKNOWN:    
        default:
            return HEAT_TYPE_INVALID;
    }
}

uint32_t Heat2Util::makeTag(const char* tag)
{
    if (tag == nullptr)
        return 0;

    char tmptag[MAX_TAG_LENGTH];
    *tmptag = '\0';

    // ensure tag string is valid
    size_t len = strlen(tag);
    if (len > MAX_TAG_LENGTH)
        return 0;
    if (len < 1)
        return 0;

    for (uint32_t idx = 0; idx < len; idx++)
    {
        tmptag[idx] = (char)toupper(tag[idx]);

        // Check valid range for tag characters
        if ((tmptag[idx] < TAG_CHAR_MIN) || (tmptag[idx] > TAG_CHAR_MAX))
            return 0;
    }       

    if ((tmptag[0] < 'A') || (tmptag[0] > 'Z'))
        return 0;;

    uint32_t t = static_cast<uint32_t>(((tmptag[0] - TAG_CHAR_MIN) & 0x3f) << 26);
    if (len > 1)
    {
        t |= (((tmptag[1] - TAG_CHAR_MIN) & 0x3f) << 20);
        if (len > 2)
        {
            t |= (((tmptag[2] - TAG_CHAR_MIN) & 0x3f) << 14);
            if (len > 3)
            { 
                t |= ((tmptag[3] - TAG_CHAR_MIN) & 0x3f) << 8;
            }
        }
    }
    return t;
}


// Static
int32_t Heat2Util::decodeTag(uint32_t tag, char *buf, uint32_t len, const bool convertToLowercase /*= false*/)
{
    if (buf == nullptr || len < 5)
        return 0;

    int32_t size = 4;
    tag >>= 8;
    for (int32_t i = 3; i >= 0; --i)
    {
        uint32_t sixbits = tag & 0x3f;
        if (sixbits)
            buf[i] = (char)(sixbits + 32);
        else
        {
            buf[i] = '\0';
            size = i;
        }
        tag >>= 6;
    }
    buf[4] = '\0';

    // Only convert the tag itself, no need to convert the full buffer length.
    if (convertToLowercase)
    {
        for (uint32_t i = 0 ; i < 4 ; ++i)
        {
            buf[i] = static_cast<char>(tolower(buf[i]));
        }
    }

    return size;
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/


} // Blaze

