/*************************************************************************************************/
/*!
    \file   getclubrecordsinternal_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubRecordsInternal

    Get club records.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "clubs/rpc/clubsslave/getclubrecordsinternal_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"
#include "clubsconstants.h"

namespace Blaze
{
namespace Clubs
{

class GetClubRecordsInternalCommand : public GetClubRecordsInternalCommandStub, private ClubsCommandBase
{
public:
    GetClubRecordsInternalCommand(Message* message, GetClubRecordsRequest* request, ClubsSlaveImpl* componentImpl)
        : GetClubRecordsInternalCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {
    }

    ~GetClubRecordsInternalCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetClubRecordsInternalCommandStub::Errors execute() override
    {
        BlazeRpcError result = Blaze::ERR_OK;

        TransactionContextPtr ptr = mComponent->getTransactionContext(mRequest.getTransactionContextId());
        if (ptr == nullptr)
        {
            TRACE_LOG("[GetClubRecordsInternalCommand] cannot obtain transaction for transaction context id[" << mRequest.getTransactionContextId() <<"].");
            return ERR_SYSTEM;
        }

        DbConnPtr dbPtr = static_cast<ClubsTransactionContext&>(*ptr).getDatabaseConnection();

        ClubsDatabase clubsDb;
        clubsDb.setDbConn(dbPtr);

        ClubRecordbookRecordMap recordBookRecords;
        result = clubsDb.getClubRecords(mRequest.getClubId(), recordBookRecords);

        if (result != Blaze::ERR_OK)
        {
            ERR_LOG("[GetClubRecordsInternalCommand] Could not get club records, database error " << result);
        }
        else
        {
            // collect user identities
            Blaze::BlazeIdVector idVector;
            for (ClubRecordbookRecordMap::const_iterator it = recordBookRecords.begin(); it != recordBookRecords.end(); ++it)
                idVector.push_back(it->second.blazeId);

            // resolve identities
            Blaze::BlazeIdToUserInfoMap infoMap;
            result = gUserSessionManager->lookupUserInfoByBlazeIds(idVector, infoMap);
            if (result != Blaze::ERR_OK)
            {
                ERR_LOG("[GetClubRecordsInternalCommand] User identity provider error (" << result << ").");
            }
            else
            {
                for (ClubRecordbookRecordMap::const_iterator it = recordBookRecords.begin(); it != recordBookRecords.end(); ++it)
                {
                    ClubRecordId recordId = it->first;
                    ClubRecordbookRecord record = it->second;

                    ClubRecordbookPtr resultRecord = BLAZE_NEW ClubRecordbook();

                    resultRecord->setClubId(record.clubId);
                    resultRecord->setRecordId(record.recordId);
                    resultRecord->setStatType(record.statType);
                    resultRecord->setStatValueInt(record.statValueInt);
                    resultRecord->setStatValueFloat(record.statValueFloat);
                    resultRecord->setLastUpdate(record.lastUpdate);

                    Blaze::BlazeIdToUserInfoMap::const_iterator userItr = infoMap.find(it->second.blazeId);
                    if (userItr != infoMap.end())
                        UserInfo::filloutUserCoreIdentification(*userItr->second, resultRecord->getUser());

                    mResponse.getClubRecordbooks()[recordId] = resultRecord;
                }
            }
        }

        return commandErrorFromBlazeError(result);
    }

};
// static factory method impl
DEFINE_GETCLUBRECORDSINTERNAL_CREATE()

} // Clubs
} // Blaze
