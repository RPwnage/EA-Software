/*************************************************************************************************/
/*!
    \file   updateclubsutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "updateclubsutil.h"

namespace Blaze
{
namespace GameReporting
{

#ifdef TARGET_clubs
bool UpdateClubsUtil::initialize(Clubs::ClubIdList &clubsToLock)
{
    // check if already initialized
    if (mInitialized)
    {
        return false;
    }

    mComponent = static_cast<Clubs::ClubsSlave*>(gController->getComponent(Clubs::ClubsSlave::COMPONENT_ID, true));
    if (mComponent == nullptr)
    {
        return false;
    }

    // start the remote transaction request.
    Blaze::StartRemoteTransactionRequest startRequest;
    Blaze::StartRemoteTransactionResponse startResponse;

    startRequest.setComponentId(Clubs::ClubsSlave::COMPONENT_ID);
    startRequest.setCustomData(12);
    startRequest.setTimeout(20000000);

    // fetch the transaction context id.
    BlazeRpcError err = gController->startRemoteTransaction(startRequest, startResponse);

    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].initialize: Error starting remote transaction (" << (ErrorHelp::getErrorName(err)) << ").");
        return false;
    }

    mTransactionContextId = startResponse.getTransactionId();

    Blaze::Clubs::LockClubsRequest lockClubsRequest;
    lockClubsRequest.setTransactionContextId(mTransactionContextId);
    clubsToLock.copyInto(lockClubsRequest.getClubIds());

    RpcCallOptions opts;
    opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
    err = mComponent->lockClubsInternal(lockClubsRequest, opts);

    if (err != ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].initialize: Error locking clubs (" << (ErrorHelp::getErrorName(err)) << ").");
        return false;
    }

    mInitialized = true;

    return true;
}

void UpdateClubsUtil::completeTransaction(bool success)
{
    TRACE_LOG("[UpdateClubsUtil].completeTransaction: Completing transaction on remote transaction " << mTransactionContextId);

    // Only take action if we actually own the remote transaction
    if (mTransactionContextId != 0)
    {
        if (mInitialized)
        {
            // If we opened a transaction, we need to commit or rollback now depending on the success status
            TRACE_LOG("[UpdateClubsUtil].completeTransaction: Completing transaction on remote transaction " << mTransactionContextId 
                      << " with " << (success? "COMMIT" : "ROLLBACK"));

            BlazeRpcError err;
            
            // Finish the remote transaction.
            Blaze::CompleteRemoteTransactionRequest completeRequest;
            completeRequest.setComponentId(Clubs::ClubsSlave::COMPONENT_ID);
            completeRequest.setTransactionId(mTransactionContextId);
            completeRequest.setCommitTransaction(success);
            err = gController->completeRemoteTransaction(completeRequest);

            if (err != Blaze::ERR_OK)
            {
                ERR_LOG("[UpdateClubsUtil].completeTransaction: Commiting or rollbacking database transaction was not successful");
            }
        }

        TRACE_LOG("[UpdateClubsUtil].completeTransaction: Releasing remote transaction.");

        mInitialized = false;
    }
}

/*************************************************************************************************/
/*!
    \brief updateClubs

    Extracts data fom game report to update clubs

    \param[in]  firstClubId      - the ID of first club
    \param[in]  firstClubScore   - the score of first club
    \param[in]  secondClubId     - the ID of second club
    \param[in]  secondClubPoints - the score of second club

    \return - true if everything went ok, false otherwise
*/
/*************************************************************************************************/
bool UpdateClubsUtil::updateClubs(Clubs::ClubId firstClubId, int32_t firstClubPoints,
                                  Clubs::ClubId secondClubId, int32_t secondClubPoints)
{
    if (!mInitialized)
    {
        return false;
    }

    eastl::string upperResult, lowerResult;

    eastl::string finalUpperRes;
    eastl::string finalLowerRes;
    const char *format = "%d:%d";
    finalUpperRes.sprintf(format, firstClubPoints, secondClubPoints); 
    finalLowerRes.sprintf(format, secondClubPoints, firstClubPoints);

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (mComponent != nullptr)
    {
        Blaze::Clubs::ReportResultsRequest reportResultRequest;
        reportResultRequest.setTransactionContextId(mTransactionContextId);
        reportResultRequest.setUpperClubId(firstClubId);
        reportResultRequest.setGameResultForUpperClub(finalUpperRes.c_str());
        reportResultRequest.setLowerClubId(secondClubId);
        reportResultRequest.setGameResultForLowerClub(finalLowerRes.c_str());

        RpcCallOptions opts;
        opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
        err = mComponent->reportResultsInternal(reportResultRequest, opts);
    }   

    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].updateClubs: Error updating clubs. Club ids(" << firstClubId <<"," << secondClubId <<"). Error(" << (ErrorHelp::getErrorName(err)) << ").");
    }
    return (err == Blaze::ERR_OK);
}

