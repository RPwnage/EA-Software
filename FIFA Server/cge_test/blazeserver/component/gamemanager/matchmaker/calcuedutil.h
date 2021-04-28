/*! ************************************************************************************************/
/*!
    \file calcuedutil.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_CALCUEDUTIL_H
#define BLAZE_GAMEMANAGER_CALCUEDUTIL_H

#include "gamemanager/matchmaker/groupuedexpressionlist.h"
#include "EABase/eabase.h"

namespace Blaze { namespace GameManager { typedef eastl::pair<UserExtendedDataName, GroupValueFormula> PropertyUEDIdentification; } }
namespace eastl
{
    template <> struct hash<Blaze::GameManager::PropertyUEDIdentification>
    {
        size_t operator()(Blaze::GameManager::PropertyUEDIdentification p) const { return static_cast<size_t>(p.second + eastl::hash<const char8_t*>()(p.first.c_str())); }
    };
    template <> struct equal_to<Blaze::GameManager::PropertyUEDIdentification>
    {
        bool operator()(const Blaze::GameManager::PropertyUEDIdentification& a, const Blaze::GameManager::PropertyUEDIdentification& b) const { return a.second == b.second && a.first == b.first; }
    };
}

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    class CalcUEDUtil
    {
    public:
        CalcUEDUtil() : mRefCount(0), mGroupValueFormula(GroupValueFormula::GROUP_VALUE_FORMULA_AVERAGE), mMinRange(INT64_MIN), mMaxRange(INT64_MAX), mDataKey(INVALID_USER_EXTENDED_DATA_KEY) {}
        virtual ~CalcUEDUtil() {}

        bool initialize(const UserExtendedDataName& uedName, GroupValueFormula groupValueFormula, UserExtendedDataValue minRange = INT64_MIN, UserExtendedDataValue maxRange = INT64_MAX, const GroupAdjustmentFormulaList* groupAdjustmentFormulaList = nullptr);
        bool initialize(const UserExtendedDataKey uedKey, GroupValueFormula groupValueFormula, UserExtendedDataValue minRange = INT64_MIN, UserExtendedDataValue maxRange = INT64_MAX, const GroupAdjustmentFormulaList* groupAdjustmentFormulaList = nullptr);

        bool calcUEDValue(UserExtendedDataValue& uedValue, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersSessionInfo) const;

        const char8_t* getUEDName() const;
        GroupValueFormula getGroupValueFormula() const { return mGroupValueFormula; }
        UserExtendedDataValue getMinRange() const { return mMinRange; }
        UserExtendedDataValue getMaxRange() const { return mMaxRange; }
        UserExtendedDataKey getUEDKey() const;
        bool normalizeUEDValue(UserExtendedDataValue& value) const;

    private:
        bool initialize(UserExtendedDataKey uedKey, UserExtendedDataName uedName, GroupValueFormula groupValueFormula, UserExtendedDataValue minRange, UserExtendedDataValue maxRange, const GroupAdjustmentFormulaList* groupAdjustmentFormulaList);
        
        friend void intrusive_ptr_add_ref(CalcUEDUtil* ptr);
        friend void intrusive_ptr_release(CalcUEDUtil* ptr);
        uint32_t mRefCount;

        mutable UserExtendedDataName mUedName; // Lazy init requires mutable.
        GroupValueFormula mGroupValueFormula;
        UserExtendedDataValue mMinRange;
        UserExtendedDataValue mMaxRange;
        GroupUedExpressionList mGroupUedExpressionList;
        mutable Blaze::UserExtendedDataKey mDataKey; // Lazy init requires mutable.
    };

    typedef eastl::intrusive_ptr<CalcUEDUtil> CalcUEDUtilPtr;
    void intrusive_ptr_add_ref(CalcUEDUtil* ptr);
    void intrusive_ptr_release(CalcUEDUtil* ptr);

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_CALCUEDUTIL_H
