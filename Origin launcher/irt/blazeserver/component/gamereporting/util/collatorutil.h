/*************************************************************************************************/
/*!
    \file   collatorutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_COLLATORUTIL_H
#define BLAZE_GAMEREPORTING_COLLATORUTIL_H

#include "EATDF/tdf.h"
#include "gamereporting/tdf/gamereporting_server.h"


namespace Blaze
{
namespace GameReporting
{
namespace Utilities
{
/**
    Collator
        Merges a GameReport based TDF to another GameReport based TDF.    
        Merge behavior is based off of the current Collator merge mode.
        Uses attribute information to define additional merge behavior.
 */
class Collator: public EA::TDF::TdfVisitor
{
    NON_COPYABLE(Collator);

public:
    Collator();
    ~Collator() override;

    //  Specifies what the merge method does when merging the source TDF with the target TDF.
    enum MergeMode
    {
        MERGE_MAPS,         //<! for maps, merges keys from source not set in the target.
        MERGE_MAPS_COPY,    //<! copies data from source to target.  For maps copies new maps entries and merges existing data in maps.
        MERGE_COPY          //<! copies unconditionally maps from source to target (wiping out target maps.)
    };
    
    void setMergeMode(MergeMode mergeMode) {
        mMergeMode = mergeMode;
    }
private:
    MergeMode getMergeMode() const {
        return mMergeMode;
    }

public:
    bool shouldMergeCopyOverExistingValues();     // Helper function to replace direct useage of getMergeMode. 

    //  merges source TDF to the Target using the current merge mode.
    void merge(EA::TDF::Tdf& target, const EA::TDF::Tdf& source);


protected:
    //  these methods are safe to override for customizing the Collator utility.  
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue = false) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue = '\0') override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue = 0.0f) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue = "", const uint32_t maxLength = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType& referenceValue, const EA::TDF::ObjectType defaultValue = EA::TDF::OBJECT_TYPE_INVALID) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId& referenceValue, const EA::TDF::ObjectId defaultValue = EA::TDF::OBJECT_ID_INVALID) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue& referenceValue, const TimeValue defaultValue = 0) override;

private:
    MergeMode mMergeMode;

    //  Current state of EA::TDF::TdfVisitor when parsing.
    struct VisitorState
    {
        enum TdfParseMode
        {
            TDF_MODE_GENERAL_VALUE,
            TDF_MODE_MAP
        };

        VisitorState() : tdfParseMode(TDF_MODE_GENERAL_VALUE), tdfParseModeState(0), entityId(0), 
            mergeMode(MERGE_COPY), lastMergeMode(MERGE_COPY), inContainer(false) {}

        // current high-level parse mode.  this is needed due to the way some containers are parsed by EA::TDF::TdfVisitor (i.e. maps.)
        TdfParseMode tdfParseMode;      
        // state value to keep track of parsing within the current TdfParseMode
        int32_t tdfParseModeState;
        // current entityId used for stats or other report data being parsed.  for example this can be used by the current report class to point to a user/club/etc.
        EntityId entityId;
        // current merge mode
        MergeMode mergeMode;
        // last merge mode
        MergeMode lastMergeMode;
        // indicates visiting occurs while inside a container (inherits state from parent, which differs from parse mode.)
        bool inContainer;
    };

    //  manage a stack of visitor states while traversing the TDF
    struct VisitorStateStack: private eastl::vector<VisitorState>
    {
        VisitorState& push(bool copyTop=true);
        void pop() { pop_back(); }
        VisitorState& top() { return back(); }
        bool isEmpty() const { return empty(); }
        void reset() { clear(); }
        void printTop();
    };
    VisitorStateStack mStateStack;

    VisitorState& updateState(VisitorState& state) const;

    template <typename T> VisitorState& collateIntDefault(const EA::TDF::Tdf& parentTdf, uint32_t tag, T& value, T referenceValue);
  
private:
    template <typename T> void mergeValue(T& target, const T& source, const EA::TDF::Tdf &parentTdf, uint32_t tag);

    bool visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue) override;
    bool visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue) override;

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue) override;

    // enumeration visitor
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue = 0 ) override;
    

};


}   // namespace Utilities

}   // namespace GameReporting
}   // namespace Blaze

#endif
