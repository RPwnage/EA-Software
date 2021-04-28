/*! ************************************************************************************************/
/*!
    \file   groupuedexpressionlist.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_GROUP_UED_EXPRESSION_LIST
#define BLAZE_MATCHMAKING_GROUP_UED_EXPRESSION_LIST

#include "gamemanager/tdf/matchmaker_config_server.h"
#include "framework/util/expression.h"
#include "EASTL/vector.h"


namespace Blaze
{
namespace GameManager
{
namespace Matchmaker
{

    /*! ************************************************************************************************/
    /*!
        \class GroupUedExpressionList
        \brief 
    *************************************************************************************************/
    class GroupUedExpressionList
    {
        NON_COPYABLE(GroupUedExpressionList);
    public:

        /*! ************************************************************************************************/
        /*!
            \brief Construct an uninitilized GroupUedExpressionList.  NOTE: do not use until successfully
                initialized.

            GroupUedExpressionLists are typically defined in the matchmaking config file and initialized
            automatically by the owning matchmaking rule at startup.

        *************************************************************************************************/
        GroupUedExpressionList();
        ~GroupUedExpressionList();

        bool initialize(const GroupAdjustmentFormulaList &adjustmentFormulas);

        UserExtendedDataValue calculateAdjustedUedValue(UserExtendedDataValue rawUedValue, uint16_t groupSize) const;
    private:

        static void resolveGroupUedExpressionType(const char8_t* nameSpace, const char8_t* name, void* context, Blaze::ExpressionValueType& type);

        struct ResolveGroupUedInfo
        {
            const UserExtendedDataValue *uedValue;
            const uint16_t *groupSize;
            bool* success; // using ptr to get around const void* param because this indicates if the resolve succeeded or not
        };

        /*! ************************************************************************************************/
        /*!
            \brief resolves criteria variables when caclulating group size adjustments to UED
    
            \param[in] nameSpace Currently unused but defined to match the ResolveVariableCb definition
            \param[in] name The name of the variable, "groupSize" is always processed as the size of the group, other variables use the target UED value
            \param[in] type The value type, ie. int.
            \param[in] context group ued expression specific context, which resolves to ResolveGroupUedInfo*.
            \param[out] val The reputation value to evaluate.
        *************************************************************************************************/
        static void resolveGroupUedVariable(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type, const void* context, Blaze::Expression::ExpressionVariableVal& val);



        typedef eastl::vector<Expression*> ExpressionList;
        ExpressionList mGroupUedExpressions;

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_MATCHMAKING_GROUP_UED_EXPRESSION_LIST
