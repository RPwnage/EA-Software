/*************************************************************************************************/
/*!
    \file   getclubrecordbook_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GetClubRecordbookCommand

    get a list of record holders of a club

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/localization.h"

// clubs includes
#include "clubs/rpc/clubsslave/getclubrecordbook_stub.h"
#include "clubsslaveimpl.h"
#include "clubscommandbase.h"

namespace Blaze
{
namespace Clubs
{

class GetClubRecordbookCommand : public GetClubRecordbookCommandStub, private ClubsCommandBase
{
public:
    GetClubRecordbookCommand(Message* message, GetClubRecordbookRequest* request, ClubsSlaveImpl* componentImpl)
        : GetClubRecordbookCommandStub(message, request),
        ClubsCommandBase(componentImpl)
    {

    }

    ~GetClubRecordbookCommand() override
    {
    }

    /* Private methods *******************************************************************************/
private:
    GetClubRecordbookCommandStub::Errors execute() override
    {
        TRACE_LOG("[GetClubRecordbookCommand] start");

        ClubsDbConnector dbc(mComponent); 
        if (dbc.acquireDbConnection(true) == nullptr)
            return ERR_SYSTEM;

        TRACE_LOG("[GetClubRecordbookCommand] received DB connection.");

        ClubRecordbookRecordMap clubRecordMap;
        BlazeRpcError error = dbc.getDb().getClubRecords(mRequest.getClubId(), clubRecordMap);
        dbc.releaseConnection();

        if (error != Blaze::ERR_OK)
        {
            if (error != Blaze::CLUBS_ERR_INVALID_CLUB_ID)
            {
                ERR_LOG("[GetClubRecordbookCommand] Database error.");
            }
            return commandErrorFromBlazeError(error);
        }

        RecordSettingItemList::const_iterator it = mComponent->getRecordSettings().begin();
        RecordSettingItemList::const_iterator end = mComponent->getRecordSettings().end();
        char8_t statStringBuf[64];

        // collect user identities
        ClubRecordbookRecordMap::const_iterator itrm = clubRecordMap.begin();
        Blaze::BlazeIdVector idVector;
        for (; itrm != clubRecordMap.end(); itrm++)
            idVector.push_back(itrm->second.blazeId);

        // resolve identities
        Blaze::BlazeIdToUserInfoMap infoMap;
        error = gUserSessionManager->lookupUserInfoByBlazeIds(idVector, infoMap);
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[GetClubRecordbookCommand] User identity provider error (" << ErrorHelp::getErrorName(error) << ").");
            return commandErrorFromBlazeError(error);
        }

        for(;it != end; ++it)
        {
            RecordStatType type;
            if (blaze_strcmp((*it)->getStatType(), RECORD_STAT_TYPE_INT) == 0)
            {
                type = CLUBS_RECORD_STAT_INT;
            }
            else if (blaze_strcmp((*it)->getStatType(), RECORD_STAT_TYPE_FLOAT) == 0)
            {
                type = CLUBS_RECORD_STAT_FLOAT;
            }
            else
            {
                ERR_LOG("[GetClubRecordbookCommand] unknown record stat type " << (*it)->getStatType());
                error = Blaze::ERR_SYSTEM;
                return commandErrorFromBlazeError(error);
            }

            ClubRecord *cr = mResponse.getClubRecordList().pull_back();
            ClubRecordbookRecordMap::iterator ritr = clubRecordMap.find((*it)->getId());
            if (ritr != clubRecordMap.end())
            {
                ClubRecordbookRecord &crb = ritr->second;
                Blaze::BlazeIdToUserInfoMap::const_iterator itui = infoMap.find(crb.blazeId);
                if (itui != infoMap.end())
                {
                    UserInfo::filloutUserCoreIdentification(*itui->second, cr->getUser());
                }
                cr->getUser().setBlazeId(crb.blazeId);
                cr->setLastUpdateTime(crb.lastUpdate);

                switch (type)
                {
                case CLUBS_RECORD_STAT_INT:
                    {
                        eastl::string numberFormat = (*it)->getFormat();
                        int position = numberFormat.find("d");
                        if(position >= 0)
                            numberFormat.replace(position,1,PRId64);
                        blaze_snzprintf(statStringBuf, sizeof(statStringBuf),
                            numberFormat.c_str(), crb.statValueInt);
                        cr->setRecordStat(statStringBuf);
                        break;
                    }
                case CLUBS_RECORD_STAT_FLOAT:
                    {
                        blaze_snzprintf(statStringBuf, sizeof(statStringBuf),
                            (*it)->getFormat(), crb.statValueFloat);
                        cr->setRecordStat(statStringBuf);
                        break;
                    }
                default:
                    {
                        ERR_LOG("[GetClubRecordbookCommand] unknown record stat type " << type);
                        error = Blaze::ERR_SYSTEM;
                        return commandErrorFromBlazeError(error);
                    }
                }
            }
            cr->setRecordId((*it)->getId());
            cr->setRecordName(gLocalization->localize((*it)->getName(), gCurrentUserSession->getSessionLocale()));
            cr->setRecordDescription(gLocalization->localize((*it)->getDescription(), gCurrentUserSession->getSessionLocale()));
            cr->setRecordStatType(type);
        }

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[GetRecordbookCommand] Database error (" << ErrorHelp::getErrorName(error) << ")");
        }

        return commandErrorFromBlazeError(error);
    }

};
// static factory method impl
DEFINE_GETCLUBRECORDBOOK_CREATE()

} // Clubs
} // Blaze
