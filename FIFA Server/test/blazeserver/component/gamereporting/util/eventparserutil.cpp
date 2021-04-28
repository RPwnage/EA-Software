/*************************************************************************************************/
/*!
    \file   eventparserutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "eventparserutil.h"
#include "gametype.h"

namespace Blaze
{
namespace GameReporting
{
namespace Utilities
{

EventTdfParser::VisitorState* EventTdfParser::VisitorStateStack::push(bool copyTop)
{
    if (size() < capacity())
    {
        if (!copyTop || empty())
        {
            return &push_back();
        }
        push_back(back());
    }
    else
    {
        // Limit capacity since we never want to resize the stack to prevent trashing references to stack elements kept by other methods.
        ERR_LOG("[EventTdfParser].push() : STACK OVERFLOW!  Report hierarchy is deeper than allowed (capacity=" << capacity() << ")");
        return nullptr;
    }
    VisitorState& cur = back();

    cur.isMapKey = false;

    return &cur;
}

/////////////////////////////////////////////////////////////////////////////////////////

EventTdfParser::EventTdfParser() 
{
    mVisitingSuccess = true;
    mVisitingTag = 0;
    mIndexCount = 0;
    mVisitingIndex = 0;
}

EventTdfParser::~EventTdfParser()
{
    mValueMaps.clear();
}

void EventTdfParser::initValuesForVisitor(const EA::TDF::TdfGenericValue& entityId, const Collections::AttributeMap& valueMap)
{
    // just insert new record for the new values from the game report...
    mIndexCount++;
    mEntityIds[mIndexCount] = entityId;

    Collections::AttributeMap* mapData = BLAZE_NEW Collections::AttributeMap();
    mValueMaps[mIndexCount] = mapData;

    mapData->insert(valueMap.begin(), valueMap.end());
}

void EventTdfParser::updateVisitorState()
{
    VisitorState& state = mStateStack.top();
    if (state.containerType == VisitorState::MAP_CONTAINER)
    {
        state.isMapKey = true;
    }
}

bool EventTdfParser::visit(const GameTypeReportConfig::Event& reportConfig, const GRECacheReport::Event& greReportConfig, EA::TDF::Tdf& tdfOut, EA::TDF::Tdf& tdfOutParent)
{
    if (mValueMaps.empty())
    {
        ERR_LOG("[EventTdfParser].visit() : Init values map failed, please check the configuration if the attributes of the event '"
            << reportConfig.getTdfMemberName() << "' are correct." );
        return false;
    }
    
    const EA::TDF::TdfMemberInfo* memberInfo = nullptr;;
    if (!tdfOut.getMemberInfoByName(reportConfig.getTdfMemberName(), memberInfo))
    {
        ERR_LOG("[EventTdfParser].visit() : tdfMemberName '" << reportConfig.getTdfMemberName() << "' is incorrect, please check it in custom code.");
        return false;
    }

    VisitorState* state = mStateStack.push();

    
    // Get the value type by the tag, and store the tag which will be used if visiting a map or list.
    mVisitingTag = memberInfo->getTag();
    if (memberInfo->getType() == EA::TDF::TDF_ACTUAL_TYPE_MAP)
    {
        state->isMapKey = true;
        state->containerType = VisitorState::MAP_CONTAINER;
    }
    else if (memberInfo->getType() == EA::TDF::TDF_ACTUAL_TYPE_LIST)
    {
        state->containerType = VisitorState::LIST_CONTAINER;
    }

    // init visit index.
    mVisitingIndex = mIndexCount;
    tdfOut.visit(*this, tdfOut, tdfOut);
    mStateStack.pop();

    if (mVisitingSuccess)
    {
        TRACE_LOG("[EventTdfParser].visit() : Successfully visited tdfMember '" << reportConfig.getTdfMemberName() << "'.");
    }
    else
    {
        ERR_LOG("[EventTdfParser].visit() : Failed to visit tdfMember '" << reportConfig.getTdfMemberName() << "'.");
    }

    return mVisitingSuccess;
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue)
{
    setIntEventValue(value, rootTdf, tag);
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue)
{
    setIntEventValue(value, rootTdf, tag);
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue)
{
    setIntEventValue(value, rootTdf, tag);
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue)
{
    setIntEventValue(value, rootTdf, tag);
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue)
{
    setIntEventValue(value, rootTdf, tag);
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue)
{
    setIntEventValue(value, rootTdf, tag);
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue)
{
    setIntEventValue(value, rootTdf, tag);
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue)
{
    setIntEventValue(value, rootTdf, tag);
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue)
{
    if (!mStateStack.isEmpty())
    {
        const char8_t* valMemberName = nullptr;
        if (parentTdf.getMemberNameByTag(tag, valMemberName))
        {
            for (Collections::AttributeMap::iterator i = mVisitingMap.begin(), e = mVisitingMap.end(); i != e; ++i)
            {
                if (blaze_stricmp(valMemberName, i->first) == 0)
                {
                    uint16_t result;
                    blaze_str2int(i->second.c_str(), &result);
                    value = result != 0;
                    break;
                }
            }
        }
    }
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue)
{
    if (!mStateStack.isEmpty())
    {
        const char8_t* valMemberName = nullptr;
        if (parentTdf.getMemberNameByTag(tag, valMemberName))
        {
            for (Collections::AttributeMap::iterator i = mVisitingMap.begin(), e = mVisitingMap.end(); i != e; ++i)
            {
                if (blaze_stricmp(valMemberName, i->first) == 0)
                {
                    blaze_str2flt(i->second.c_str(), value);
                    break;
                }
            }
        }
    }
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
{
    if (!mStateStack.isEmpty())
    {
        VisitorState& state = mStateStack.top();

        if (state.isMapKey)
        {
            state.isMapKey = false;
            value = mEntityIds[mVisitingIndex].asString();
        }
        else
        {
            const char8_t* valMemberName = nullptr;
            if (parentTdf.getMemberNameByTag(tag, valMemberName))
            {
                for (Collections::AttributeMap::iterator i = mVisitingMap.begin(), e = mVisitingMap.end(); i != e; ++i)
                {
                    if (blaze_stricmp(valMemberName, i->first) == 0)
                    {
                        value = i->second;
                        updateVisitorState();
                        break;
                    }
                }
            }
        }
    }
}

bool EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
{
    if (!mStateStack.isEmpty())
    {
        if (mVisitingTag == tag)
        {
            if (mVisitingIndex < 0)
            {
                ERR_LOG("[EventTdfParser].visit() : visiting Index out of scope when visiting tdf'" << parentTdf.getClassName() << "'.");
                mVisitingSuccess = false;

                return false;
            }

            VisitorState& state = mStateStack.top();
        
            mVisitingMap.clear();
            mVisitingMap.insert(mValueMaps[mVisitingIndex]->begin(), mValueMaps[mVisitingIndex]->end());
            mVisitingIndex--;

            value.visit(*this, value, value);

            if (state.containerType == VisitorState::MAP_CONTAINER)
            {
                updateVisitorState();
            }
        }
    }

    return true;
}

void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
{
    if (!mStateStack.isEmpty())
    {
        VisitorState& state = mStateStack.top();
        
        if (state.containerType == VisitorState::LIST_CONTAINER && mVisitingTag == tag)
        {
            value.initVector(value.vectorSize() + 1);
            value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
        }
    }
}


void EventTdfParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
{
    if (!mStateStack.isEmpty())
    {
        VisitorState& state = mStateStack.top();
        
        if (state.containerType == VisitorState::MAP_CONTAINER && mVisitingTag == tag)
        {
            value.initMap(value.mapSize() + 1);
            value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
        }
    }
}

template <typename T> void EventTdfParser::setIntEventValue(T& value, const EA::TDF::Tdf& parentTdf, uint32_t tag)
{
    if (!mStateStack.isEmpty())
    {
        VisitorState& state = mStateStack.top();

        if (state.isMapKey)
        {
            state.isMapKey = false;
            EA::TDF::TdfGenericReference refValue(value);
            mEntityIds[mVisitingIndex].convertToIntegral(refValue);
        }
        else
        {
            const char8_t* valMemberName = nullptr;
            if (parentTdf.getMemberNameByTag(tag, valMemberName))
            {
                for (Collections::AttributeMap::iterator i = mVisitingMap.begin(), e = mVisitingMap.end(); i != e; ++i)
                {
                    if (blaze_stricmp(valMemberName, i->first) == 0)
                    {
                        blaze_str2int(i->second.c_str(), &value);
                        break;
                    }
                }
            }
        }
    }
}



}   // namespace Utilities

}   // namespace GameReporting
}   // namespace Blaze
