/*! *********************************************************************************************/
/*!
    \file   configmerger.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/config/configmerger.h"

namespace Blaze
{

    ConfigMerger::ConfigMerger()
        : mStackTopOffset(0)
    {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        mReferenceItr = nullptr;
        mMemberItr = nullptr;
        mInTdf = false;
#endif // EA_TDF_INCLUDE_CHANGE_TRACKING
    }

    //Starts visiting a TDF
    bool ConfigMerger::merge(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &tdfLeft)
    {
        if (tdf.getTdfId() != tdfLeft.getTdfId())
            return false;

        //  top level element defaults to true always.
        mConfigMergeStack.set(mStackTopOffset);

        //  The merge needs to maintain the original string buffer for 'tdf' and explicitly set new string values when merging them over from tdfLeft.
        tdf.visit(*this, tdf, tdfLeft);
        return true;
    }

    bool ConfigMerger::isMergeAllowed(const EA::TDF::Tdf& parentTdf, uint32_t tag)
    {
        bool allowReconfigure = mConfigMergeStack.test(mStackTopOffset);

#ifdef BLAZE_ENABLE_RECONFIGURE_TAG_INFO
        const EA::TDF::TdfMemberInfo* memberInfo = nullptr;
        if (parentTdf.getMemberInfoByTag(tag, memberInfo))
        {
            switch (memberInfo->reconfig)
            {
            case EA::TDF::TdfMemberInfo::RECONFIGURE_DEFAULT:
                // no-op
                break;
            case EA::TDF::TdfMemberInfo::RECONFIGURE_YES:
                allowReconfigure = true;
                break;
            case EA::TDF::TdfMemberInfo::RECONFIGURE_NO:
                allowReconfigure = false;
                break;
            default:
                allowReconfigure = false;
                BLAZE_ERR_LOG(Log::CONFIG, "[ConfigMerger].isMergeAllowed: invalid value (" << memberInfo->reconfig << ") for reconfigurable setting for EA::TDF::Tdf "
                    << parentTdf.getClassName() << ", name(" << memberInfo->getMemberName() << "), tag(" << SbFormats::Raw("0x%08x", tag) << "), default to non-reconfigurable");
                break;
            };
        }
#endif
        if (!allowReconfigure && mReconfigurableSettingOverride)
        {
            allowReconfigure = true;
            BLAZE_TRACE_LOG(Log::CONFIG, "[ConfigMerger].isMergeAllowed: Overriding merge setting for EA::TDF::Tdf "
                << parentTdf.getClassName() << ", tag(" << SbFormats::Raw("0x%08x", tag) << ") because ancestor is a new element in a reconfigurable collection.");
        }

        return allowReconfigure;
    }

    void ConfigMerger::logReconfigureValueIgnored(const EA::TDF::Tdf& parentTdf, uint32_t tag)
    {
        const EA::TDF::TdfMemberInfo* memberInfo = nullptr;
        parentTdf.getMemberInfoByTag(tag, memberInfo);
        BLAZE_WARN_LOG(Log::CONFIG, "[ConfigMerger].logReconfigureValueIgnored: "
            << parentTdf.getClassName() << "::" << (memberInfo ? memberInfo->getMemberName() : "<unknown>") << ", tag(" << SbFormats::Raw("0x%08x", tag) << ") is not reconfigurable, change ignored.");
    }

    void ConfigMerger::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
    {
        //  the target TDF should own the string copied from the source (removing the reference into the EA::TDF::Tdf's string buffer table.)
        if (isMergeAllowed(parentTdf, tag))
        {
            value.set(referenceValue.c_str(), referenceValue.length());
            updateMemberChangeTrackingBitToMatchReferenceTdf();
        }
        else if (value != referenceValue)
        {
            logReconfigureValueIgnored(parentTdf, tag);
        }
    }

    //  these methods may alter the merge setting stack as they descend into a complex data type.
    //  for vectors primitive collections are treated as a simple copy, object vectors are merged by position, and new members are inserted at the end.
    void ConfigMerger::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
    {
        if (isMergeAllowed(parentTdf, tag))
        {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            EA::TDF::TdfMemberIterator* tempMemberIt = mMemberItr;
            EA::TDF::TdfMemberIteratorConst* tempReferenceIt = mReferenceItr;
            bool tempInTdf = mInTdf;
            mInTdf = false;
#endif
            if (value.isObjectCollection())
            {
                const auto curSize = value.vectorSize();
                const auto newSize = referenceValue.vectorSize();
                if (curSize == newSize)
                {
                    // up to curSize, standard merge
                    value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
                }
                else if (curSize > newSize)
                {
                    // trim all elements after newSize, standard merge
                    value.resizeVector(newSize, false);
                    value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
                }
                else
                {
                    // up to curSize standard merge, then resize, and set mReconfigurableSettingOverride 
                    // and visit the rest as if they are newly added (not subject to reconfigurable setting)
                    value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
                    value.resizeVector(newSize, false);
                    
                    auto old = mReconfigurableSettingOverride;

                    // NOTE: This setting cannot be disabled by member's children, 
                    // because once an element of a reconfigurable collection 
                    // is determined to be new, we must recursively include its entire state
                    mReconfigurableSettingOverride = true;
                    for (auto i = curSize; i < newSize; ++i)
                    {
                        EA::TDF::TdfGenericReference curValueRef;
                        EA::TDF::TdfGenericReferenceConst newValueRef;
                        if (!value.getReferenceByIndex(i, curValueRef) || !referenceValue.getValueByIndex(i, newValueRef))
                        {
                            BLAZE_ERR_LOG(Log::CONFIG, "[ConfigMerger].visit: Failed to get reference for a new element in vector for EA::TDF::Tdf "
                                << parentTdf.getClassName() << ", tag(" << SbFormats::Raw("0x%08x", tag) << ").");
                            continue;
                        }
                        visitReference(rootTdf, parentTdf, tag, curValueRef, &newValueRef);
                    }
                    
                    mReconfigurableSettingOverride = old;
                }
            }
            else
            {
                value.initVector(referenceValue.vectorSize());
                value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
            }
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            mReferenceItr = tempReferenceIt;
            mMemberItr = tempMemberIt;
            mInTdf = tempInTdf;
#endif // EA_TDF_INCLUDE_CHANGE_TRACKING
            updateMemberChangeTrackingBitToMatchReferenceTdf();
        }
        else if (!value.equalsValue(referenceValue))
        {
            logReconfigureValueIgnored(parentTdf, tag);
        }
    }

    // for maps, it is possible to merge members with the same key, and add members with new keys, 
    // so rather than being lazy that's what we're going to do.
    void ConfigMerger::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
    {
        if (isMergeAllowed(parentTdf, tag))
        {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            EA::TDF::TdfMemberIterator* tempMemberIt = mMemberItr;
            EA::TDF::TdfMemberIteratorConst* tempReferenceIt = mReferenceItr;
            bool tempInTdf = mInTdf;
            mInTdf = false;
#endif
            if (value.isObjectCollection())
            {
                const auto curSize = value.mapSize();
                const auto newSize = referenceValue.mapSize();

                EA::TDF::TdfGenericReferenceConst refFirst, refSecond, valFirst;
                // Remove any elements that aren't in the new collection, 
                // walk in reverse order because collection may shrink
                for (auto i = curSize; i > 0; --i)
                {
                    EA::TDF::TdfGenericReferenceConst valSecond;
                    if (value.getValueByIndex(i - 1, valFirst, valSecond))
                    {
                        if (!referenceValue.getValueByKey(valFirst, refSecond))
                        {
                            value.eraseValueByKey(valFirst);
                        }
                    }
                }
                for (auto i = 0; i < newSize; ++i)
                {
                    if (referenceValue.getValueByIndex(i, refFirst, refSecond))
                    {
                        EA::TDF::TdfGenericReference valSecond;
                        if (value.insertKeyGetValue(refFirst, valSecond))
                        {
                            auto old = mReconfigurableSettingOverride;

                            // NOTE: This setting cannot be disabled by member's children, 
                            // because once an element of a reconfigurable collection 
                            // is determined to be new, we must recursively include its entire state
                            mReconfigurableSettingOverride = true;

                            visitReference(rootTdf, parentTdf, tag, valSecond, &refSecond);

                            mReconfigurableSettingOverride = old;
                        }
                        else
                        {
                            visitReference(rootTdf, parentTdf, tag, valSecond, &refSecond);
                        }
                    }
                }
            }
            else
            {
                value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
            }

#if EA_TDF_INCLUDE_CHANGE_TRACKING   
            mReferenceItr = tempReferenceIt;
            mMemberItr = tempMemberIt;
            mInTdf = tempInTdf;
#endif // EA_TDF_INCLUDE_CHANGE_TRACKING
            updateMemberChangeTrackingBitToMatchReferenceTdf();
        }
        else if (!value.equalsValue(referenceValue))
        {
            logReconfigureValueIgnored(parentTdf, tag);
        }
    }

    //  descend directly into a TDF within the configuration.   Alter merge stack accordingly.
    bool ConfigMerger::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
    {
        if (isMergeAllowed(parentTdf, tag))
        {
            mConfigMergeStack.set(++mStackTopOffset, true);
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            EA::TDF::TdfMemberIterator* tempMemberIt = mMemberItr;
            EA::TDF::TdfMemberIteratorConst* tempReferenceIt = mReferenceItr;
            bool tempInTdf = mInTdf;
            mInTdf = true;
#endif // EA_TDF_INCLUDE_CHANGE_TRACKING
            value.visit(*this, value, referenceValue);
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            mReferenceItr = tempReferenceIt;
            mMemberItr = tempMemberIt;
            mInTdf = tempInTdf;
#endif // EA_TDF_INCLUDE_CHANGE_TRACKING
            mConfigMergeStack.set(mStackTopOffset--, false);
        }
        else if (!value.equalsValue(referenceValue))
        {
            logReconfigureValueIgnored(parentTdf, tag);
        }
        return true;
    }

    bool ConfigMerger::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue)
    {
        if (isMergeAllowed(parentTdf, tag))
        {
            mConfigMergeStack.set(++mStackTopOffset, true);
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            EA::TDF::TdfMemberIterator* tempMemberIt = mMemberItr;
            EA::TDF::TdfMemberIteratorConst* tempReferenceIt = mReferenceItr;
            bool tempInTdf = mInTdf;
            mInTdf = true;
#endif // EA_TDF_INCLUDE_CHANGE_TRACKING
            if (value.getActiveMemberIndex() == EA::TDF::TdfUnion::INVALID_MEMBER_INDEX && referenceValue.getActiveMemberIndex() != EA::TDF::TdfUnion::INVALID_MEMBER_INDEX)
            {
                //We are visiting from a map, so the value was cleared. Lets just do a copy from ref to value.
                referenceValue.copyInto(value);
            }
            else
            {
                value.visitActiveMember(*this, value, referenceValue);
            }
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            mReferenceItr = tempReferenceIt;
            mMemberItr = tempMemberIt;
            mInTdf = tempInTdf;
#endif // EA_TDF_INCLUDE_CHANGE_TRACKING
            updateMemberChangeTrackingBitToMatchReferenceTdf();
            mConfigMergeStack.set(mStackTopOffset--, false);
        }
        else if (!value.equalsValue(referenceValue))
        {
            logReconfigureValueIgnored(parentTdf, tag);
        }
        return true;
    }

    bool ConfigMerger::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue)
    {

        if (isMergeAllowed(parentTdf, tag))
        {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            EA::TDF::TdfMemberIterator* tempMemberIt = mMemberItr;
            EA::TDF::TdfMemberIteratorConst* tempReferenceIt = mReferenceItr;
            bool tempInTdf = mInTdf;
            mInTdf = false;
#endif
            if ((value.isValid() && referenceValue.isValid()) && (value.get()->getTdfId() == referenceValue.get()->getTdfId()))
            {
                // same type, merge
                mConfigMergeStack.set(++mStackTopOffset, true);
                visit(rootTdf, parentTdf, tag, *value.get(), *referenceValue.get());
                mConfigMergeStack.set(mStackTopOffset--, false);
            }
            else if (referenceValue.isValid())
            {
                // different type, or new entry, replace
                referenceValue.copyInto(value);
            }
            else
            {
                // If the reference is the default, then we need to clear the existing value.
                value.clear();
            }

#if EA_TDF_INCLUDE_CHANGE_TRACKING   
            mReferenceItr = tempReferenceIt;
            mMemberItr = tempMemberIt;
            mInTdf = tempInTdf;
#endif // EA_TDF_INCLUDE_CHANGE_TRACKING
            updateMemberChangeTrackingBitToMatchReferenceTdf();
        }
        else if (!value.equalsValue(referenceValue))
        {
            logReconfigureValueIgnored(parentTdf, tag);
        }

        return true;
    }

    bool ConfigMerger::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue)
    {
        if (isMergeAllowed(parentTdf, tag))
        {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
            EA::TDF::TdfMemberIterator* tempMemberIt = mMemberItr;
            EA::TDF::TdfMemberIteratorConst* tempReferenceIt = mReferenceItr;
            bool tempInTdf = mInTdf;
            mInTdf = false;
#endif
            if ((value.isValid() && referenceValue.isValid()) && (value.get().getTdfId() == referenceValue.get().getTdfId()))
            {
                // same type, merge
                mConfigMergeStack.set(++mStackTopOffset, true);
                visitReference(rootTdf, parentTdf, tag, value.get(), &referenceValue.get());
                mConfigMergeStack.set(mStackTopOffset--, false);
            }
            else if (referenceValue.isValid())
            {
                // different type, or new entry, replace
                referenceValue.copyInto(value);
            }
            else
            {
                // If the reference is the default, then we need to clear the existing value.
                value.clear();
            }

#if EA_TDF_INCLUDE_CHANGE_TRACKING   
            mReferenceItr = tempReferenceIt;
            mMemberItr = tempMemberIt;
            mInTdf = tempInTdf;
#endif // EA_TDF_INCLUDE_CHANGE_TRACKING
            updateMemberChangeTrackingBitToMatchReferenceTdf();
        }

        return true;
    }

}

