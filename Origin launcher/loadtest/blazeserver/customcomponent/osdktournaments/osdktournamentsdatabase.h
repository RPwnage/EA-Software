/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_OSDKTOURNAMENTSDATABASE_H
#define BLAZE_OSDKTOURNAMENTSDATABASE_H

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#include "framework/callback.h"
#include "blazerpcerrors.h"
#include "framework/database/dbconn.h"
#include "framework/tdf/userdefines.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/database/preparedstatement.h"
#include "osdktournaments/tdf/osdktournamentstypes.h"
#include "osdktournaments/tdf/osdktournamentstypes_server.h"
#include "osdktournaments/osdktournamentmatchnode.h"
#include "osdktournaments/rpc/osdktournamentsslave.h"

#include "EASTL/hash_map.h"
#include "eathread/eathread_storage.h"

namespace Blaze
{

// forward declarations
class PreparedStatement;
class DbRow;

namespace OSDKTournaments
{

typedef eastl::hash_map<TournamentId, TournamentMatchNode*> TournamentNodeMap;

class OSDKTournamentsDatabase
{
public:
	
	OSDKTournamentsDatabase(DbConnPtr& dbConnection);
    virtual ~OSDKTournamentsDatabase() {}

    static void initialize(uint32_t dbId);
    bool setDbConnection(DbConnPtr& dbConn);
    bool startTransaction();
    bool completeTransaction(bool success);

    BlazeRpcError upsertTournament(TournamentData* data);
    BlazeRpcError getMemberCount(TournamentId tournId, uint32_t& memberCount);
    BlazeRpcError insertMember(TournamentId tournamentId, TournamentMemberId memberId, const char8_t* tournAttribute, TeamId team);
    BlazeRpcError removeMember(TournamentId tournamentId, TournamentMemberId memberId);
    BlazeRpcError getMemberStatus(TournamentMemberId memberId, TournamentMode mode, TournamentMemberData& data);
    BlazeRpcError getMemberStatus(TournamentMemberId memberId, TournamentMode mode, TournamentId tournamentId, TournamentMemberData& data);
    BlazeRpcError insertMatchNode(TournamentMatchNode* node, TournamentMemberId treeOwnerMemberId, uint32_t& newMatchId);
    BlazeRpcError getLastMatchIdForMembers(TournamentMemberId memberOneId, TournamentMemberId memberTwoId, uint32_t& memberOneLastMatch, uint32_t& memberTwoLastMatch);
    BlazeRpcError updateMemberStatus(TournamentMemberId memberId, TournamentId tournId, bool isActive, uint32_t level, uint32_t lastMatchId, TeamId team);
    BlazeRpcError updateMemberStatusByTournament(TournamentId tournamentId, bool isActive, uint32_t level, uint32_t lastMatchId, TeamId team);
    BlazeRpcError getTreeForMember(TournamentMemberId memberId, const TournamentMemberData* memberData, const TournamentData* tournData, uint32_t& lastMatchId, TournamentNodeMap& nodeMap);
    BlazeRpcError fetchTrophies(TournamentMemberId memberId, uint32_t maxTrophies, uint32_t locale, GetTrophiesResponse::TournamentTrophyDataList& trophyList);
    BlazeRpcError awardTrophy(TournamentMemberId memberId, TournamentId tournId);
    BlazeRpcError deleteTree(TournamentMemberId memberId, TournamentId tournId);
    BlazeRpcError addSubtree(TournamentMemberId treeOwnerMemberId, uint32_t rootMatchId, uint32_t& newMatchId, TournamentNodeMap* nodeMap, uint32_t maxDepth, uint32_t& depth);
    BlazeRpcError deleteTournamentTrees(TournamentId tournamentId);

private:
    void numberTreeNodes(uint32_t matchId, uint32_t nodeNum, TournamentNodeMap* nodeMap) const;

    Blaze::DbConnPtr mDbConn;
    bool mInTransaction;

    static EA_THREAD_LOCAL PreparedStatementId mCmdInsertTournament;
    static EA_THREAD_LOCAL PreparedStatementId mCmdInsertMember;
    static EA_THREAD_LOCAL PreparedStatementId mCmdRemoveMember;
    static EA_THREAD_LOCAL PreparedStatementId mCmdGetTournForMember;
    static EA_THREAD_LOCAL PreparedStatementId mCmdGetTournForMemberWithId;
    static EA_THREAD_LOCAL PreparedStatementId mCmdGetTournMode;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateMemberStatus;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpdateMemberStatusByTournament;
    static EA_THREAD_LOCAL PreparedStatementId mCmdInsertMatch;
    static EA_THREAD_LOCAL PreparedStatementId mCmdGetMatches;
    static EA_THREAD_LOCAL PreparedStatementId mCmdUpsertTrophy;
    static EA_THREAD_LOCAL PreparedStatementId mCmdDeleteTree;
    static EA_THREAD_LOCAL PreparedStatementId mCmdDeleteTournamentTrees;

}; // OSDKTournamentsDatabase

} // OSDKTournaments
} // Blaze

#endif //BLAZE_OSDKTOURNAMENTSDATABASE_H
