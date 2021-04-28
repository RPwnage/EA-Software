/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbresult.h"
#include "framework/database/dbrow.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/util/localization.h"

#include "osdktournaments/osdktournamentsdatabase.h"

namespace Blaze
{
namespace OSDKTournaments
{

static const char8_t TOURNDB_INSERT_TOURNAMENT[] = 
    "INSERT INTO `osdktournaments_tournaments` ("
    "`tournamentId`, `name`, `mode`, `description`, `trophyName`, `trophyMetaData`, `numRounds`"
    ") VALUES (?, ?, ?, ?, ?, ?, ?)"
    " ON DUPLICATE KEY UPDATE `mode` = VALUES(`mode`), `description` = VALUES(`description`), `trophyName` = VALUES(`trophyName`),"
    " `trophyMetaData` = VALUES(`trophyMetaData`), `numRounds` = VALUES(`numRounds`)";

static const char8_t TOURNDB_INSERT_MEMBER[] = 
    "INSERT INTO `osdktournaments_members` ("
    "`memberId`, `tournamentId`, `attribute`, `isActive`, `team`"
    ") VALUES (?, ?, ?, ?, ?)";

static const char8_t TOURNDB_REMOVE_MEMBER[] = 
    "DELETE FROM `osdktournaments_members`"
    "WHERE `tournamentId` = ? AND `memberId` = ? ";

static const char8_t TOURNDB_SELECT_MEMBERSHIP_STATUS[] =
    "SELECT members.`memberId`, tournaments.`mode`, tournaments.`tournamentId`, members.`isActive`, members.`lastMatchId`, members.`level`, members.`attribute`, members.`team` "
    "FROM `osdktournaments_members` members INNER JOIN `osdktournaments_tournaments` tournaments "
    "WHERE members.`tournamentId` = tournaments.`tournamentId` "
    "AND members.`memberId` = ? AND tournaments.`mode` = ?";

static const char8_t TOURNDB_SELECT_MEMBERSHIP_STATUS_WITH_ID[] =
    "SELECT members.`memberId`, tournaments.`mode`, tournaments.`tournamentId`, members.`isActive`, members.`lastMatchId`, members.`level`, members.`attribute`, members.`team` "
    "FROM `osdktournaments_members` members INNER JOIN `osdktournaments_tournaments` tournaments "
    "WHERE members.`tournamentId` = tournaments.`tournamentId` "
    "AND members.`memberId` = ? AND tournaments.`mode` = ? AND tournaments.`tournamentId` = ?";


static const char8_t TOURNDB_GET_MEMBER_COUNT[] = 
    "SELECT COUNT(1) as `memCount` FROM `osdktournaments_members` WHERE `tournamentId` = $u";

static const char8_t TOURNDB_UPDATE_MEMBER_STATUS[] = 
    "UPDATE `osdktournaments_members`"
    " SET `lastMatchId` = ?, `isActive` = ?, `level` = ?, `team` = ?"
    " WHERE `memberId` = ? AND `tournamentId` = ?";

static const char8_t TOURNDB_UPDATE_MEMBER_STATUS_BY_TOURN_ID[] =
    "UPDATE `osdktournaments_members`"
    " SET `lastMatchId` = ?, `isActive` = ?, `level` = ?, `team` = ?"
    " WHERE `tournamentId` = ?";

static const char8_t TOURNDB_INSERT_MATCH[] = 
    "INSERT INTO `osdktournaments_matches` ("
    "`tournamentId`, `memberOneId`, `memberTwoId`, `memberOneTeam`, `memberTwoTeam`, `memberOneScore`,"
    " `memberTwoScore`, `memberOneLastMatchId`, `memberTwoLastMatchId`, `memberOneAttribute`,"
    " `memberTwoAttribute`, `memberOneMetaData`, `memberTwoMetaData`, `treeOwnerMemberId`"
    ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

static const char8_t TOURNDB_GET_MATCHES[] = 
    "SELECT * FROM `osdktournaments_matches` WHERE `treeOwnerMemberId` = ?";

static const char8_t TOURNDB_GET_TROPHIES[] =
    "SELECT UNIX_TIMESTAMP(`timestamp`) as `awardTime`, troph.`tournamentId`, `awardCount`, `trophyName`, `trophyMetaData`"
    " FROM `osdktournaments_trophies` troph, `osdktournaments_tournaments` tourn"
    " WHERE `memberId` = $D AND troph.`tournamentId` = tourn.`tournamentId`"
    " ORDER BY `timestamp` DESC LIMIT $u";

static const char8_t TOURNDB_UPSERT_TROPHY[] = 
    "INSERT INTO `osdktournaments_trophies` SET"
    " `memberId` = ?, `tournamentId` = ?, `timestamp` = NOW()"
    " ON DUPLICATE KEY UPDATE"
    " `awardCount` = `awardCount` + 1, `timestamp` = NOW()";

static const char8_t TOURNDB_DELETE_TREE[] =
    "DELETE FROM `osdktournaments_matches` WHERE `treeOwnerMemberId` = ? AND `tournamentId` = ?";

static const char8_t TOURNDB_DELETE_TOURNAMENT_TREES[] =
    "DELETE FROM `osdktournaments_matches` WHERE `tournamentId` = ?";


EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdInsertTournament = 0;
EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdInsertMember = 0;
EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdRemoveMember = 0;
EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdGetTournForMember = 0;
EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdGetTournForMemberWithId = 0;
EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdUpdateMemberStatus = 0;
EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdUpdateMemberStatusByTournament = 0;
EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdInsertMatch = 0;
EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdGetMatches = 0;
EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdUpsertTrophy = 0;
EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdDeleteTree = 0;
EA_THREAD_LOCAL PreparedStatementId OSDKTournamentsDatabase::mCmdDeleteTournamentTrees = 0;

OSDKTournamentsDatabase::OSDKTournamentsDatabase(DbConnPtr& dbConnection) : 
    mDbConn(dbConnection),
    mInTransaction(false)
{ }

void OSDKTournamentsDatabase::initialize(uint32_t dbId)
{
    mCmdInsertTournament = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_tournaments_insert", TOURNDB_INSERT_TOURNAMENT);

    mCmdInsertMember = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_members_insert", TOURNDB_INSERT_MEMBER);

    mCmdRemoveMember = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_members_remove", TOURNDB_REMOVE_MEMBER);

    mCmdGetTournForMember = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_members_get_status", TOURNDB_SELECT_MEMBERSHIP_STATUS);

    mCmdGetTournForMemberWithId = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_members_get_status_with_id", TOURNDB_SELECT_MEMBERSHIP_STATUS_WITH_ID);

    mCmdUpdateMemberStatus = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_members_update_status", TOURNDB_UPDATE_MEMBER_STATUS);

    mCmdUpdateMemberStatusByTournament = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_members_update_status_by_tourn_id", TOURNDB_UPDATE_MEMBER_STATUS_BY_TOURN_ID);

    mCmdInsertMatch = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_matches_insert", TOURNDB_INSERT_MATCH);

    mCmdGetMatches = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_matches_select", TOURNDB_GET_MATCHES);

    mCmdUpsertTrophy = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_trophies_insert", TOURNDB_UPSERT_TROPHY);

    mCmdDeleteTree = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_matches_delete", TOURNDB_DELETE_TREE);

    mCmdDeleteTournamentTrees = gDbScheduler->registerPreparedStatement(dbId,
        "osdktournaments_matches_delete_trees", TOURNDB_DELETE_TOURNAMENT_TREES);
}

bool OSDKTournamentsDatabase::setDbConnection(DbConnPtr& dbConn) 
{
    if (true == mInTransaction)
    {
        TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].setDbConnection() - Can't set connection.  Currently in a transaction");
        return false;
    }
    mDbConn = dbConn;
    return true;
}

bool OSDKTournamentsDatabase::startTransaction()
{
    if (true == mInTransaction)
    {
        TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].startTransaction() - already in a transaction");
        return false;
    }

