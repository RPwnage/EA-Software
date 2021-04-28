/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_HEAT2UTIL_H
#define BLAZE_HEAT2UTIL_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class RawBuffer;

class BLAZESDK_API Heat2Util
{
public:
    const static int32_t ELEMENT_TYPE_MASK = 0x1f; // 5 bits

    const static uint8_t ID_TERM = 0x00;

    const static uint32_t VARSIZE_MORE = 0x80;
    const static uint32_t VARSIZE_NEGATIVE = 0x40;

    const static uint32_t VARSIZE_MAX_ENCODE_SIZE = 10;
    const static uint32_t FLOAT_SIZE = 4;

    const static uint32_t HEADER_TYPE_OFFSET = 3;

    // Length of an attribute header
    const static uint32_t HEADER_SIZE = 4; 

    const static uint32_t MAX_TAG_LENGTH = 4;
    const static int32_t TAG_CHAR_MIN = 32;
    const static int32_t TAG_CHAR_MAX = 95;
    const static uint32_t TAG_UNSPECIFIED = 0;

    //These are currently used by the HEAT codec.  These should not be used in general practice.
    typedef enum
    {
        HEAT_TYPE_MIN = 0,

        HEAT_TYPE_INTEGER = HEAT_TYPE_MIN, 
        HEAT_TYPE_STRING = 1, 
        HEAT_TYPE_BLOB = 2,
        HEAT_TYPE_TDF = 3,
        HEAT_TYPE_LIST = 4, 
        HEAT_TYPE_MAP = 5, 
        HEAT_TYPE_UNION = 6, 
        HEAT_TYPE_VARIABLE = 7, 
        HEAT_TYPE_OBJECT_TYPE = 8, 
        HEAT_TYPE_OBJECT_ID = 9, 
        HEAT_TYPE_FLOAT = 10, 
        HEAT_TYPE_TIMEVALUE = 11,
        HEAT_TYPE_GENERIC_TYPE = 12,

        HEAT_TYPE_INVALID, HEAT_TYPE_MAX = HEAT_TYPE_INVALID, 
    } HeatType;

    static HeatType getHeatTypeFromTdfType(EA::TDF::TdfType type);
    static HeatType getCollectionHeatTypeFromTdfType(EA::TDF::TdfType type, bool heat1BackCompat = false);
    
    static uint32_t makeTag(const char* tag);
    static int32_t decodeTag(uint32_t tag, char* buf, uint32_t len, const bool convertToLowercase = false);

    template<typename VisitContext>
    static const EA::TDF::TdfMemberInfo* getContextSpecificMemberInfo(const VisitContext& context); 
};

// This method is a template because TdfVisitContext and TdfVisitContextConst are unrelated mirror types, ugh!
template<typename VisitContext>
const EA::TDF::TdfMemberInfo* Heat2Util::getContextSpecificMemberInfo(const VisitContext& context)
{
    const EA::TDF::TdfMemberInfo* memberInfo = context.getMemberInfo();
    const VisitContext* parentContext = context.getParent();
    if (parentContext != nullptr)
    {
        const EA::TDF::TdfType parentType = parentContext->getType();
        if (parentType == EA::TDF::TDF_ACTUAL_TYPE_UNION)
        {
            const EA::TDF::TdfMemberInfo* parentMemberInfo = parentContext->getMemberInfo();
            if (parentMemberInfo == nullptr)
            {
                // IMPORTANT: There is an inconsistency in HEAT2 union encoding whereby unions that are elements of
                // collections do not encode their member's header, whereas union members of a tdf always do;
                // therefore, until this is rectified with HEAT3, when the parent is a union, we need to
                // check its memberInfo which will only be set if the union itself is a member of a tdf.
                memberInfo = nullptr;
            }
        }
        else if ((parentType == EA::TDF::TDF_ACTUAL_TYPE_VARIABLE) || 
            (parentType == EA::TDF::TDF_ACTUAL_TYPE_GENERIC_TYPE))
        {
            const EA::TDF::TdfMemberInfo* parentMemberInfo = parentContext->getMemberInfo();
            if (parentMemberInfo != nullptr)
            {
                // IMPORTANT: There is an additional inconsistency in HEAT2 Variable and GenericType encoding whereby
                // the internal member of VariableTdf and GenericType reuses the 'tag' of its parent object 
                // (if the parent is not an element in a container, otherwise same logic as above for unions applies)
                memberInfo = parentMemberInfo;
            }
        }
    }
    return memberInfo;
}

} // Blaze

#endif // BLAZE_HEAT2UTIL_H

