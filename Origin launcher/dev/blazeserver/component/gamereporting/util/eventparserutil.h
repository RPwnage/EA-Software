/*************************************************************************************************/
/*!
    \file   eventparserutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_EVENTPARSERUTIL_H
#define BLAZE_GAMEREPORTING_EVENTPARSERUTIL_H

#include "EATDF/tdf.h"
#include "gamereporting/tdf/gamereporting_server.h"
#include "gamereporttdf.h"


namespace Blaze
{
namespace GameReporting
{
namespace Utilities
{
/*
    EventTdfParser
        Parse the vaule from GameReport tdf into GameEvent tdf.
*/
class EventTdfParser: public EA::TDF::TdfVisitor
{
public:
    typedef int8_t VisitIndex;
    EventTdfParser();
    ~EventTdfParser() override;

    bool visit(const GameTypeReportConfig::Event& reportConfig, const GRECacheReport::Event& greReport, EA::TDF::Tdf& tdfOut, EA::TDF::Tdf& tdfOutParent);
    void initValuesForVisitor(const EA::TDF::TdfGenericValue& entityId, const Collections::AttributeMap& valueMap);

protected:    

    //  EA::TDF::TdfVisitor methods
    struct VisitorState
    {
        enum ContainerType { NO_CONTAINER, LIST_CONTAINER, MAP_CONTAINER };

        VisitorState() : isMapKey(false), containerType(NO_CONTAINER) {}
        VisitorState(bool mapKey, ContainerType conType) : isMapKey(mapKey), containerType(conType) {};
        VisitorState(const VisitorState& src): isMapKey(src.isMapKey), containerType(src.containerType)  {}

        bool isMapKey;
        ContainerType containerType;
    };
    
    //  manage a stack of visitor states while traversing the TDF
    //  using a fixed_vector to prevent resizing of the vector and enforcing a string limit.
    //  the node count is set relatively higher than the expected report depth.
    struct VisitorStateStack: private eastl::fixed_vector<VisitorState, 48, false>
    {
        VisitorState* push(bool copyTop=true);
        void pop() { pop_back(); }
        VisitorState& top() { return back(); }
        bool isEmpty() const { return empty(); }
        void reset() { clear(); }
    };
    VisitorStateStack mStateStack;   

protected:    
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue) override { return true; }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue) override { return true; }
    
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue) override { return true; }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue = false) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue) override {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue=0.0f) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue = "", const uint32_t maxLength = 0) override;

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue) override  {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue) override  {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue = 0 ) override  {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue) override {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue) override  {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue &referenceValue, const TimeValue defaultValue = 0) override  {}


protected: 
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue) override;

private:
    //  no-ops, functionality superceded by other methods in this visitor, which is responsible for visiting the EA::TDF::Tdf based on configuration.
    bool visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue) override { return true; }
    bool visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue) override { return true; }

    template <typename T> void setIntEventValue(T& value, const EA::TDF::Tdf& parentTdf, uint32_t tag);

    bool mVisitingSuccess;

    // This is used for update the state if it is visiting map.
    void updateVisitorState();
    
    uint32_t mVisitingTag; // This is used for storing the current visiting tag when the parser is visiting a map.
    VisitIndex mIndexCount; // Total count of the value and ids map and it's also used for init the mVisitingIndex when starting a new visit.

    VisitIndex mVisitingIndex;  // It's used as the key value for the two maps below. It indicates which AttributeMap should be used for the current visit.
    Collections::AttributeMap mVisitingMap; // The AttributeMap is using now.

    eastl::map<VisitIndex, EA::TDF::TdfGenericValue> mEntityIds;  // It's used for storing the INDEX.
    eastl::map<VisitIndex, EA::TDF::tdf_ptr<Collections::AttributeMap> > mValueMaps; // It's used for storing the values that come from the game report, It should be used together with the mEntityIds when visiting a map.
};


}   // namespace Utilities

}   // namespace GameReporting
}   // namespace Blaze

#endif