    BlazeRpcError dbError = mDbConn->startTxn();

    if (DB_ERR_OK != dbError)
    {
        TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].startTransaction() - Could not start transaction!");
        return false;
    }

    mInTransaction = true;
    return true;
}

bool OSDKTournamentsDatabase::completeTransaction(bool success)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].completeTransaction() - completing transaction on connection " 
              << (mDbConn != NULL ? mDbConn->getConnNum() : 0));

    bool result = false;

    // Only take action if we actually own the connection
    if (mDbConn != NULL)
    {
        if (true == mInTransaction)
        {
            // If we opened a transaction, we need to commit or rollback now depending on the success status
            TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].completeTransaction() - Completing transaction on connection "
                << mDbConn->getConnNum() << " with " << (success? "COMMIT" : "ROLLBACK"));

            BlazeRpcError dbError;

            if (true == success) 
            {    
                dbError = mDbConn->commit();
            }
            else
            {
                dbError = mDbConn->rollback();
            }

            if (DB_ERR_OK != dbError)
            {
                ERR_LOG("[OSDKTournamentsDatabase:" << this << "].completeTransaction() - database transaction was not successful");
            }
            else
            {
                result = true;
            }
        }
        mInTransaction = false;
    }

    return result;
}

BlazeRpcError OSDKTournamentsDatabase::upsertTournament(TournamentData* data)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].upsertTournament: name: " << data->getName());
    BlazeRpcError dbError = Blaze::DB_ERR_OK;

    if (mDbConn != NULL)
    {
        PreparedStatement* statement = mDbConn->getPreparedStatement(mCmdInsertTournament);

        if (NULL != statement)
        {
            uint32_t col = 0;

            statement->setInt32(col++, data->getTournamentId());
            statement->setString(col++, data->getName());
            statement->setInt32(col++, data->getMode());
            statement->setString(col++, data->getDescription());
            statement->setString(col++, data->getTrophyName());
            statement->setString(col++, data->getTrophyMetaData());
            statement->setInt32(col++, data->getNumRounds());

            DbResultPtr dbResult;
            dbError = mDbConn->executePreparedStatement(statement, dbResult);
            if (DB_ERR_OK == dbError)
            {

                //retrieve the member count
                uint32_t memberCount = 0;
                dbError = getMemberCount(data->getTournamentId(), memberCount);
                if (DB_ERR_OK == dbError)
                {
                    data->setTotalMemberCount(memberCount);
                }
            }
        }
    }
    else
    {
        dbError = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return dbError;
}