/*************************************************************************************************/
/*!
    \brief fetchClubRecords

    Fetches club records for the specified club

    \param[in]  clubId - club ID

    \return - true if everything went ok, false otherwise
*/
/*************************************************************************************************/
bool UpdateClubsUtil::fetchClubRecords(Clubs::ClubId clubId)
{
    if (!mInitialized)
    {
        return false;
    }

    // make sure the map is empty 
    mRecordMap.clear();

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (mComponent != nullptr)
    {
        Blaze::Clubs::GetClubRecordsRequest getClubRecordsRequest;
        Blaze::Clubs::GetClubRecordsResponse getClubRecordsResponse;

        getClubRecordsRequest.setTransactionContextId(mTransactionContextId);
        getClubRecordsRequest.setClubId(clubId);

        RpcCallOptions opts;
        opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
        err = mComponent->getClubRecordsInternal(getClubRecordsRequest, getClubRecordsResponse, opts);
        if (err == Blaze::ERR_OK)
        {
            getClubRecordsResponse.getClubRecordbooks().copyInto(mRecordMap);
        }
    }
    
    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].fetchClubRecords: Error fetching club records. Club id("<< clubId <<"). Error (" << (ErrorHelp::getErrorName(err)) << ").");
    }
    return (err == Blaze::ERR_OK);
}

/*************************************************************************************************/
/*!
    \brief getClubRecord

    Returns the fetched club record for the specified record IDs

    \param[in]     recordId - record ID as configured in clubs.cfg
    \param[in/out] record   - the record to read the fetched club record into

    \return - true if the record is found in map
*/
/*************************************************************************************************/
bool UpdateClubsUtil::getClubRecord(Clubs::ClubRecordId recordId, Clubs::ClubRecordbook &record)
{
    Clubs::ClubRecordbookMap::const_iterator it = mRecordMap.find(recordId);

    if (it == mRecordMap.end())
    {
        return false;
    }

   (it->second)->copyInto(record);

    return true;
}

/*************************************************************************************************/
/*!
    \brief updateClubRecordInt

    Updates existing club record for the specified club and record IDs

    \param[in]  dbconn - database connection to use to access clubs DB.
                clubId - club ID
                recordId - record ID as configured in clubs.cfg
                newRecordHolder - blaze ID of the new record holder
                recordStatVal - the new int stat value for this record

    \return - true if everything went ok, false otherwise
*/
/*************************************************************************************************/
bool UpdateClubsUtil::updateClubRecordInt(Clubs::ClubId clubId, Clubs::ClubRecordId recordId, 
                                          BlazeId newRecordHolder, int64_t recordStatVal)
{
    if (!mInitialized)
    {
        return false;
    }

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (mComponent != nullptr)
    {
        Blaze::Clubs::UpdateClubRecordRequest updateClubRecordRequest;        

        updateClubRecordRequest.setTransactionContextId(mTransactionContextId);
        updateClubRecordRequest.getClubRecordbook().setClubId(clubId);
        updateClubRecordRequest.getClubRecordbook().setRecordId(recordId);
        updateClubRecordRequest.getClubRecordbook().getUser().setBlazeId(newRecordHolder);
        updateClubRecordRequest.getClubRecordbook().setStatValueInt(recordStatVal);
        updateClubRecordRequest.getClubRecordbook().setStatType(Clubs::CLUBS_RECORD_STAT_INT);

        RpcCallOptions opts;
        opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
        err = mComponent->updateClubRecordInternal(updateClubRecordRequest, opts);
    }
   
    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].updateClubRecordInt: Error updating club records. Club id("<< clubId <<"). Record Id("<< recordId <<"). New Record Holder("<<
            newRecordHolder <<"). Error(" << (ErrorHelp::getErrorName(err)) << ").");
    }
    return (err == Blaze::ERR_OK);
}

