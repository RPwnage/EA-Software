/*************************************************************************************************/
/*!
    \file   collateutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "collatorutil.h"

namespace Blaze
{
namespace GameReporting
{
namespace Utilities
{

Collator::VisitorState& Collator::VisitorStateStack::push(bool copyTop)
{
    if (!copyTop || empty())
    {
        return push_back();
    }
    push_back(back());
    VisitorState& cur = back();

    //  TDF parsing values should reset
    cur.tdfParseMode = VisitorState::TDF_MODE_GENERAL_VALUE;
    cur.tdfParseModeState = 0;

    return cur;
}


void Collator::VisitorStateStack::printTop()
{
    const VisitorState& topVal = top();
    ERR_LOG("[Collator_VisitorStateStack].printTop : size=" << size() << ", tdfParseMode=" << topVal.tdfParseMode << ",cur, tdfParseModeState=" 
             << topVal.tdfParseModeState << ", entityId=" << topVal.entityId);
}


/**
    Collator
        Merges a GameReport based TDF to another GameReport based TDF. 
        Merge behavior is based off of the current Collator merge mdoe.
        Uses attribute information to define additional merge behavior.
 */
Collator::Collator()
{
    mMergeMode = MERGE_MAPS;
}

Collator::~Collator()
{
}


//  merges source TDF to the Target using the current merge mode.
void Collator::merge(EA::TDF::Tdf& target, const EA::TDF::Tdf& source)
{
    //  validate that the source and target are the same.
    visit(target, source);
}



////////////////////////////////////////////////////////////////////////////////////////////////


Collator::VisitorState& Collator::updateState(VisitorState& state) const
{
    //  special cases for maps where if we're inside visitMembers wer're going through keys and values.
    if (state.tdfParseMode == VisitorState::TDF_MODE_MAP)
    {
        //  treating this as a key to keep consistency though maps can't have TDF keys!
        if (state.tdfParseModeState == 0)
            state.tdfParseModeState = 1;        // read in value next.
        else if (state.tdfParseMode == 1)
            state.tdfParseModeState = 0;        // read in key next.
    }

    return state;
}


////////////////////////////////////////////////////////////////////////////////////////////////
//  EA::TDF::TdfVisitor
bool Collator::visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue)
{
    VisitorState& state = mStateStack.push();
    state.mergeMode = getMergeMode();
    state.lastMergeMode = state.mergeMode;
    tdf.visit(*this, tdf, referenceValue);
    mStateStack.pop();

    if (!mStateStack.isEmpty())
    {
        ERR_LOG("[Collator].visit() : parse stack is not empty.  There is likely an error in the EA::TDF::TdfVisitor parsing scheme.");
        ERR_LOG("[Collator].visit() : EA::TDF::Tdf = " << tdf.getClassName());
        mStateStack.printTop();
        mStateStack.reset();
    }

    return true;
}

bool Collator::shouldMergeCopyOverExistingValues()
{
    const VisitorState& state = mStateStack.top();
    // special note for maps - if merging maps, merge over map values to target
    // the merge_map logic requires visiting within a container, and being a map value.
    return (state.mergeMode == MERGE_COPY ||
            (state.mergeMode == MERGE_MAPS_COPY && (state.tdfParseMode != VisitorState::TDF_MODE_MAP || state.tdfParseModeState != 0))
            );
}


//  The Collator uses:
//      attribute information gathered from the tag to decide whether to copy value from the reference to the target.
//      merge mode.

template <typename T> void Collator::mergeValue(T& target, const T& source, const EA::TDF::Tdf &parentTdf, uint32_t tag)
{
    if (shouldMergeCopyOverExistingValues())
    {
        target = source;
    }
}


bool Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
{
    mStateStack.push();
    {           
        value.visit(*this, rootTdf, referenceValue);
    }
    mStateStack.pop();

    VisitorState& curstate = mStateStack.top();
    updateState(curstate);

    return true;
}


bool Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue)
{
    mStateStack.push();
    {
        if (referenceValue.isValid())
        {
            if (!value.isValid())
            {
                // if merging into a nullptr variable TDF, create it in the reference's image, then visit.
                value.create(referenceValue.get()->getTdfId());    
            }
            value.get()->visit(*this, rootTdf, *referenceValue.get());
        }
    }
    mStateStack.pop();

    VisitorState& curstate = mStateStack.top();
    updateState(curstate);
    return true;
}

bool Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue)
{
    mStateStack.push();
    {
        if (referenceValue.isValid())
        {
            if (!value.isValid())
            {
                // if merging into a nullptr variable TDF, create it in the reference's image, then visit.
                value.setValue(referenceValue.getValue());
            }
            else
            {
                visitReference(rootTdf, parentTdf, tag, value.getValue(), &referenceValue.getValue());
            }
            
        }
    }
    mStateStack.pop();

    VisitorState& curstate = mStateStack.top();
    updateState(curstate);
    return true;
}