BlazeRpcError OSDKTournamentsDatabase::getMemberCount(TournamentId tournId, uint32_t& memberCount)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].getMemberCount: TournamentId: " << tournId);
    BlazeRpcError dbError = Blaze::DB_ERR_OK;

    if (mDbConn != NULL)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != NULL)
        {
            query->append(TOURNDB_GET_MEMBER_COUNT, static_cast<uint32_t>(tournId));
            DbResultPtr dbResult;
            dbError = mDbConn->executeQuery(query, dbResult);
            
            if (DB_ERR_OK == dbError)
            {
                if (false == dbResult->empty())
                {
                    DbResult::const_iterator it = dbResult->begin();
                    DbRow *dbRow = *it;
                    uint32_t column = 0;
                    memberCount = dbRow->getUInt(column);
                }
            }
        }
    }
    else
    {
        dbError = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return dbError;
}

BlazeRpcError OSDKTournamentsDatabase::insertMember(TournamentId tournamentId, TournamentMemberId memberId, const char8_t* tournAttribute, TeamId team)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].insertMember: Inserting member [blazeid]: " << memberId << " into tournament [id]: " << tournamentId);

    BlazeRpcError error = Blaze::DB_ERR_OK; 

    if (mDbConn != NULL)
    {
        DbResultPtr dbResult;
        PreparedStatement* statement = mDbConn->getPreparedStatement(mCmdInsertMember);

        if (NULL != statement)
        {
            uint32_t col = 0;

            statement->setInt64(col++, memberId);
            statement->setInt32(col++, tournamentId);
            statement->setString(col++, tournAttribute);
            statement->setInt32(col++, 1);
            statement->setInt32(col++, team);

            error = mDbConn->executePreparedStatement(statement, dbResult);
        }
    }
    else
    {
        error = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return error;
}