/*************************************************************************************************/
/*!
    \brief updateClubRecordFloat

    Updates existing club record  for the specified club and record IDs

    \param[in]  dbconn - database connection to use to access clubs DB.
                clubId - club ID
                recordId - record ID as configured in clubs.cfg
                newRecordHolder - blaze ID of the new record holder
                recordStatVal - the new float stat value for this record
    
    \return - true if everything went ok, false otherwise
*/
/*************************************************************************************************/
bool UpdateClubsUtil::updateClubRecordFloat(Clubs::ClubId clubId, Clubs::ClubRecordId recordId, 
                                            BlazeId newRecordHolder, float_t recordStatVal)
{
    if (!mInitialized)
    {
        return false;
    }

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (mComponent != nullptr)
    {
        Blaze::Clubs::UpdateClubRecordRequest updateClubRecordRequest;        

        updateClubRecordRequest.setTransactionContextId(mTransactionContextId);
        updateClubRecordRequest.getClubRecordbook().setClubId(clubId);
        updateClubRecordRequest.getClubRecordbook().setRecordId(recordId);
        updateClubRecordRequest.getClubRecordbook().getUser().setBlazeId(newRecordHolder);
        updateClubRecordRequest.getClubRecordbook().setStatValueFloat(recordStatVal);
        updateClubRecordRequest.getClubRecordbook().setStatType(Clubs::CLUBS_RECORD_STAT_FLOAT);

        RpcCallOptions opts;
        opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
        err = mComponent->updateClubRecordInternal(updateClubRecordRequest, opts);
    }

    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].updateClubRecordFloat: Error updating club records. Club id("<< clubId <<"). Record Id("<< recordId <<"). New Record Holder("<<
            newRecordHolder <<"). Error(" << (ErrorHelp::getErrorName(err)) << ").");
    }
    return (err == Blaze::ERR_OK);
}

/*************************************************************************************************/
/*!
    \brief updateClubRival

    Update a club rival

    \param[in]  clubId - clubid of the club to fetch
                rival  - rival for the clubId

    \return - true if id is valid, false otherwise or in case of system error
*/
/*************************************************************************************************/
bool UpdateClubsUtil::updateClubRival(Clubs::ClubId clubId, Clubs::ClubRival &rival)
{
    if (!mInitialized)
    {
        return false;
    }

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (mComponent != nullptr)
    {
        Blaze::Clubs::UpdateClubRivalRequest updateClubRivalRequest;        

        updateClubRivalRequest.setTransactionContextId(mTransactionContextId);
        rival.copyInto(updateClubRivalRequest.getClubRival());

        RpcCallOptions opts;
        opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
        err = mComponent->updateClubRivalInternal(updateClubRivalRequest, opts);
    }

    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].updateClubRival: Error updating club rival. Club Id(" << clubId << "). Error("<< (ErrorHelp::getErrorName(err)) << ").");
    }
    return (err == Blaze::ERR_OK);
}

/*************************************************************************************************/
/*!
    \brief awardClub

    Awards a club

    \param[in]  clubId  - Id of club to award
                awardId - Id of award

    \return - true if club is successfully awarded, false otherwise
*/
/*************************************************************************************************/
bool UpdateClubsUtil::awardClub(Clubs::ClubId clubId, Clubs::ClubAwardId awardId)
{
    if (!mInitialized)
    {
        return false;
    }

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (mComponent != nullptr)
    {

        Blaze::Clubs::AwardClubRequest awardClubRequest;
        awardClubRequest.setTransactionContextId(mTransactionContextId);
        awardClubRequest.setClubId(clubId);
        awardClubRequest.setAwardId(awardId);

        RpcCallOptions opts;
        opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
        err = mComponent->awardClubInternal(awardClubRequest, opts);
    }

    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].awardClub: Error awarding club. Club id("<< clubId <<"). Award id("<< awardId <<"). Error(" << (ErrorHelp::getErrorName(err)) << ").");
    }
    return (err == Blaze::ERR_OK);
}

