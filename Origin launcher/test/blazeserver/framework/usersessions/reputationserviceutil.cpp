/*! ************************************************************************************************/
/*!
    \file reputationserviceutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/usersessions/reputationserviceutil.h"
#include "framework/usersessions/reputationserviceutilfactory.h"
#include "framework/usersessions/usersessionmanager.h"


namespace Blaze
{
namespace ReputationService
{

bool ReputationServiceUtil::isMock() const
{
    return (gUserSessionManager->getConfig().getReputation().getUseMock() || gController->getUseMockConsoleServices());
}

bool ReputationServiceUtil::doSessionReputationUpdate(UserSessionId userSessionId) const
{
    bool hasPoorReputation = gUserSessionManager->getConfig().getReputation().getReputationDefaultValue();
    UserInfoData userInfoData;
    UserSessionExtendedData userExtendedData;
    ServiceName validServiceName;

    if ((gUserSessionManager->getRemoteUserInfo(userSessionId, userInfoData) != ERR_OK) ||
        (gUserSessionManager->getRemoteUserExtendedData(userSessionId, userExtendedData) != ERR_OK))
    {
        // skip if we can't find the user session
        BLAZE_WARN_LOG(Log::USER, "[ReputationServiceUtil].doReputationUpdate: Could not retrieve UserSession data with Id '" << userSessionId << "' when attempting to update reputation.");
        return hasPoorReputation;
    }
    gUserSessionManager->getServiceName(userSessionId, validServiceName);

    BlazeRpcError err = updateReputation(userInfoData, validServiceName.c_str(), userExtendedData, hasPoorReputation);
    // just log that an error occurred
    if (EA_UNLIKELY(err != ERR_OK))
    {
        BLAZE_WARN_LOG(Log::USER, "[ReputationServiceUtil].doReputationUpdate: User " << userInfoData.getId() 
            << " with external Id'" << userInfoData.getPlatformInfo().getExternalIds().getXblAccountId() << "' got error '" << ErrorHelp::getErrorName(err) << "' when attempting to update reputation.");
    }

    return hasPoorReputation;
}


bool ReputationServiceUtil::doReputationUpdate(const UserSessionIdList &userSessionIdList, const UserIdentificationList& externalUserList) const
{
    bool foundPoorReputationUser = false;
    // check all the logged-in users explicitly
    UserSessionIdList::const_iterator sessionIdIter = userSessionIdList.begin();
    UserSessionIdList::const_iterator sessionIdEnd = userSessionIdList.end();
    for (; sessionIdIter != sessionIdEnd; ++sessionIdIter)
    {
        UserSessionId userSessionId = *sessionIdIter;
        if (doSessionReputationUpdate(userSessionId))
            foundPoorReputationUser = true;
    }

    if (!foundPoorReputationUser)
    {
        // no external rep provider, just use the default lookup value
        foundPoorReputationUser = gUserSessionManager->getConfig().getReputation().getReputationDefaultValue();
    }

    return foundPoorReputationUser;
}

// static
// retrieves the external reputation and updates the UED of the given UserSession if the value has changed
Blaze::BlazeRpcError ReputationServiceUtil::updateReputation( const UserInfoData &userInfoData, const char8_t* serviceName, UserSessionExtendedData &userExtendedData, bool& hasPoorReputation ) const
{ 
    // internal rep based on UED, it's always updated when the source value changes
    hasPoorReputation = userHasPoorReputation(userExtendedData); 
    return ERR_OK; 
}


// extended data provider interface
BlazeRpcError ReputationServiceUtil::onLoadExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData)
{ 
    const ReputationConfig& reputationConfig = gUserSessionManager->getConfig().getReputation();
    UserExtendedDataMap& userMap = extendedData.getDataMap();
    bool isPoorReputation = gUserSessionManager->getConfig().getReputation().getReputationDefaultValue();

    if(reputationConfig.getUseExternalReputation())
    {
        // For external reputation we use the default values to begin with, and will call doSessionReputationUpdate later. 
    }
    else // internal reputation, we depend on stored UED
    {
        //lookup the key
        UserExtendedDataKey reputationSourceKey = 0;
        if(gUserSessionManager->getUserExtendedDataKey(reputationConfig.getReputationSourceUEDName(), reputationSourceKey))
        {
            // lookup succeeded
            UserExtendedDataValue sourceValue = (userMap)[reputationSourceKey];
            // calculate reputation

            isPoorReputation = evaluateReputation(sourceValue);
        }
    }

    UserExtendedDataKey reputationKey = UED_KEY_FROM_IDS(UserSessionManager::COMPONENT_ID,EXTENDED_DATA_REPUTATION_VALUE_KEY);

    BLAZE_TRACE_LOG(Log::USER, "[ReputationServiceUtil].onLoadExtendedData: inserting '" << (UserExtendedDataValue)isPoorReputation << "' into UED for user blaze ID '" 
        << data.getId() << "'.");
    //pack extended data
    (userMap)[reputationKey] = (UserExtendedDataValue)isPoorReputation;
    // always return ERR_OK
    return ERR_OK; 
}

void ReputationServiceUtil::resolveReputationVariable(const char8_t* nameSpace, const char8_t* name, Blaze::ExpressionValueType type, const void* context, Blaze::Expression::ExpressionVariableVal& val)
{
    const ResolveReputationInfo* info = static_cast<const ResolveReputationInfo*>(context);
    *(info->success) = false;

    // need this init to avoid a memory checking tool error; the value will be irrelevant if the resolve is not successful
    val.intVal = 0;
    if (type == Blaze::EXPRESSION_TYPE_INT)
    {
        val.intVal = *(info->reputationValue);
        *(info->success) = true;
        return;
    }

    BLAZE_ERR_LOG(Log::USER, "[ReputationServiceUtil].resolveReputationVariable: resolve failed expression type wasn't an int.");
}

// static
bool ReputationServiceUtil::evaluateReputation(const UserExtendedDataValue &value)
{
    // now evaluate the reputation
    bool resolveSuccess;
    ResolveReputationInfo info;
    info.reputationValue = &value; 
    info.success = &resolveSuccess;

    // Comments in EntryCriteriaEvaluator indicate load testing showed that constructing this functor was a time sink
    // and declared static. Followed this pattern as the reputation resolve will have a similar call rate.
    static Expression::ResolveVariableCb resolveReputationVariableCb(&ReputationServiceUtil::resolveReputationVariable);


    const Expression* repExpression = gUserSessionManager->getReputationServiceUtilFactory().getReputationExpression();
    // can be null
    if ( repExpression != nullptr)
    {
        int64_t eval = repExpression->evalInt(resolveReputationVariableCb, &info);
        if (resolveSuccess)
        {
            // reputation expression return value treated as bool, true is equivalent to having poor reputation
            return eval != 0;
        }
    }

    BLAZE_WARN_LOG(Log::USER, "[ReputationServiceUtil].evaluateReputation: evaluateReputation failed, using default reputation value.");
    return gUserSessionManager->getConfig().getReputation().getReputationDefaultValue();
}

BlazeRpcError ReputationServiceUtil::onRefreshExtendedData(const UserInfoData& data, UserSessionExtendedData &extendedData) 
{ 
    if (!gUserSessionManager->getConfig().getReputation().getUseExternalReputation())
    {
        return onLoadExtendedData(data, extendedData); 
    }

    // don't do anything if we're using external reputation
    return ERR_OK;
}

// static
// returns true if the given user is a has a good reputation, based on UED and the configured goodReputationValidationExpression and reputationUEDName
bool ReputationServiceUtil::userHasPoorReputation( const UserSessionExtendedData& userExtendedData )
{ 
    return userHasPoorReputation(userExtendedData.getDataMap()); 
}

// static
bool ReputationServiceUtil::userHasPoorReputation(const UserExtendedDataMap& userExtendedDataMap)
{ 
    UserExtendedDataValue hasPoorReputation = gUserSessionManager->getConfig().getReputation().getReputationDefaultValue();
    UserSession::getDataValue(userExtendedDataMap, UserSessionManager::COMPONENT_ID, EXTENDED_DATA_REPUTATION_VALUE_KEY, hasPoorReputation);
    return hasPoorReputation != 0; 
}

}
}