BlazeRpcError OSDKTournamentsDatabase::removeMember(TournamentId tournamentId, TournamentMemberId memberId)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].removeMember: Removing member [blazeid]: " << memberId << " from tournament [id]: " << tournamentId);

    BlazeRpcError error = Blaze::DB_ERR_OK; 

    if (mDbConn != NULL)
    {
        DbResultPtr dbResult;
        PreparedStatement* statement = mDbConn->getPreparedStatement(mCmdRemoveMember);

        if (NULL != statement)
        {
            uint32_t col = 0;

            statement->setInt32(col++, tournamentId);
            statement->setInt64(col++, memberId);
            error = mDbConn->executePreparedStatement(statement, dbResult);
        }
    }
    else
    {
        error = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return error;
}

BlazeRpcError OSDKTournamentsDatabase::getMemberStatus(TournamentMemberId memberId, TournamentMode mode, TournamentMemberData& data)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].getMemberStatus: Retrieving membership status for [memberId]: " << memberId <<
              "[mode]: " << mode << "");

    // redirect to the version that takes a tournament id, specifying the invalid tournament id.
    return getMemberStatus(memberId, mode, INVALID_TOURNAMENT_ID, data);
}

BlazeRpcError OSDKTournamentsDatabase::getMemberStatus(TournamentMemberId memberId, TournamentMode mode, TournamentId tournamentId, TournamentMemberData& data)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].getMemberStatus: Retrieving membership status for [memberId]: " << memberId <<
              "[mode]: " << mode << ", [tournId]: " << tournamentId << "");

    BlazeRpcError error = Blaze::DB_ERR_OK; 
	if (mode < 4)
	{
		return error;
	}

    if (mDbConn != NULL)
    {
        DbResultPtr dbResult;

        // if the tournament id is valid, use the "withId" query, otherwise use the query without the id
        PreparedStatement* statement = (INVALID_TOURNAMENT_ID != tournamentId) ?
            mDbConn->getPreparedStatement(mCmdGetTournForMemberWithId) : mDbConn->getPreparedStatement(mCmdGetTournForMember);

        if (NULL != statement)
        {
            statement->setInt64(0, memberId);
            statement->setInt32(1, mode);

            if (tournamentId != INVALID_TOURNAMENT_ID)
            {
                // set the tournament id for the query if the id is valid
                statement->setInt32(2, static_cast<int32_t>(tournamentId));
            }

            error = mDbConn->executePreparedStatement(statement, dbResult);
            if (DB_ERR_OK == error)
            {
                if (false == dbResult->empty())
                {
                    DbResult::const_iterator it = dbResult->begin();
                    DbRow *dbRow = *it;
                    data.setMemberId(memberId);
                    data.setTournamentId(static_cast<TournamentId>(dbRow->getUInt("tournamentId")));
                    data.setIsActive(dbRow->getInt("isActive") > 0 ? true : false);
                    data.setLevel(dbRow->getUInt("level"));
                    data.setLastMatchId(dbRow->getUInt("lastMatchId"));
                    data.setTournAttribute(dbRow->getString("attribute"));
                    data.setTeamId(dbRow->getInt("team"));
                }
            }
        }
    }
    else
    {
        error = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return error;
}

BlazeRpcError OSDKTournamentsDatabase::insertMatchNode(TournamentMatchNode* node, TournamentMemberId treeOwnerMemberId, uint32_t& newMatchId)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].insertMatchNode: memberOne: " << node->getMemberOneId() << ", memberTwo: " << node->getMemberTwoId());
    BlazeRpcError dbError = Blaze::DB_ERR_OK;

    if (mDbConn != NULL)
    {
        PreparedStatement* statement = mDbConn->getPreparedStatement(mCmdInsertMatch);

        if (NULL != statement)
        {
            uint32_t col = 0;

            statement->setInt32(col++, node->getTournamentId());
            statement->setInt64(col++, node->getMemberOneId());
            statement->setInt64(col++, node->getMemberTwoId());
            statement->setInt32(col++, node->getMemberOneTeam());
            statement->setInt32(col++, node->getMemberTwoTeam());
            statement->setInt32(col++, node->getMemberOneScore());
            statement->setInt32(col++, node->getMemberTwoScore());
            statement->setInt32(col++, node->getLeftNodeId());
            statement->setInt32(col++, node->getRightNodeId());
            statement->setString(col++, node->getMemberOneAttribute());
            statement->setString(col++, node->getMemberTwoAttribute());
            statement->setString(col++, node->getMemberOneMetaData());
            statement->setString(col++, node->getMemberTwoMetaData());
            statement->setInt64( col++, treeOwnerMemberId);

            DbResultPtr dbResult;
            dbError = mDbConn->executePreparedStatement(statement, dbResult);
            if (DB_ERR_OK == dbError)
            {
                if (dbResult->getLastInsertId() > 0)
                {
                    newMatchId = static_cast<uint32_t>(dbResult->getLastInsertId());
                }
            }
        }
    }
    else
    {
        dbError = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return dbError;
}