/*************************************************************************************************/
/*!
    \brief checkMembership

    Checks for user membership in a given club

    \param[in]  clubId  - club ID
                blazeId - the blaze ID to check membership for

    \return - true if user is member, false otherwise or in case of system error
*/
/*************************************************************************************************/
bool UpdateClubsUtil::checkMembership(Clubs::ClubId clubId, BlazeId blazeId)
{
    if (!mInitialized)
    {
        return false;
    }

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (mComponent != nullptr)
    {
        Blaze::Clubs::CheckMembershipRequest checkMembershipRequest;
        checkMembershipRequest.setTransactionContextId(mTransactionContextId);
        checkMembershipRequest.setClubId(clubId);
        checkMembershipRequest.setBlazeId(blazeId);

        RpcCallOptions opts;
        opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
        err = mComponent->checkMembershipInternal(checkMembershipRequest, opts);
    }

    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].checkMembership: Error checking membership. Club id("<< clubId <<"). User id("<< blazeId <<"). Error(" << (ErrorHelp::getErrorName(err)) << ").");
    }
    return (err == Blaze::ERR_OK);
}

/*************************************************************************************************/
/*!
    \brief validateClubId

    Checks if club id is valid

    \param[in]  clubId - ID of the club to check

    \return - true if id is valid, false otherwise or in case of system error
*/
/*************************************************************************************************/
bool UpdateClubsUtil::validateClubId(Clubs::ClubId clubId)
{
    Clubs::ClubsSlave* component;
    TransactionContextId transactionContextId;

    // Blocking call can potentially alter mInitialized, so cache it off here so that we will correctly complete the transaction
    bool initialized = mInitialized;

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (initialized)
    {
        if (mComponent == nullptr)
        {
            return false;
        }
        component = mComponent;
        transactionContextId = mTransactionContextId;
    }
    else
    {
        component = static_cast<Clubs::ClubsSlave*>(gController->getComponent(Clubs::ClubsSlave::COMPONENT_ID, true));

        if (component == nullptr)
        {
            return false;
        }

        // start the remote transaction request.
        Blaze::StartRemoteTransactionRequest startRequest;
        Blaze::StartRemoteTransactionResponse startResponse;

        startRequest.setComponentId(Clubs::ClubsSlave::COMPONENT_ID);
        startRequest.setCustomData(13);
        startRequest.setTimeout(20000000);

        // fetch the transaction context id.
        err = gController->startRemoteTransaction(startRequest, startResponse);

        if (err != Blaze::ERR_OK)
        {
            ERR_LOG("[UpdateClubsUtil].validateClubId: Error starting remote transaction. Club id("<< clubId <<"). Error(" << (ErrorHelp::getErrorName(err)) << ").");
            return false;
        }

        transactionContextId = startResponse.getTransactionId();
    }

    Blaze::Clubs::CheckClubIdRequest checkClubIdRequest;
    checkClubIdRequest.setTransactionContextId(transactionContextId);
    checkClubIdRequest.setClubId(clubId);

    RpcCallOptions opts;
    opts.routeTo.setInstanceIdFromKey(transactionContextId);
    err = component->checkClubIdInternal(checkClubIdRequest, opts);
    // track whether the actual rpc succeeded or not separately.
    // and log error but don't return early as we want to finish the transaction. 
    bool success = (err == Blaze::ERR_OK); 
    if (!success)
    {
        ERR_LOG("[UpdateClubsUtil].validateClubId: Error checking club id. Club id("<< clubId <<"). Error("<< (ErrorHelp::getErrorName(err)) << ").");
    }

    if (!initialized)
    {
        Blaze::CompleteRemoteTransactionRequest completeRequest;
        completeRequest.setComponentId(Clubs::ClubsSlave::COMPONENT_ID);
        completeRequest.setTransactionId(transactionContextId);
        completeRequest.setCommitTransaction(success);
        err = gController->completeRemoteTransaction(completeRequest);

        if (err != Blaze::ERR_OK)
        {
            ERR_LOG("[UpdateClubsUtil].validateClubId: Commiting or rollbacking database transaction was not successful. . Club id("<< clubId <<").");
        }
    }

    return (err == Blaze::ERR_OK) && success;
}

