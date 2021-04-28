/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include <EATDF/tdfvisit.h>

#include <EATDF/tdfvector.h>
#include <EATDF/tdfmap.h>
#include <EATDF/tdfvariable.h>
#include <EAAssert/eaassert.h>

namespace EA
{
namespace TDF
{
 

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/


void TdfVisitor::visitReference(Tdf& rootTdf, Tdf& parentTdf, uint32_t tag, const TdfGeneric& val, const TdfGenericConst* _ref, const TdfMemberInfo::DefaultValue& defaultVal, uint32_t additionalValue)
{
    TdfGenericConst ref = _ref != nullptr ? *_ref : val;
    
    switch (val.getType())
    {
        case TDF_ACTUAL_TYPE_MAP:                visit(rootTdf, parentTdf, tag, val.asMap(), ref.asMap()); break;
        case TDF_ACTUAL_TYPE_LIST:               visit(rootTdf, parentTdf, tag, val.asList(), ref.asList()); break;
        case TDF_ACTUAL_TYPE_FLOAT:              visit(rootTdf, parentTdf, tag, val.asFloat(), ref.asFloat(), defaultVal.floatDefault); break;
        case TDF_ACTUAL_TYPE_ENUM:               visit(rootTdf, parentTdf, tag, val.asEnum(), ref.asEnum(), ref.getTypeDescription().asEnumMap(), static_cast<int32_t>(defaultVal.intDefault.low)); break;
        case TDF_ACTUAL_TYPE_STRING:             visit(rootTdf, parentTdf, tag, val.asString(), ref.asString(), defaultVal.stringDefault != nullptr ? defaultVal.stringDefault : "", additionalValue); break;
        case TDF_ACTUAL_TYPE_VARIABLE:           visit(rootTdf, parentTdf, tag, val.asVariable(), ref.asVariable()); break;
        case TDF_ACTUAL_TYPE_GENERIC_TYPE:       visit(rootTdf, parentTdf, tag, val.asGenericType(), ref.asGenericType()); break;
        case TDF_ACTUAL_TYPE_BITFIELD:           visit(rootTdf, parentTdf, tag, val.asBitfield(), ref.asBitfield()); break;
        case TDF_ACTUAL_TYPE_BLOB:               visit(rootTdf, parentTdf, tag, val.asBlob(), ref.asBlob()); break;
        case TDF_ACTUAL_TYPE_UNION:              visit(rootTdf, parentTdf, tag, val.asUnion(), ref.asUnion()); break;
        case TDF_ACTUAL_TYPE_TDF:                visit(rootTdf, parentTdf, tag, val.asTdf(), ref.asTdf()); break;
        case TDF_ACTUAL_TYPE_OBJECT_TYPE:        visit(rootTdf, parentTdf, tag, val.asObjectType(), ref.asObjectType()); break;
        case TDF_ACTUAL_TYPE_OBJECT_ID:          visit(rootTdf, parentTdf, tag, val.asObjectId(), ref.asObjectId()); break;
        case TDF_ACTUAL_TYPE_TIMEVALUE:          visit(rootTdf, parentTdf, tag, val.asTimeValue(), ref.asTimeValue(), TimeValue(static_cast<int64_t>(((uint64_t)defaultVal.intDefault.high << 32) | defaultVal.intDefault.low))); break;
        case TDF_ACTUAL_TYPE_BOOL:               visit(rootTdf, parentTdf, tag, val.asBool(), ref.asBool(), defaultVal.intDefault.low != 0); break;
        case TDF_ACTUAL_TYPE_INT8:               
            if (val.getTypeDescription().isIntegralChar())
                visit(rootTdf, parentTdf, tag, (char8_t&)val.asInt8(), (char8_t)ref.asInt8(), static_cast<char8_t>(defaultVal.intDefault.low));
            else
                visit(rootTdf, parentTdf, tag, val.asInt8(), ref.asInt8(), static_cast<int8_t>(defaultVal.intDefault.low));
            break;
        case TDF_ACTUAL_TYPE_UINT8:              visit(rootTdf, parentTdf, tag, val.asUInt8(), ref.asUInt8(), static_cast<uint8_t>(defaultVal.intDefault.low)); break;
        case TDF_ACTUAL_TYPE_INT16:              visit(rootTdf, parentTdf, tag, val.asInt16(), ref.asInt16(), static_cast<int16_t>(defaultVal.intDefault.low)); break;
        case TDF_ACTUAL_TYPE_UINT16:             visit(rootTdf, parentTdf, tag, val.asUInt16(), ref.asUInt16(), static_cast<uint16_t>(defaultVal.intDefault.low)); break;
        case TDF_ACTUAL_TYPE_INT32:              visit(rootTdf, parentTdf, tag, val.asInt32(), ref.asInt32(), static_cast<int32_t>(defaultVal.intDefault.low)); break;
        case TDF_ACTUAL_TYPE_UINT32:             visit(rootTdf, parentTdf, tag, val.asUInt32(), ref.asUInt32(), static_cast<uint32_t>(defaultVal.intDefault.low)); break;
        case TDF_ACTUAL_TYPE_INT64:              visit(rootTdf, parentTdf, tag, val.asInt64(), ref.asInt64(), static_cast<int64_t>(((uint64_t)defaultVal.intDefault.high << 32) | defaultVal.intDefault.low)); break;
        case TDF_ACTUAL_TYPE_UINT64:             visit(rootTdf, parentTdf, tag, val.asUInt64(), ref.asUInt64(), static_cast<uint64_t>(((uint64_t)defaultVal.intDefault.high << 32) | defaultVal.intDefault.low)); break;
        default:
            EA_FAIL_MSG("Cannot visit unknown TdfType");
    }
}

bool TdfMemberVisitor::visitContext(const TdfVisitContext& context)
{
    //First call the visitor on this object
    if (!visitValue(context))
        return false;

    //Now visit the internals, and do any post visit.
    const TdfGenericReference& ref = context.getValue();
    switch (ref.getType())
    {
    case TDF_ACTUAL_TYPE_LIST: 
        {
            pushStack();
            TdfVectorBase& vector = ref.asList();
            vector.visitMembers(*this, context);
            popStack();
        }        
        break;

    case TDF_ACTUAL_TYPE_MAP:
        {
            pushStack();
            TdfMapBase& map = ref.asMap();
            map.visitMembers(*this, context);
            popStack();
        }
        break;

    case TDF_ACTUAL_TYPE_VARIABLE:
        {
            VariableTdfBase& var = ref.asVariable();
            if (var.get() != nullptr)
            {
                pushStack();
                TdfGenericReference tdfRef(*var.get());
                TdfVisitContext c(context, tdfRef);
                visitContext(c);
                popStack();
            }
        }
        break;
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
        {
            GenericTdfType& var = ref.asGenericType();
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            if (var.isSet())
            {
#endif
                pushStack();
                TdfGenericReference tdfRef(var.get());
                TdfVisitContext c(context, tdfRef);
                visitContext(c);           
                popStack();
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            }
#endif
        }
        break;

    case TDF_ACTUAL_TYPE_TDF:
    case TDF_ACTUAL_TYPE_UNION: 
        {
            pushStack();
            Tdf& tdf = ref.asTdf();
            tdf.visitMembers(*this, context);
            popStack();
        }        
        break;

    default:                           
        //no internals
        break;
    }

    if (!postVisitValue(context))
        return false;

    return true;
}

void TdfMemberVisitor::pushStack()
{
    if (mStateDepth >= MAX_STATE_DEPTH)
    {
        //  perhaps we should extend the stack?  better than overwriting memory.
        eastl::string msg;
        msg.append_sprintf("[TdfMemberVisitor].pushStack: Attempted to increment stack past max size of %u", MAX_STATE_DEPTH);
        EA_FAIL_MSG(msg.c_str());
        return;
    }
    ++mStateDepth;
}

void TdfMemberVisitor::popStack()
{
    if (mStateDepth > 0)
    {
        --mStateDepth;
    }
}


bool TdfMemberVisitorConst::visitContext(const TdfVisitContextConst& context)
{
    //First call the visitor on this object
    if (!visitValue(context))
        return false;

    //Now visit the internals, and do any post visit.
    const TdfGenericReferenceConst& ref = context.getValue();
    switch (ref.getType())
    {
    case TDF_ACTUAL_TYPE_LIST: 
        {
            pushStack();
            const TdfVectorBase& vector = ref.asList();
            if (!vector.visitMembers(*this, context))
            {
                popStack();
                return false;
            }
            popStack();
        }        
        break;

    case TDF_ACTUAL_TYPE_MAP:
        {
            pushStack();
            const TdfMapBase& map = ref.asMap();
            if (!map.visitMembers(*this, context))
            {
                popStack();
                return false;
            }
            popStack();
        }
        break;

    case TDF_ACTUAL_TYPE_VARIABLE:
        {
            const VariableTdfBase& var = ref.asVariable();
            if (var.get() != nullptr)
            {
                pushStack();
                TdfGenericReferenceConst tdfRef(*var.get());
                TdfVisitContextConst c(context, tdfRef);
                if (!visitContext(c))
                {
                    popStack();
                    return false;
                }
                popStack();
            }
        }
        break;
    case TDF_ACTUAL_TYPE_GENERIC_TYPE:
        {
            const GenericTdfType& gen = ref.asGenericType();
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            if (gen.isSet())
            {
#endif
                pushStack();
                TdfGenericReferenceConst tdfRef(gen.get());
                TdfVisitContextConst c(context, tdfRef);
                visitContext(c);
                popStack();
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            }
#endif
        }
        break;

    case TDF_ACTUAL_TYPE_TDF:
    case TDF_ACTUAL_TYPE_UNION: 
        {
            pushStack();
            const Tdf& tdf = ref.asTdf();
            if (!tdf.visitMembers(*this, context))
            {
                popStack();
                return false;
            }
            popStack();
        }        
        break;

    default:                           
        //no internals
        break;
    }

    if (!postVisitValue(context))
        return false;

    return true;
}

void TdfMemberVisitorConst::pushStack()
{
    if (mStateDepth >= MAX_STATE_DEPTH)
    {
        //  perhaps we should extend the stack?  better than overwriting memory.
        eastl::string msg;
        msg.append_sprintf("[TdfMemberVisitorConst].pushStack: Attempted to increment stack past max size of %u", MAX_STATE_DEPTH);
        EA_FAIL_MSG(msg.c_str());
        return;
    }
    ++mStateDepth;
}

void TdfMemberVisitorConst::popStack()
{
    if (mStateDepth > 0)
    {
        --mStateDepth;
    }
}


} //namespace TDF

} //namespace EA

