/*************************************************************************************************/
/*!
    \file   osdkseasonalplaydb.h

    $Header: //gosdev/games/FIFA/2022/GenX/loadtest/blazeserver/customcomponent/osdkseasonalplay/osdkseasonalplaydb.h#1 $
    $Change: 1652815 $
    $DateTime: 2021/03/02 02:27:50 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class OSDKFeedbackDb

    Central implementation of db operations for OSDK Seasonal Play.

    \note

*/
/*************************************************************************************************/

#ifndef BLAZE_OSDKSEASONALPLAY_DB_H
#define BLAZE_OSDKSEASONALPLAY_DB_H

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/database/dbconn.h"

// osdkfeedback includes
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "framework/database/preparedstatement.h"

namespace Blaze
{
    class DbConn;
    class DbResult;
    class Query;
    class PreparedStatement;

    namespace OSDKSeasonalPlay
    {
        class OSDKSeasonalPlayMaster;
        class OSDKSeasonalPlaySlaveImpl;

        class OSDKSeasonalPlayDb
        {
        public:
            OSDKSeasonalPlayDb();
            explicit OSDKSeasonalPlayDb(uint32_t dbId);
            
            virtual ~OSDKSeasonalPlayDb();

            static void initialize(uint32_t dbId);
            void setDbConnection(DbConnPtr dbConn);

            DbConnPtr& getDbConnPtr() { return mDbConn; }
            BlazeRpcError getBlazeRpcError() { return mStatementError; }

        public:
            // called by RPC commands
            // any DB command resets the previous BlazeRpcError.
            BlazeRpcError fetchAwardsByMember(MemberId memberId, MemberType memberType, SeasonId seasonId, uint32_t seasonNumber, AwardList &outAwardList);
            BlazeRpcError fetchAwardsBySeason(SeasonId seasonId, uint32_t seasonNumber, AwardHistoricalFilter historicalFilter, AwardList &outAwardList);
            BlazeRpcError fetchSeasonId(MemberId memberId, MemberType memberType, SeasonId &outSeasonId);
            BlazeRpcError fetchSeasonMembers(SeasonId seasonId, eastl::vector<MemberId> &outMemberList);
            BlazeRpcError fetchSeasonNumber(SeasonId seasonId, uint32_t &outSeasonNumber, int32_t &outLastRolloverStatPeriodId);
            BlazeRpcError fetchTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus &outStatus);
            BlazeRpcError insertAward(AwardId awardId, MemberId memberId, MemberType memberType, SeasonId seasonId, LeagueId leagueId, uint32_t seasonNumber, bool8_t historical, const char8_t *metadata);
            BlazeRpcError insertNewSeason(SeasonId seasonId);
            BlazeRpcError insertRegistration(MemberId memberId, MemberType memberType, SeasonId seasonId, LeagueId leagueId);
            BlazeRpcError updateRegistration(MemberId memberId, MemberType memberType, SeasonId newSeasonId, LeagueId newLeagueId);
            BlazeRpcError updateSeasonNumber(SeasonId seasonId, uint32_t seasonNumber, uint32_t lastStatPeriodId);
            BlazeRpcError updateMemberTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus status);
            BlazeRpcError updateSeasonTournamentStatus(SeasonId seasonId, TournamentStatus status);
            BlazeRpcError updateAwardMember(uint32_t assignmentId, MemberId memberId);
            BlazeRpcError deleteAward(uint32_t assignmentid);
            BlazeRpcError deleteRegistration(MemberId memberId, MemberType memberType);
            BlazeRpcError lockSeason(SeasonId seasonId);

        private:
            PreparedStatement* getStatementFromId(PreparedStatementId id);

            void executeStatementInTxn(PreparedStatement* statement, DbResultPtr& result);
            void executeStatement(PreparedStatement* statement, DbResultPtr& result);
            void executeStatement(PreparedStatementId statementId, DbResultPtr& result);

            DbConnPtr                 mDbConn;

            BlazeRpcError         mStatementError;

        private:
            // static data
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchAwardsMemberAllSeasons;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchAwardsMemberSeasonNumber;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchAwardsSeasonAllSeasons;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchAwardsSeasonSeasonNumber;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchSeasonMembers;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchSeasonNumber;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchSeasonRegistration;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdInsertAward;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdInsertNewSeason;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdInsertRegistration;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateRegistration;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateSeasonNumber;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateMemberTournamentStatus;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateSeasonTournamentStatus;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateAwardMember;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdDeleteAward;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdDeleteRegistration;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdLockSeason;            
        };

    } // OSDKSeasonalPlay
} // Blaze

#endif // BLAZE_OSDKSEASONALPLAY_DB_H