/*************************************************************************************************/
/*!
    \brief checkUserInClub

    Checks if user belongs to any club

    \param[in]  blazeId - id of user to check membership for

    \return - true if user belongs to any club in any domain, false otherwise or in case of system error
*/
/*************************************************************************************************/
bool UpdateClubsUtil::checkUserInClub(BlazeId blazeId)
{
    Clubs::ClubsSlave* component;
    TransactionContextId transactionContextId;

    // Blocking call can potentially alter mInitialized, so cache it off here so that we will correctly complete the transaction
    bool initialized = mInitialized;

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (initialized)
    {
        if (mComponent == nullptr)
        {
            return false;
        }
        component = mComponent;
        transactionContextId = mTransactionContextId;
    }
    else
    {
        component = static_cast<Clubs::ClubsSlave*>(gController->getComponent(Clubs::ClubsSlave::COMPONENT_ID, true));

        if (component == nullptr)
        {
            return false;
        }

        // start the remote transaction request.
        Blaze::StartRemoteTransactionRequest startRequest;
        Blaze::StartRemoteTransactionResponse startResponse;

        startRequest.setComponentId(Clubs::ClubsSlave::COMPONENT_ID);
        startRequest.setCustomData(14);
        startRequest.setTimeout(20000000);

        // fetch the transaction context id.
        err = gController->startRemoteTransaction(startRequest, startResponse);

        if (err != Blaze::ERR_OK)
        {
            ERR_LOG("[UpdateClubsUtil].checkUserInClub: Error starting remote transaction. User id("<< blazeId <<"). Error("<< (ErrorHelp::getErrorName(err)) << ").");
            return false;
        }

        transactionContextId = startResponse.getTransactionId();
    }

    Blaze::Clubs::CheckUserInClubRequest checkUserInClubRequest;
    checkUserInClubRequest.setTransactionContextId(transactionContextId);
    checkUserInClubRequest.setBlazeId(blazeId);

    RpcCallOptions opts;
    opts.routeTo.setInstanceIdFromKey(transactionContextId);
    err = component->checkUserInClubInternal(checkUserInClubRequest, opts);
    // track whether the actual rpc succeeded or not separately.
    // and log error but don't return early as we want to finish the transaction. 
    bool success = (err == Blaze::ERR_OK);
    if (!success)
    {
        ERR_LOG("[UpdateClubsUtil].checkUserInClub: Error checking user in club. User id("<< blazeId <<"). Error("<< (ErrorHelp::getErrorName(err)) << ").");
    }

    if (!initialized)
    {
        Blaze::CompleteRemoteTransactionRequest completeRequest;
        completeRequest.setComponentId(Clubs::ClubsSlave::COMPONENT_ID);
        completeRequest.setTransactionId(transactionContextId);
        completeRequest.setCommitTransaction(success);
        err = gController->completeRemoteTransaction(completeRequest);

        if (err != Blaze::ERR_OK)
        {
            ERR_LOG("[UpdateClubsUtil].checkUserInClub: Commiting or rollbacking database transaction was not successful. User id("<< blazeId <<").");
        }
    }

    return (err == Blaze::ERR_OK) && success;
}

/*************************************************************************************************/
/*!
    \brief getClubSetting

    Fetches the club setting

    \param[in]     clubId       - ID of the club to fetch
    \param[in/out] clubSettings - clubSettings object to fetch ClubSettings into

    \return - true if id is valid, false otherwise or in case of system error
*/
/*************************************************************************************************/
bool UpdateClubsUtil::getClubSettings(Clubs::ClubId clubId, Clubs::ClubSettings &clubSettings)
{
    if (!mInitialized)
    {
        return false;
    }

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (mComponent != nullptr)
    {
        Clubs::GetClubSettingsRequest getClubSettingsRequest;
        Clubs::GetClubSettingsResponse getClubSettingsResponse;

        getClubSettingsRequest.setTransactionContextId(mTransactionContextId);
        getClubSettingsRequest.setClubId(clubId);

        RpcCallOptions opts;
        opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
        err = mComponent->getClubSettingsInternal(getClubSettingsRequest, getClubSettingsResponse, opts);
        if (err == Blaze::ERR_OK)
        {
            getClubSettingsResponse.getClubSettings().copyInto(clubSettings);
        }
    }
    
    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].getClubSettings: Error in getting club settings. Club Id("<< clubId <<"). Error(" << (ErrorHelp::getErrorName(err)) << ").");
    }
    return (err == Blaze::ERR_OK);
}

