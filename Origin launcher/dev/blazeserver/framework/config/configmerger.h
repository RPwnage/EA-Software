/*! *********************************************************************************************/
/*!
    \file   configmerger.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef CONFIG_MERGER_H
#define CONFIG_MERGER_H

/************* Include files ***************************************************************************/
#include "EATDF/tdf.h"
#include "EASTL/bitset.h"

/************ Defines/Macros/Constants/Typedefs *******************************************************/

namespace Blaze
{

class ConfigMerger : private EA::TDF::TdfVisitor
{
public:
    ConfigMerger();

    //Starts visiting a TDF
    bool merge(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &tdfLeft);

private:
    eastl::bitset<128> mConfigMergeStack;
    size_t mStackTopOffset;

    bool isMergeAllowed(const EA::TDF::Tdf& parentTdf, uint32_t tag);
    void logReconfigureValueIgnored(const EA::TDF::Tdf& parentTdf, uint32_t tag);

#if EA_TDF_INCLUDE_CHANGE_TRACKING
    EA::TDF::TdfMemberIterator* mMemberItr;
    EA::TDF::TdfMemberIteratorConst* mReferenceItr;
    bool mInTdf = false;
    bool mReconfigurableSettingOverride = false;

    bool preVisitCheck(EA::TDF::Tdf& rootTdf, EA::TDF::TdfMemberIterator& memberIt, EA::TDF::TdfMemberIteratorConst& referenceIt) override 
    {
        mReferenceItr = &referenceIt;
        mMemberItr = &memberIt;
        return true; 
    }
#endif // EA_TDF_INCLUDE_CHANGE_TRACKING


    void updateMemberChangeTrackingBitToMatchReferenceTdf() 
    {
#if EA_TDF_INCLUDE_CHANGE_TRACKING
        if (mInTdf)
        {
            if (mReferenceItr != nullptr && mMemberItr != nullptr)
            {
                bool refSet = mReferenceItr->isSet();
                if (refSet != mMemberItr->isSet())
                {
                    if (refSet)
                        mMemberItr->markSet();
                    else
                        mMemberItr->clearIsSet();
                }
            }
            else
            {
                EA_FAIL_MSG("[ConfigMerger].updateMemberChangeTrackingBitToMatchReferenceTdf: mReferenceItr or mMemberItr is nullptr");
            }
            mReferenceItr = nullptr;
            mMemberItr = nullptr;
        }
#endif // EA_TDF_INCLUDE_CHANGE_TRACKING
    }


    template<typename P> inline void mergeValue(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, P& target, const P& source, const P defaultValue) 
    {
        if (isMergeAllowed(parentTdf, tag)) 
        {
            target = source;
            updateMemberChangeTrackingBitToMatchReferenceTdf();
        }
        else if (target != source)
        {
            logReconfigureValueIgnored(parentTdf, tag);
        }
    }


private:
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue = false) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue = 0) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue = 0) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue = 0) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue = 0) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue = 0) override{ mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue = 0) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue = 0) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue = 0) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue = 0) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue = 0.0f) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue) override  { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TimeValue &value, const EA::TDF::TimeValue &referenceValue, const EA::TDF::TimeValue defaultValue) override { mergeValue(rootTdf, parentTdf, tag, value, referenceValue, defaultValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue) override 
    {
        if (isMergeAllowed(parentTdf, tag)) 
        {
            value.setBits(referenceValue.getBits());
            updateMemberChangeTrackingBitToMatchReferenceTdf();
        }
        else if (value.getBits() != referenceValue.getBits())
        {
            logReconfigureValueIgnored(parentTdf, tag);
        }
    }

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue) override 
    {
        if (isMergeAllowed(parentTdf, tag)) 
        { 
            referenceValue.copyInto(value);
            updateMemberChangeTrackingBitToMatchReferenceTdf();
        }
        else if (!(value == referenceValue))
        {
            logReconfigureValueIgnored(parentTdf, tag);
        }
    }

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue = "", const uint32_t maxLength = 0) override;

    // enumeration visitor
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue = 0 ) override 
    {
        // just call the regular int32_t visitor
        visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
    }

    // these methods may alter the merge setting stack as they descend into a complex data type.
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue) override;

    //The top level operations just call merge
    bool visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue) override { return merge(tdf, referenceValue); }
    bool visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue) override { return merge(tdf, referenceValue); }

};

}   // namespace Blaze


#endif // RECONFIGURE_HELPER_H

