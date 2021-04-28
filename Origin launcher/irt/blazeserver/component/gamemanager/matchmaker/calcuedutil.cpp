/*! ************************************************************************************************/
/*!
    \file calcuedutil.cpp

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/calcuedutil.h"
#include "gamemanager/matchmaker/groupvalueformulas.h"
#include "framework/usersessions/usersessionmanager.h"// for gUserSessionManager in getUEDKey()

namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief initialize util's params
    ***************************************************************************************************/
    bool CalcUEDUtil::initialize(const UserExtendedDataName& uedName, GroupValueFormula groupValueFormula, UserExtendedDataValue minRange, UserExtendedDataValue maxRange, const GroupAdjustmentFormulaList* groupAdjustmentFormulaList)
    {
        return initialize(INVALID_USER_EXTENDED_DATA_KEY, uedName, groupValueFormula, minRange, maxRange, groupAdjustmentFormulaList);
    }
    bool CalcUEDUtil::initialize(const UserExtendedDataKey uedKey, GroupValueFormula groupValueFormula, UserExtendedDataValue minRange, UserExtendedDataValue maxRange, const GroupAdjustmentFormulaList* groupAdjustmentFormulaList)
    {
        return initialize(uedKey, "", groupValueFormula, minRange, maxRange, groupAdjustmentFormulaList);
    }

    /*! ************************************************************************************************/
    /*! \brief initialize. Either uedKey, or uedName which is used to resolve the key, must be provided
    ***************************************************************************************************/
    bool CalcUEDUtil::initialize(UserExtendedDataKey uedKey, UserExtendedDataName uedName, GroupValueFormula groupValueFormula, UserExtendedDataValue minRange, UserExtendedDataValue maxRange, const GroupAdjustmentFormulaList* groupAdjustmentFormulaList)
    {
        mDataKey = uedKey;
        mUedName = uedName;
        mGroupValueFormula = groupValueFormula;
        mMinRange = minRange;
        mMaxRange = maxRange;

        if (mUedName.empty() && (mDataKey == INVALID_USER_EXTENDED_DATA_KEY))
        {
            ERR_LOG("[CalcUEDUtil].initialize: ERROR neither uedName nor uedDataKey provided, can't initialize.");
            return false;
        }
        if ((mMaxRange != INVALID_USER_EXTENDED_DATA_VALUE) && (mMaxRange < mMinRange))
        {
            ERR_LOG("[CalcUEDUtil].initialize: ERROR invalid min/max range, minRange(" << mMinRange << ") must be < maxRange(" << mMaxRange << "), failed for UED(" << getUEDName() << ").");
            return false;
        }

        if ((groupAdjustmentFormulaList != nullptr) && !mGroupUedExpressionList.initialize(*groupAdjustmentFormulaList))
        {
            ERR_LOG("[CalcUEDUtil].initialize: ERROR failed to initialize group UED expressions for UED(" << getUEDName() << ").");
            return false;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief calculates the UED value for the user or group depending on which is valid.  If the sessionItr
            is nullptr then defer to the ownerSession.  If the ownerSession is also nullptr then this function returns
            false.  Both sessionItr and ownerSession cannot be nullptr.

        \param[out] uedValue - The calculated UED Value
        \param[in] ownerBlazeId - The blaze Id of the owner, used to calculate if a UED value is individual
        \param[in] ownerGroupSize - The group size of the calling owner.
        \param[in] membersSessionInfo - The UserSessionInfo of a group of users used to calculate the group UED value. If the individual UserSessionInfos have a mGroupSize of 0, we use the owner group size.

        \return false if the value fails to be calculated (indicates a system error)
    *************************************************************************************************/
    bool CalcUEDUtil::calcUEDValue(UserExtendedDataValue& uedValue, BlazeId ownerBlazeId, uint16_t ownerGroupSize, const GameManager::UserSessionInfoList& membersSessionInfo) const
    {
        // If we successfully calculate the value, clamp at the min/max values.
        if ((*GroupValueFormulas::getCalcGroupFunction(getGroupValueFormula()))(getUEDKey(), mGroupUedExpressionList, ownerBlazeId, ownerGroupSize, membersSessionInfo, uedValue)) /*lint !e613 */
        {
            if (normalizeUEDValue(uedValue))
            {
                TRACE_LOG("[CalcUEDUtil].calcUEDValue: calculated uedValue(" << uedValue << ") is out of range");
            }
            return true;
        }
        return false;
    }

    const char8_t* CalcUEDUtil::getUEDName() const
    {
        if (mUedName.empty())
        {
            const char8_t* name = gUserSessionManager->getUserExtendedDataName(getUEDKey());
            if (name == nullptr)
            {
                //custom UEDs etc can be unnamed
                TRACE_LOG("[CalcUEDUtil].getUEDName: no name currently registered for UED key(" << getUEDKey() << ")");
                return "";
            }
            mUedName = name;
        }
        return mUedName.c_str();
    }
    
    Blaze::UserExtendedDataKey CalcUEDUtil::getUEDKey() const
    {
        if (mUedName.empty() && (mDataKey == INVALID_USER_EXTENDED_DATA_KEY))
        {
            ERR_LOG("[CalcUEDUtil].getUEDKey: ERROR uedName nor uedDataKey provided, can't get value.");
            return INVALID_USER_EXTENDED_DATA_KEY;
        }
        if (mDataKey == INVALID_USER_EXTENDED_DATA_KEY)
        {
            // Lazy Init because user extended data isn't available on configuration.
            if (!gUserSessionManager->getUserExtendedDataKey(getUEDName(), mDataKey))
            {
                TRACE_LOG("[CalcUEDUtil].getDataKey: error: key not found for UED(" << getUEDName() << ")");
            }
        }
        return mDataKey;
    }

    bool CalcUEDUtil::normalizeUEDValue(UserExtendedDataValue& value) const
    {
        UserExtendedDataValue minRange = getMinRange();
        UserExtendedDataValue maxRange = getMaxRange();

        if (value < minRange)
        {
            WARN_LOG("[CalcUEDUtil].normalizeUEDValue: UED(" << getUEDName()
                << ") value (" << value << ") outside of min range, clamping to min range(" << minRange << ")");
            value = minRange;
            return true;
        }

        if (value > maxRange)
        {
            WARN_LOG("[CalcUEDUtil].normalizeUEDValue: UED(" << getUEDName()
                <<  ") value (" << value << ") outside of max range, clamping to max range(" << maxRange << ")");
            value = maxRange;
            return true;
        }

        return false;
    }


    void intrusive_ptr_add_ref(CalcUEDUtil* ptr) { ++ptr->mRefCount; }

    void intrusive_ptr_release(CalcUEDUtil* ptr)
    {
        if (ptr->mRefCount > 0)
        {
            --ptr->mRefCount;
            if (ptr->mRefCount == 0)
                delete ptr;
        }
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