/*************************************************************************************************/
/*!
    \brief getClubsComponentSettings

    Fetches the clubs component settings

    \param[in/out]  settings - clubs component settings 

    \return - true if id is valid, false otherwise or in case of system error
*/
/*************************************************************************************************/
bool UpdateClubsUtil::getClubsComponentSettings(Clubs::ClubsComponentSettings &settings)
{
    if (!mInitialized)
    {
        return false;
    }

    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (mComponent != nullptr)
    {
        Clubs::ClubsComponentSettings response;
        RpcCallOptions opts;
        opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
        err = mComponent->getClubsComponentSettingsInternal(response, opts);
        if (err == Blaze::ERR_OK)
        {
            response.copyInto(settings);
        }
    }
    
    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].getClubsComponentSettings: Error in getting club component settings (" << (ErrorHelp::getErrorName(err)) << ").");
    }
    return (err == Blaze::ERR_OK);
}

/*************************************************************************************************/
/*!
    \brief listClubRivals

    Fetches a list of club rivals

    \param[in]     clubId    - clubid of the club to fetch rivals for
    \param[in/out] rivalList - list of rivals

    \return - true if id is valid, false otherwise or in case of system error
*/
/*************************************************************************************************/
bool UpdateClubsUtil::listClubRivals(Clubs::ClubId clubId, Clubs::ClubRivalList &rivalList)
{
    if (!mInitialized)
    {
        return false;
    }
    
    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (mComponent != nullptr)
    {
        Clubs::ListRivalsInternalRequest listRivalsRequest;
        Clubs::ListRivalsResponse listRivalsResponse;

        listRivalsRequest.setTransactionContextId(mTransactionContextId);
        listRivalsRequest.setClubId(clubId);

        RpcCallOptions opts;
        opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
        err = mComponent->listRivalsInternal(listRivalsRequest, listRivalsResponse, opts);
        if (err == Blaze::ERR_OK)
        {
            listRivalsResponse.getClubRivalList().copyInto(rivalList);
        }
    }

    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].listClubRivals: Error in listing club rivals. Club id("<< clubId <<"). Error(" << (ErrorHelp::getErrorName(err)) << ").");
    }
    return (err == Blaze::ERR_OK);
}

/*************************************************************************************************/
/*!
    \brief listClubMembers

    Fetches a list of club members

    \param[in]     clubId     - ID of the club to fetch member list for
    \param[in/out] memberList - list of club members

    \return - true if id is valid, false otherwise or in case of system error
*/
/*************************************************************************************************/
bool UpdateClubsUtil::listClubMembers(Clubs::ClubId clubId, Clubs::ClubMemberList &memberList)
{
    if (!mInitialized)
    {
        return false;
    }
    
    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (mComponent != nullptr)
    {
        Clubs::GetMembersInternalRequest getMembersRequest;
        Clubs::GetMembersResponse getMembersResponse;

        getMembersRequest.setTransactionContextId(mTransactionContextId);
        getMembersRequest.setClubId(clubId);

        RpcCallOptions opts;
        opts.routeTo.setInstanceIdFromKey(mTransactionContextId);
        err = mComponent->getMembersInternal(getMembersRequest, getMembersResponse, opts);
        if (err == Blaze::ERR_OK)
        {
            getMembersResponse.getClubMemberList().copyInto(memberList);
        }
    }

    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[UpdateClubsUtil].listClubMembers: Error in listing club members. Club id("<< clubId <<"). Error("<< (ErrorHelp::getErrorName(err)) << ").");
    }
    return (err == Blaze::ERR_OK);
}
#endif

} //namespace GameReporting
} //namespace Blaze
