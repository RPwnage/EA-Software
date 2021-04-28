/*************************************************************************************************/
/*!
    \file   entrycriteria.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_ENTERYCRITERIA_H
#define BLAZE_ENTERYCRITERIA_H

#include "framework/tdf/entrycriteria.h"
#include "framework/util/expression.h"

#include "EASTL/vector_map.h"

namespace Blaze
{

class UserSession;
//class Expression;

typedef eastl::vector_map<EntryCriteriaName, Expression*> ExpressionMap;

/*************************************************************************************************/
/*!
\class EntryCriteriaEvaluator

Evaluator interface defined to evaluate entry criteria.

*/
/*************************************************************************************************/
class EntryCriteriaEvaluator
{
public:

    EntryCriteriaEvaluator() : mCreatedExpressions(false)
    {
    }

    virtual ~EntryCriteriaEvaluator()
    {
    }

    /*! ************************************************************************************************/
    /*!
        \brief Accessor for the entry criteria for this entry criteria evaluator.  These are string
            representations of the entry criteria defined by the client or configs.

        \return EntryCriteriaMap* reference to the entry criteria for this entry criteria evaluator.
    *************************************************************************************************/
    virtual const EntryCriteriaMap& getEntryCriteria() const = 0;

    /*! ************************************************************************************************/
    /*!
        \brief Accessor for the entry criteria expressions for this entry criteria evaluator.  Expressions
            are used to evaluate the criteria against a given user session.

        \return ExpressionMap* reference to the entry criteria expressions for this entry criteria evaluator.
    *************************************************************************************************/
    virtual ExpressionMap& getEntryCriteriaExpressions() = 0;
    virtual const ExpressionMap& getEntryCriteriaExpressions() const = 0;

    /*! ************************************************************************************************/
    /*!
        \brief evaluates all of the entry criteria against the user id.

        \param[in] blazeId The user session to evaluate.
        \param[out] failedCriteria If false this will contain the name of the failed criteria.
        \return true if the user passes all of the entry criteria.
    *************************************************************************************************/
    bool evaluateEntryCriteria(BlazeId blazeId, const UserExtendedDataMap& dataMap, EntryCriteriaName& failedCriteria) const;
    

    /*! ************************************************************************************************/
    /*!
        \brief Iterates through the list of Game Entry Criteria strings and parses them into Expressions.
            Used on the slave during replication since Expressions aren't tdfs and cannot be replicated.

        \return The number of errors encountered during parsing.
    *************************************************************************************************/
    virtual int32_t createCriteriaExpressions();

    /*! ************************************************************************************************/
    /*! \brief clears the criteria expressions and deletes any instantiated objects.
    *************************************************************************************************/
    virtual void clearCriteriaExpressions();

    /*! ************************************************************************************************/
    /*!
        \brief validates the entry criteria is able to be parsed.  Returns true on success or false on the
            first invalid entry criteria.
    
        \param[in] entryCriteriaMap the entry criteria to validate.
        \param[out] failedCriteria the name of the first invalid criteria.
        \param[out] outMap optional ExpressionMap that can be used to skip createCriteriaExpressions() (by clearing the existing expressions and replacing them with these new ones)
        \return true on success, false on the first invalid entry criteria.
    *************************************************************************************************/
    static bool validateEntryCriteria(const EntryCriteriaMap& entryCriteriaMap, EntryCriteriaName& failedCriteria, ExpressionMap* outMap = nullptr);

protected:
    struct ResolveCriteriaInfo
    {
        BlazeId blazeId;
        const UserExtendedDataMap* extendedDataMap;
        bool* success; // using ptr to get around const void* param because this indicates if the resolve succeeded or not
    };

    /*! ************************************************************************************************/
    /*!
        \brief resolves criteria variable during an evaluateEntryCriteria.  Criteria variables are in
            the form of componentName_VariableId (eg. stats_71, or DNF) and rely upon data in the user
            extended data.
    
        \param[in] nameSpace Currently unused but define to match the ResolveVariableCb definition
        \param[in] name The name of the variable to get the value of in the form of componentName_VariableId, eg. stats_71.
        \param[in] type The value type, ie. int.
        \param[in] context Entry Criteria specific context, which resolves to ResolveCriteriaInfo*.
        \param[out] val The value in the UserExtendedData which coresponds to the variable passed in the name.
    *************************************************************************************************/
    static void resolveCriteriaVariable(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type, const void* context, Blaze::Expression::ExpressionVariableVal& val);

    /*! ************************************************************************************************/
    /*!
        \brief Resolves the criteria type when parsing an expression.
    
        \param[in] nameSpace Currently unused but define to match the ResolveVariableCb definition
        \param[in] variable The variable to determine the type of.
        \param[in] context UserSession*
        \param[out] type The resolved criteria type.
    *************************************************************************************************/
    static void resolveCriteriaType(const char8_t* nameSpace, const char8_t* variable, void* context, Blaze::ExpressionValueType& type);

private:
    // Function called by validateEntryCriteria and createCriteriaExpressions.
    static int32_t createCriteriaExpressions(const EntryCriteriaMap& entryCriteriaMap, ExpressionMap* outMap, bool earlyOutOnError, EntryCriteriaName* lastFailedCriteria);
    bool mCreatedExpressions;
};

} // Blaze
#endif // BLAZE_ENTERYCRITERIA_H
