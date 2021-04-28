/*************************************************************************************************/
/*!
    \file   fifacupsdb.h

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/fifacupsdb.h#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

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

#ifndef BLAZE_FIFACUPS_DB_H
#define BLAZE_FIFACUPS_DB_H

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

// osdkfeedback includes
#include "fifacups/tdf/fifacupstypes.h"
#include "framework/database/preparedstatement.h"

namespace Blaze
{
    class DbConn;
    class DbResult;
    class Query;
    class PreparedStatement;

    namespace FifaCups
    {
        class FifaCupsMaster;
        class FifaCupsSlaveImpl;

		struct MemberInfo
		{
			MemberId mMemberId;
			MemberType mMemberType;
			uint32_t mQualifyingDiv;
		};

        class FifaCupsDb
        {
        public:
            FifaCupsDb(FifaCupsSlaveImpl* component);
            FifaCupsDb();
            virtual ~FifaCupsDb();

            static void initialize(uint32_t dbId);
            BlazeRpcError setDbConnection(DbConnPtr dbConn);

            BlazeRpcError getBlazeRpcError() {return mStatementError;}
            void clearBlazeRpcError() {mStatementError = ERR_OK;}

        public:
            // called by RPC commands
            BlazeRpcError fetchSeasonId(MemberId memberId, MemberType memberType, SeasonId &outSeasonId);
			BlazeRpcError fetchRegistrationDetails(MemberId memberId, MemberType memberType, SeasonId &outSeasonId, uint32_t &outQualifyingDiv, uint32_t &outPendingDiv, uint32_t &activeCupId, TournamentStatus &outTournamentStatus);
            BlazeRpcError fetchSeasonMembers(SeasonId seasonId, eastl::vector<MemberId> &outMemberList);
			BlazeRpcError fetchTournamentMembers(SeasonId seasonId, TournamentStatus tournamentStatus, eastl::vector<MemberInfo> &outMemberList);
			BlazeRpcError fetchSeasonNumber(SeasonId seasonId, uint32_t &outSeasonNumber, uint32_t &outLastRolloverStatPeriodId);
            BlazeRpcError fetchTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus &outStatus);
			BlazeRpcError fetchQualifyingDiv(MemberId memberId, MemberType memberType, uint32_t &outQualifyingDiv);
            BlazeRpcError insertNewSeason(SeasonId seasonId);
            BlazeRpcError insertRegistration(MemberId memberId, MemberType memberType, SeasonId seasonId, LeagueId leagueId, uint32_t qualifyingDiv);
            BlazeRpcError updateRegistration(MemberId memberId, MemberType memberType, SeasonId newSeasonId, LeagueId newLeagueId);
            BlazeRpcError updateSeasonNumber(SeasonId seasonId, uint32_t seasonNumber, uint32_t lastStatPeriodId);
            BlazeRpcError updateMemberTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus status);
			BlazeRpcError updateMemberQualifyingDiv(MemberId memberId, MemberType memberType, uint32_t qualifyingDiv);
			BlazeRpcError updateMemberPendingDiv(MemberId memberId, MemberType memberType, int32_t pendingDiv);
            BlazeRpcError updateSeasonTournamentStatus(SeasonId seasonId, TournamentStatus status);
			BlazeRpcError updateSeasonsActiveCupId(SeasonId seasonId, uint32_t cupId);
			BlazeRpcError updateMemberActiveCupId(MemberId memberId, MemberType memberType, int32_t activeCupId);
			BlazeRpcError updateCopyPendingDiv(SeasonId seasonId);
			BlazeRpcError updateClearPendingDiv(SeasonId seasonId, uint32_t clearedValue);

        private:
            PreparedStatement* getStatementFromId(PreparedStatementId id);

            void executeStatementInTxn(PreparedStatement* statement, DbResultPtr& result);
            void executeStatement(PreparedStatement* statement, DbResultPtr& result);
            void executeStatement(PreparedStatementId statementId, DbResultPtr& result);

            FifaCupsMaster*   mComponentMaster;
            DbConnPtr                 mDbConn;

            BlazeRpcError         mStatementError;

        private:
            // static data
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchSeasonMembers;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchTournamentMembers;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchSeasonNumber;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchSeasonRegistration;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdInsertNewSeason;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdInsertRegistration;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateRegistration;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateSeasonNumber;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateMemberTournamentStatus;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateMemberQualifyingDiv;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateMemberPendingDiv;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateMemberActiveCupId;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateSeasonTournamentStatus;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateSeasonActiveCupId;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateCopyPendingDivision;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateClearPendingDivision;

            
        };

    } // FifaCups
} // Blaze

#endif // BLAZE_FIFACUPS_DB_H