//  Merge Map - vists members and conditionally adds new keys from referenceValue
//      note that to support
void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
{
    VisitorState& state = mStateStack.push();
    {
        state.tdfParseMode = VisitorState::TDF_MODE_MAP;
        state.tdfParseModeState = 0;            // start with key.
        state.inContainer = true;

        //  if we're copying the reference map to the target, then wipe out the target map and initialize the target so it matches the source map's format.
        if (state.mergeMode == MERGE_COPY)
        {
            value.clearMap();
            value.initMap(referenceValue.mapSize());
            value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
        }
        else if (state.mergeMode == MERGE_MAPS || state.mergeMode == MERGE_MAPS_COPY)
        {
            value.mergeMembers(*this, rootTdf, parentTdf, tag, referenceValue);
        }
    }
    mStateStack.pop();

    VisitorState& curstate = mStateStack.top();
    updateState(curstate);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
{
    mStateStack.push();
    {
        if (shouldMergeCopyOverExistingValues())
        {
            // NOTE: We never merge lists, even if the merge mode is set to merge maps.
            //  If the mode is set to MERGE_MAPS (or this is a , then we simply keep all lists at their old values.
            // (The following code copies the new list over the old one)
            value.clearVector();
            value.initVector(referenceValue.vectorSize());
            value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
        }
    }
    mStateStack.pop();
    VisitorState& curstate = mStateStack.top();
    updateState(curstate);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue)
{
    VisitorState& state = mStateStack.top();
    mergeValue(value, referenceValue, parentTdf, tag);
    updateState(state);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue)
{
    VisitorState& state = mStateStack.top();
    mergeValue(value, referenceValue, parentTdf, tag);
    updateState(state);
}


template <typename T> Collator::VisitorState& Collator::collateIntDefault(const EA::TDF::Tdf& parentTdf, uint32_t tag, T& value, T referenceValue)
{
    VisitorState& state = mStateStack.top();

    mergeValue(value, referenceValue, parentTdf, tag);

    updateState(state);

    return state;
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue)
{
    collateIntDefault(parentTdf, tag, value, referenceValue);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue)
{
    collateIntDefault(parentTdf, tag, value, referenceValue);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue)
{
    collateIntDefault(parentTdf, tag, value, referenceValue);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue)
{
    collateIntDefault(parentTdf, tag, value, referenceValue);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue)
{
    collateIntDefault(parentTdf, tag, value, referenceValue);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue)
{
    collateIntDefault(parentTdf, tag, value, referenceValue);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue)
{
    collateIntDefault(parentTdf, tag, value, referenceValue);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue)
{
    collateIntDefault(parentTdf, tag, value, referenceValue);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue)
{
    VisitorState& state = mStateStack.top(); 
    mergeValue(value, referenceValue, parentTdf, tag);
    updateState(state);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue)
{
    VisitorState& state = mStateStack.top();
    mergeValue(value, referenceValue, parentTdf, tag);
    updateState(state);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
{
    VisitorState& state = mStateStack.top();
    mergeValue(value, referenceValue, parentTdf, tag);
    updateState(state);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue)
{
    VisitorState& state = mStateStack.top();

    // special note for maps - if merging maps, merge over map values to target
    // the merge_map logic requires visiting within a container, and being a map value.
    if (shouldMergeCopyOverExistingValues())
    {
        referenceValue.copyInto(value);
    }

    updateState(state);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue)
{
    VisitorState& state = mStateStack.top();
    visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
    updateState(state);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType& referenceValue, const EA::TDF::ObjectType defaultValue)
{
    VisitorState& state = mStateStack.top();
    mergeValue(value, referenceValue, parentTdf, tag);
    updateState(state);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId& referenceValue, const EA::TDF::ObjectId defaultValue)
{
    VisitorState& state = mStateStack.top();
    mergeValue(value, referenceValue, parentTdf, tag);
    updateState(state);
}


void Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue& referenceValue, const TimeValue defaultValue)
{
    VisitorState& state = mStateStack.top();
    mergeValue(value, referenceValue, parentTdf, tag);
    updateState(state);
}


bool Collator::visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue)
{
    mStateStack.push();
    visit(tdf, tdf, 0, tdf, referenceValue);
    mStateStack.pop();
    return true;
}


bool Collator::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue)
{
    VisitorState& state = mStateStack.top();
    value.switchActiveMember(referenceValue.getActiveMemberIndex());
    value.visit(*this, rootTdf, referenceValue); 
    updateState(state);
    return true;
}

}   // namespace Utilities

}   // namespace GameReporting
}   // namespace Blaze