BlazeRpcError OSDKTournamentsDatabase::updateMemberStatus(TournamentMemberId memberId, TournamentId tournId, bool isActive, uint32_t level, uint32_t lastMatchId, TeamId team)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].updateMemberStatus: TournamentMemberId: " << memberId << ", isActive " << isActive 
              << ", level: " << level << ", lastMatch: " << lastMatchId);
    BlazeRpcError dbError = Blaze::DB_ERR_OK;

    if (mDbConn != NULL)
    {
        PreparedStatement* statement = mDbConn->getPreparedStatement(mCmdUpdateMemberStatus);

        if (NULL != statement)
        {
            uint32_t col = 0;

            statement->setInt32(col++, lastMatchId);
            statement->setInt32(col++, isActive ? 1 : 0);
            statement->setInt32(col++, level);
            statement->setInt32(col++, team);
            statement->setInt64(col++, memberId);
            statement->setInt32(col++, tournId);

            DbResultPtr dbResult;
            dbError = mDbConn->executePreparedStatement(statement, dbResult);
        }
    }
    else
    {
        dbError = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return dbError;
}

BlazeRpcError OSDKTournamentsDatabase::updateMemberStatusByTournament(TournamentId tournamentId, bool isActive, uint32_t level, uint32_t lastMatchId, TeamId team)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].updateMemberStatus: tournament " << tournamentId << ", isActive " << isActive 
              << ", level: " << level << ", lastMatch: " << lastMatchId);
    BlazeRpcError dbError = Blaze::DB_ERR_OK;

    if (mDbConn != NULL)
    {
        PreparedStatement* statement = mDbConn->getPreparedStatement(mCmdUpdateMemberStatusByTournament);

        if (NULL != statement)
        {
            uint32_t col = 0;

            statement->setInt32(col++, lastMatchId);
            statement->setInt32(col++, isActive ? 1 : 0);
            statement->setInt32(col++, level);
            statement->setInt32(col++, team);
            statement->setInt32(col++, tournamentId);

            DbResultPtr dbResult;
            dbError = mDbConn->executePreparedStatement(statement, dbResult);
        }
    }
    else
    {
        dbError = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return dbError;
}

BlazeRpcError OSDKTournamentsDatabase::getTreeForMember(TournamentMemberId memberId, const TournamentMemberData* memberData, const TournamentData* tournData, 
    uint32_t& lastMatchId, TournamentNodeMap& nodeMap)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].getTreeForMember: Member: " << memberId);

    BlazeRpcError error = Blaze::DB_ERR_OK; 

    if (mDbConn != NULL)
    {
        DbResultPtr dbResult;
        PreparedStatement* statement = mDbConn->getPreparedStatement(mCmdGetMatches);

        if (NULL != statement)
        {
            statement->setInt64(0, memberId);

            error = mDbConn->executePreparedStatement(statement, dbResult);
            if (DB_ERR_OK == error)
            {
                if (false == dbResult->empty())
                {
                    for (DbResult::const_iterator it = dbResult->begin(); it != dbResult->end(); it++)
                    {
                        DbRow *dbRow = *it;
                        //note: the calling function is responsible for freeing this memory
                        TournamentMatchNode* node = BLAZE_NEW_MGID(OSDKTournamentsSlave::COMPONENT_MEMORY_GROUP, "TournamentMatchNode") TournamentMatchNode();
                        lastMatchId = dbRow->getUInt("matchId");
                        node->setTournamentId(dbRow->getUInt("tournamentId"));
                        node->setMemberOneId(dbRow->getInt64("memberOneId"));
                        node->setMemberTwoId(dbRow->getInt64("memberTwoId"));
                        node->setMemberOneTeam(dbRow->getInt("memberOneTeam"));
                        node->setMemberTwoTeam(dbRow->getInt("memberTwoTeam"));
                        node->setMemberOneScore(dbRow->getUInt("memberOneScore"));
                        node->setMemberTwoScore(dbRow->getUInt("memberTwoScore"));
                        node->setLeftNodeId(dbRow->getUInt("memberOneLastMatchId"));
                        node->setRightNodeId(dbRow->getUInt("memberTwoLastMatchId"));
                        node->setMemberOneAttribute(dbRow->getString("memberOneAttribute"));
                        node->setMemberTwoAttribute(dbRow->getString("memberTwoAttribute"));
                        node->setMemberOneMetaData(dbRow->getString("memberOneMetaData"));
                        node->setMemberTwoMetaData(dbRow->getString("memberTwoMetaData"));

                        OSDKTournaments::OSDKTournamentsSlaveImpl* tournSlave = static_cast<OSDKTournaments::OSDKTournamentsSlaveImpl*>(gController->getComponent( OSDKTournaments::OSDKTournamentsSlaveImpl::COMPONENT_ID ));
                        eastl::string memberNameBuffer(MAX_PERSONA_LENGTH, '\0');
                        if (true == tournSlave->getMemberName(node->getMemberOneId(), tournData->getMode(), memberNameBuffer))
                        {
                            node->setMemberOneName(memberNameBuffer.c_str());
                        }

                        if (true == tournSlave->getMemberName(node->getMemberTwoId(), tournData->getMode(), memberNameBuffer))
                        {
                            node->setMemberTwoName(memberNameBuffer.c_str());
                        }

                        nodeMap.insert(eastl::make_pair(lastMatchId, node));
                    }

                    uint32_t rootNodeNumber = 1;
                    //if the member has not won the tournament, find out the node id of the last match they played.
                    if (memberData->getLevel() <= tournData->getNumRounds())
                    {
                        uint32_t depth = tournData->getNumRounds() - memberData->getLevel();
                        if (true == memberData->getIsActive())
                        {
                            depth++;
                        }

                        for (uint32_t i = 0; i < depth; ++i)
                        {
                            rootNodeNumber = (2 * rootNodeNumber) + 1;
                        }
                    }
                    numberTreeNodes(lastMatchId, rootNodeNumber, &nodeMap);
                }
            }
        }
    }
    else
    {
        error = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return error;
}

BlazeRpcError OSDKTournamentsDatabase::fetchTrophies(TournamentMemberId memberId, uint32_t maxTrophies, uint32_t locale, GetTrophiesResponse::TournamentTrophyDataList& trophyList)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].fetchTrophies: Retrieving trophies for member [blazeid]: " << memberId);

    BlazeRpcError error = Blaze::DB_ERR_OK; 

    if (mDbConn != NULL)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

        if (query != NULL)
        {
            query->append(TOURNDB_GET_TROPHIES, memberId, maxTrophies);
            DbResultPtr dbResult;
            error = mDbConn->executeQuery(query, dbResult);

            if (DB_ERR_OK == error)
            {
                if (false == dbResult->empty())
                {
                    for (DbResult::const_iterator it = dbResult->begin();
                        it != dbResult->end(); it++)
                    {
                        DbRow *dbRow = *it;
                        TournamentTrophyData* trophy = BLAZE_NEW_MGID(OSDKTournamentsSlave::COMPONENT_MEMORY_GROUP, "TournamentTrophyData") TournamentTrophyData();
                        trophy->setMemberId(memberId);
                        trophy->setTournamentId(static_cast<TournamentId>(dbRow->getUInt("tournamentId")));
                        trophy->setAwardCount(dbRow->getUInt("awardCount"));
                        trophy->setTrophyNameLocKey(dbRow->getString("trophyName"));
                        trophy->setTrophyMetaData(dbRow->getString("trophyMetaData"));
                        trophy->setTrophyName(gLocalization->localize(trophy->getTrophyNameLocKey(), locale));
                        trophy->setAwardTime(dbRow->getUInt("awardTime"));
                        trophyList.push_back(trophy);
                    }
                }
            }
        }
    }    
    else
    {
        error = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return error;
}

BlazeRpcError OSDKTournamentsDatabase::awardTrophy(TournamentMemberId memberId, TournamentId tournId)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].awardTrophy: Awarding trophy to member [blazeid]: " << memberId << " for tournament: " << tournId);

    BlazeRpcError error = Blaze::DB_ERR_OK; 

    if (mDbConn != NULL)
    {
        DbResultPtr dbResult;
        PreparedStatement* statement = mDbConn->getPreparedStatement(mCmdUpsertTrophy);

        if (NULL != statement)
        {
            uint32_t col = 0;
            statement->setInt64(col++, memberId);
            statement->setInt32(col++, tournId);

            error = mDbConn->executePreparedStatement(statement, dbResult);
        }
    }
    else
    {
        error = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return error;
}

BlazeRpcError OSDKTournamentsDatabase::deleteTree(TournamentMemberId memberId, TournamentId tournId)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].deleteTree: memberId: " << memberId << ", tournamentId: " << tournId << "");
    BlazeRpcError error = Blaze::DB_ERR_OK; 

    if (mDbConn != NULL)
    {
        DbResultPtr dbResult;
        PreparedStatement* statement = mDbConn->getPreparedStatement(mCmdDeleteTree);

        if (NULL != statement)
        {
            statement->setInt64(0, memberId);
            statement->setInt32(1, static_cast<int32_t>(tournId));
            error = mDbConn->executePreparedStatement(statement, dbResult);
        }
    }
    else
    {
        error = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return error;
}

BlazeRpcError OSDKTournamentsDatabase::addSubtree(TournamentMemberId treeOwnerMemberId, uint32_t rootMatchId, 
    uint32_t& newMatchId, TournamentNodeMap* nodeMap, uint32_t maxDepth, uint32_t& depth)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].addSubtree: memberId: " << treeOwnerMemberId);
    BlazeRpcError error = Blaze::DB_ERR_OK; 
    ++depth;

    if ((NULL != nodeMap) && (rootMatchId > 0) && (depth < maxDepth))
    {
        TournamentNodeMap::iterator itr = nodeMap->find(rootMatchId);
        if (itr != nodeMap->end())
        {
            TournamentMatchNode* node = itr->second;

            //recurse to insert the children first.  we need their Ids
            uint32_t leftMatchId = 0;
            addSubtree(treeOwnerMemberId, node->getLeftNodeId(), leftMatchId, nodeMap, maxDepth, depth);
            uint32_t rightMatchId = 0;
            addSubtree(treeOwnerMemberId, node->getRightNodeId(), rightMatchId, nodeMap, maxDepth, depth);

            node->setLeftNodeId(leftMatchId);
            node->setRightNodeId(rightMatchId);
            insertMatchNode(node, treeOwnerMemberId, newMatchId);
        }
    }
    else
    {
        newMatchId = 0;
    }
    --depth;
    return error;
}

BlazeRpcError OSDKTournamentsDatabase::deleteTournamentTrees(TournamentId tournamentId)
{
    TRACE_LOG("[OSDKTournamentsDatabase:" << this << "].deleteTournamentTrees: tournament: " << tournamentId);
    BlazeRpcError error = Blaze::DB_ERR_OK; 

    if (mDbConn != NULL)
    {
        PreparedStatement* statement = mDbConn->getPreparedStatement(mCmdDeleteTournamentTrees);

        if (NULL != statement)
        {
            statement->setInt32(0, tournamentId);
            DbResultPtr dbResult;
            error = mDbConn->executePreparedStatement(statement, dbResult);
        }
    }    
    else
    {
        error = Blaze::DB_ERR_NO_CONNECTION_AVAILABLE;
    }

    return error;
}

void OSDKTournamentsDatabase::numberTreeNodes(uint32_t matchId, uint32_t nodeNum, TournamentNodeMap* nodeMap) const
{
    if ((NULL != nodeMap) && (matchId > 0))
    {
        TournamentNodeMap::iterator itr = nodeMap->find(matchId);
        if (itr != nodeMap->end())
        {
            TournamentMatchNode* node = itr->second;
            node->setNodeId(nodeNum);

            numberTreeNodes(node->getLeftNodeId(), (2 * nodeNum) + 1, nodeMap);
            numberTreeNodes(node->getRightNodeId(), (2 * nodeNum), nodeMap);
        }
    }
}

} // Tournaments
} // Blaze
