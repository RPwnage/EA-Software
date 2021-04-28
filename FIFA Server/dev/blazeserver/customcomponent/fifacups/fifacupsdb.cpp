/*************************************************************************************************/
/*!
    \file   fifacupsdb.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/fifacupsdb.cpp#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaCupsDb

    Central implementation of db operations for OSDK Seasonal Play.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "fifacups/fifacupsdb.h"
#include "fifacups/fifacupsqueries.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"

#include "framework/database/dbrow.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// fifacups includes
#include "fifacupsslaveimpl.h"
#include "fifacups/tdf/fifacupstypes.h"

namespace Blaze
{
    namespace FifaCups
    {

        // Statics
        EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdFetchSeasonMembers              = 0;
        EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdFetchTournamentMembers          = 0;
		EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdFetchSeasonNumber               = 0;
        EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdFetchSeasonRegistration         = 0;
        EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdInsertNewSeason                 = 0;
        EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdInsertRegistration              = 0;
        EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdUpdateRegistration              = 0;
        EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdUpdateSeasonNumber              = 0;
        EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdUpdateMemberTournamentStatus    = 0;
		EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdUpdateMemberQualifyingDiv	    = 0;
		EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdUpdateMemberPendingDiv		    = 0;
		EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdUpdateMemberActiveCupId			= 0;
		EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdUpdateSeasonTournamentStatus    = 0;
		EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdUpdateSeasonActiveCupId		    = 0;
		EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdUpdateCopyPendingDivision	    = 0;
		EA_THREAD_LOCAL PreparedStatementId FifaCupsDb::mCmdUpdateClearPendingDivision      = 0;


        /*** Public Methods ******************************************************************************/

        /*************************************************************************************************/
        /*!
            \brief constructor

            Creates a db handler from the slave component.
        */
        /*************************************************************************************************/
        FifaCupsDb::FifaCupsDb(FifaCupsSlaveImpl* component) : 
            mDbConn(),
            mStatementError(ERR_OK)
        {
            if (NULL != component)
            {
                mComponentMaster = component->getMaster();
            }
            else
            {
                ASSERT(false && "[FifaCupsDb] - unable to determine master instance from slave!");
            }
        }

        /*************************************************************************************************/
        /*!
            \brief constructor

            Creates a db handler from the master component.
        */
        /*************************************************************************************************/
        FifaCupsDb::FifaCupsDb() : 
            mComponentMaster(NULL), 
            mDbConn(),
            mStatementError(ERR_OK)
        {
        }

        /*************************************************************************************************/
        /*!
            \brief destructor

            Destroys a db handler.
        */
        /*************************************************************************************************/
        FifaCupsDb::~FifaCupsDb()
        {
        }

        /*************************************************************************************************/
        /*!
            \brief initialize

            Registers prepared statements for the specified dbId.
        */
        /*************************************************************************************************/
        void FifaCupsDb::initialize(uint32_t dbId)
        {
            INFO_LOG("[FifaCupsDb].initialize() - dbId = "<<dbId <<" ");

            // Fetch
            mCmdFetchSeasonMembers = gDbScheduler->registerPreparedStatement(dbId,
                "fifacups_fetch_season_members", FIFACUPS_FETCH_SEASON_MEMBERS);
			mCmdFetchTournamentMembers = gDbScheduler->registerPreparedStatement(dbId,
				"fifacups_fetch_tournament_members", FIFACUPS_FETCH_TOURNAMENT_MEMBERS);
            mCmdFetchSeasonRegistration = gDbScheduler->registerPreparedStatement(dbId,
                "fifacups_fetch_registration", FIFACUPS_FETCH_REGISTRATION);
            mCmdFetchSeasonNumber = gDbScheduler->registerPreparedStatement(dbId,
                "fifacups_fetch_season_number", FIFACUPS_FETCH_SEASON_NUMBER);

            // Insert
            mCmdInsertNewSeason = gDbScheduler->registerPreparedStatement(dbId,
                "fifacups_insert_new_season", FIFACUPS_INSERT_NEW_SEASON);
            mCmdInsertRegistration = gDbScheduler->registerPreparedStatement(dbId,
                "fifacups_insert_registration", FIFACUPS_INSERT_REGISTRATION);

            // Update
            mCmdUpdateRegistration = gDbScheduler->registerPreparedStatement(dbId,
                "fifacups_update_registration", OSDSEASONALPLAY_UPDATE_REGISTRATION);
            mCmdUpdateSeasonNumber = gDbScheduler->registerPreparedStatement(dbId,
                "fifacups_update_season_number", FIFACUPS_UPDATE_SEASON_NUMBER);
            mCmdUpdateMemberTournamentStatus = gDbScheduler->registerPreparedStatement(dbId,
                "fifacups_update_member_tournament_status", FIFACUPS_UPDATE_MEMBER_TOURNAMENT_STATUS);
			mCmdUpdateMemberQualifyingDiv = gDbScheduler->registerPreparedStatement(dbId,
				"fifacups_update_member_qualifying_div", FIFACUPS_UPDATE_MEMBER_QUALIFYING_DIV);
			mCmdUpdateMemberPendingDiv = gDbScheduler->registerPreparedStatement(dbId,
				"fifacups_update_member_pending_div", FIFACUPS_UPDATE_MEMBER_PENDING_DIV);
			mCmdUpdateMemberActiveCupId = gDbScheduler->registerPreparedStatement(dbId,
				"fifacups_update_member_active_cupid", FIFACUPS_UPDATE_MEMBER_ACTIVE_CUPID);
            mCmdUpdateSeasonTournamentStatus = gDbScheduler->registerPreparedStatement(dbId,
                "fifacups_update_season_tournament_status", FIFACUPS_UPDATE_SEASON_TOURNAMENT_STATUS);
			mCmdUpdateSeasonActiveCupId = gDbScheduler->registerPreparedStatement(dbId,
				"fifacups_update_season_active_cupid", FIFACUPS_UPDATE_SEASON_ACTIVE_CUPID);
			mCmdUpdateCopyPendingDivision = gDbScheduler->registerPreparedStatement(dbId,
				"fifacups_update_copy_pending_division", FIFACUPS_UPDATE_COPY_PENDING_DIVISION);
			mCmdUpdateClearPendingDivision = gDbScheduler->registerPreparedStatement(dbId,
				"fifacups_update_clear_pending_division", FIFACUPS_CLEAR_PENDING_DIVISION);

            // Delete
       }
        
        /*************************************************************************************************/
        /*!
            \brief setDbConnection

            Sets the database connection to a new instance.
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::setDbConnection(DbConnPtr dbConn)
        { 
			BlazeRpcError error = ERR_OK;
            if (dbConn == NULL) 
            {
				TRACE_LOG("[FifaCupsDb].setDbConnection() - NULL dbConn passed in");
				error = FIFACUPS_ERR_DB;
            }
            else
            {
                mDbConn = dbConn;
            }

			return error;
        }


        /*************************************************************************************************/
        /*!
            \brief fetchSeasonId

            Retrieves the season id that the entity is registered in
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::fetchSeasonId(MemberId memberId, Blaze::FifaCups::MemberType memberType, Blaze::FifaCups::SeasonId &outSeasonId)
        {
            PreparedStatement *statement = getStatementFromId(mCmdFetchSeasonRegistration);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt64(0, memberId);
                statement->setInt32(1, memberType);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult); 
                if (ERR_OK == mStatementError && dbResult != NULL && !dbResult->empty())
                {
                    DbRow* thisRow = *(dbResult->begin());
                    outSeasonId = thisRow->getUInt("seasonId");
                }
                else
                {
                    TRACE_LOG("[FifaCupsDb].fetchSeasonId() - unable to find a registration for member "<<memberId <<" of type "<<MemberTypeToString(memberType) <<" ");
                    mStatementError = FIFACUPS_ERR_NOT_REGISTERED;
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief fetchRegistrationDetails

            Retrieves the season id and qualifying division the entity is registered in
        */
        /*************************************************************************************************/
		BlazeRpcError FifaCupsDb::fetchRegistrationDetails(MemberId memberId, MemberType memberType, SeasonId &outSeasonId, uint32_t &outQualifyingDiv, uint32_t &outPendingDiv, uint32_t &outActiveCupId, TournamentStatus &outTournamentStatus)
		{
			PreparedStatement *statement = getStatementFromId(mCmdFetchSeasonRegistration);
			if (NULL != statement && ERR_OK == mStatementError)
			{
				statement->setInt64(0, memberId);
				statement->setInt32(1, memberType);

				DbResultPtr dbResult;
				executeStatement(statement, dbResult); 
				if (ERR_OK == mStatementError && dbResult != NULL && !dbResult->empty())
				{
					DbRow* thisRow = *(dbResult->begin());
					outSeasonId = thisRow->getUInt("seasonId");
					outQualifyingDiv = thisRow->getUInt("qualifyingDiv");
					outPendingDiv = thisRow->getUInt("pendingDiv");
					outActiveCupId = thisRow->getUInt("activeCupId");
					outTournamentStatus = static_cast<TournamentStatus>(thisRow->getUInt("tournamentStatus"));
				}
				else
				{
					TRACE_LOG("[FifaCupsDb].fetchSeasonId() - unable to find a registration for member "<<memberId <<" of type "<<MemberTypeToString(memberType) <<" ");
					mStatementError = FIFACUPS_ERR_NOT_REGISTERED;
				}
			}

			return mStatementError;
		}

        /*************************************************************************************************/
        /*!
            \brief fetchSeasonMembers

            Retrieves all members registered in the season
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::fetchSeasonMembers(SeasonId seasonId, eastl::vector<MemberId> &outMemberList)
        {
            PreparedStatement *statement = getStatementFromId(mCmdFetchSeasonMembers);

            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, seasonId);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError && dbResult != NULL && !dbResult->empty())
                {
                    DbResult::const_iterator itr = dbResult->begin();
                    DbResult::const_iterator itr_end = dbResult->end();
                    for ( ; itr != itr_end; ++itr)
                    {
                        DbRow* thisRow = (*itr);

                        outMemberList.push_back(thisRow->getInt64("memberId"));
                    }
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief fetchTournamentMembers

            Retrieves all members registered in the season
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::fetchTournamentMembers(SeasonId seasonId, TournamentStatus tournamentStatus, eastl::vector<MemberInfo> &outMemberList)
        {
            PreparedStatement *statement = getStatementFromId(mCmdFetchTournamentMembers);

            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, seasonId);
				statement->setInt32(1, tournamentStatus);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError && dbResult != NULL && !dbResult->empty())
                {
                    DbResult::const_iterator itr = dbResult->begin();
                    DbResult::const_iterator itr_end = dbResult->end();
                    for ( ; itr != itr_end; ++itr)
                    {
                        DbRow* thisRow = (*itr);

						MemberInfo info;

						info.mMemberId = thisRow->getInt64("memberId");
						info.mMemberType = static_cast<MemberType>(thisRow->getInt("memberType"));
						info.mQualifyingDiv = thisRow->getUInt("qualifyingDiv");

                        outMemberList.push_back(info);
                    }
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief fetchSeasonNumber

            Retrieve the season number for the current season
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::fetchSeasonNumber(SeasonId seasonId, uint32_t &outSeasonNumber, uint32_t &outLastRolloverStatPeriodId)
        {
            PreparedStatement *statement = getStatementFromId(mCmdFetchSeasonNumber);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, seasonId);    

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError && dbResult != NULL && !dbResult->empty())
                {
                    DbRow* thisRow = *(dbResult->begin());
                    outSeasonNumber = thisRow->getUInt("seasonNumber");
                    outLastRolloverStatPeriodId = thisRow->getUInt("lastRollOverStatPeriodId");
                }
                else
                {
                    TRACE_LOG("[FifaCupsDb].fetchSeasonNumber() - unable to find season with id "<<seasonId<<" ");
                    mStatementError = FIFACUPS_ERR_SEASON_NOT_FOUND;
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief fetchTournamentStatus

            Retrieves the tournament status that the entity is registered in
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::fetchTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus &outStatus)
        {
            PreparedStatement *statement = getStatementFromId(mCmdFetchSeasonRegistration);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt64(0, memberId);
                statement->setInt32(1, memberType);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult); 
                if (ERR_OK == mStatementError && dbResult != NULL && !dbResult->empty())
                {
                    DbRow* thisRow = *(dbResult->begin());
                    outStatus = static_cast<TournamentStatus>(thisRow->getUInt("tournamentStatus"));
                }
                else
                {
                    TRACE_LOG("[FifaCupsDb].fetchTournamentStatus() - unable to find a registration for member "<<memberId<<" of type "<<MemberTypeToString(memberType) <<" ");
                    mStatementError = FIFACUPS_ERR_NOT_REGISTERED;
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief fetchQualifyingDiv

            Retrieves the qualifying division of the entity
        */
        /*************************************************************************************************/
		BlazeRpcError FifaCupsDb::fetchQualifyingDiv(MemberId memberId, MemberType memberType, uint32_t &outQualifyingDiv)

        {
            PreparedStatement *statement = getStatementFromId(mCmdFetchSeasonRegistration);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt64(0, memberId);
                statement->setInt32(1, memberType);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult); 
                if (ERR_OK == mStatementError && dbResult != NULL && !dbResult->empty())
                {
                    DbRow* thisRow = *(dbResult->begin());
                    outQualifyingDiv = thisRow->getInt("qualifyingDiv");
                }
                else
                {
                    TRACE_LOG("[FifaCupsDb].fetchQualifyingDiv() - unable to find a registration for member "<<memberId<<" of type "<<MemberTypeToString(memberType) <<" ");
                    mStatementError = FIFACUPS_ERR_NOT_REGISTERED;
                }
            }

            return mStatementError;
        }


        /*************************************************************************************************/
        /*!
            \brief insertNewSeason

            Inserts a new season into the seasons table
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::insertNewSeason(SeasonId seasonId)
        {
            PreparedStatement *statement = getStatementFromId(mCmdInsertNewSeason);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, seasonId);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief insertRegistration

            Inserts a new season registration into the registration table
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::insertRegistration(MemberId memberId, MemberType memberType, SeasonId seasonId, LeagueId leagueId, uint32_t qualifyingDiv)
        {
            PreparedStatement *statement = getStatementFromId(mCmdInsertRegistration);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt64(0, memberId);
                statement->setInt32(1, memberType);
                statement->setInt32(2, seasonId);
                statement->setInt32(3, leagueId);
				statement->setInt32(4, qualifyingDiv);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);

                // TODO - how to reinterpret an error in the case that the entity is already registered?
            }

            return mStatementError;
        }


        /*************************************************************************************************/
        /*!
            \brief updateRegistration

            updates an existing entry in the registration table with a new season id and league id
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::updateRegistration(MemberId memberId, MemberType memberType, SeasonId newSeasonId, LeagueId newLeagueId)
        {
            PreparedStatement *statement = getStatementFromId(mCmdUpdateRegistration);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, newSeasonId);
                statement->setInt32(1, newLeagueId);
                statement->setInt64(2, memberId);
                statement->setInt32(3, memberType);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve only one row.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() != 1)
                    {
                        // registration not found
                        ERR_LOG("[FifaCupsDb].fetchSeasonNumber() - unable to update registration for member "<<memberId<<" of type "<<MemberTypeToString(memberType) <<" ");
                        mStatementError = FIFACUPS_ERR_INVALID_PARAMS;
                    }
                }
            }

            return mStatementError;
        }


        /*************************************************************************************************/
        /*!
            \brief updateSeasonNumber

            update the season number and last stat period id of a season
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::updateSeasonNumber(SeasonId seasonId, uint32_t seasonNumber, uint32_t lastStatPeriodId)
        {
            PreparedStatement *statement = getStatementFromId(mCmdUpdateSeasonNumber);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, seasonNumber);
                statement->setInt32(1, lastStatPeriodId);
                statement->setInt32(2, seasonId);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve only one row.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() != 1)
                    {
                        // season not found
                        ERR_LOG("[FifaCupsDb].updateSeasonNumber() - unable to update season number for season "<<seasonId<<" ");
                        mStatementError = FIFACUPS_ERR_DB;
                    }
                }
            }

            return mStatementError;
        }


        /*************************************************************************************************/
        /*!
            \brief updateMemberTournamentStatus

            updates the tournament status for a member
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::updateMemberTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus status)
        {
            PreparedStatement *statement = getStatementFromId(mCmdUpdateMemberTournamentStatus);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, status);
                statement->setInt64(1, memberId);
                statement->setInt32(2, memberType);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve only one row.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() != 1)
                    {
                        // registration not found
                        ERR_LOG("[FifaCupsDb].updateMemberTournamentStatus() - unable to update tournament status for member "<<memberId<<" of type "<<MemberTypeToString(memberType) <<" ");
                        mStatementError = FIFACUPS_ERR_INVALID_PARAMS;
                    }
                }
            }

            return mStatementError;
        }


        /*************************************************************************************************/
        /*!
            \brief updateMemberQualifyingDiv

            updates the qualifying division for a member
        */
        /*************************************************************************************************/
		BlazeRpcError FifaCupsDb::updateMemberQualifyingDiv(MemberId memberId, MemberType memberType, uint32_t qualifyingDiv)

        {
            PreparedStatement *statement = getStatementFromId(mCmdUpdateMemberQualifyingDiv);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, qualifyingDiv);
                statement->setInt64(1, memberId);
                statement->setInt32(2, memberType);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve only one row.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() != 1)
                    {
                        // registration not found
                        ERR_LOG("[FifaCupsDb].updateMemberQualifyingDiv() - unable to update qualifying div for member "<<memberId<<" of type "<<MemberTypeToString(memberType) <<" ");      
                        mStatementError = FIFACUPS_ERR_INVALID_PARAMS;
                    }
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief updateMemberPendingDiv

            updates the pending division for a member
        */
        /*************************************************************************************************/
		BlazeRpcError FifaCupsDb::updateMemberPendingDiv(MemberId memberId, MemberType memberType, int32_t pendingDiv)
        {
            PreparedStatement *statement = getStatementFromId(mCmdUpdateMemberPendingDiv);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, pendingDiv);
                statement->setInt64(1, memberId);
                statement->setInt32(2, memberType);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve only one row.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() != 1)
                    {
                        // registration not found
                        ERR_LOG("[FifaCupsDb].updateMemberPendingDiv() - unable to update pending div for member "<<memberId<<" of type "<<MemberTypeToString(memberType) <<" ");
                        mStatementError = FIFACUPS_ERR_INVALID_PARAMS;
                    }
                }
            }

            return mStatementError;
        }

		/*************************************************************************************************/
        /*!
            \brief updateMemberActiveCupId

            updates the pending division for a member
        */
        /*************************************************************************************************/
		BlazeRpcError FifaCupsDb::updateMemberActiveCupId(MemberId memberId, MemberType memberType, int32_t activeCupId)
        {
            PreparedStatement *statement = getStatementFromId(mCmdUpdateMemberActiveCupId);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, activeCupId);
                statement->setInt64(1, memberId);
                statement->setInt32(2, memberType);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve only one row.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() != 1)
                    {
						if (dbResult == NULL)
						{
							ERR_LOG("[FifaCupsDb].updateMemberActiveCupId() - dbResult is NULL");
						}
						else if (dbResult->getAffectedRowCount() != 1)
						{
							ERR_LOG("[FifaCupsDb].updateMemberActiveCupId() - more or less than one row was affected " << dbResult->getAffectedRowCount());
						}
                        // registration not found
                        ERR_LOG("[FifaCupsDb].updateMemberActiveCupId() - unable to update active cup id for member "<<memberId<<" of type "<<MemberTypeToString(memberType) <<" ");
                        mStatementError = FIFACUPS_ERR_INVALID_PARAMS;
                    }
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief updateSeasonTournamentStatus

            updates the tournament status for all members of a season
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::updateSeasonTournamentStatus(SeasonId seasonId, TournamentStatus status)
        {
            PreparedStatement *statement = getStatementFromId(mCmdUpdateSeasonTournamentStatus);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, status);
                statement->setInt32(1, seasonId);

                DbResultPtr dbResult;
                executeStatementInTxn(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve one or more rows.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() == 0)
                    {
                        // season not found
                        ERR_LOG("[FifaCupsDb].updateSeasonTournamentStatus() - unable to update tournament status for season "<<seasonId <<" ");
                        mStatementError = FIFACUPS_ERR_INVALID_PARAMS;
                    }
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief updateSeasonsActiveCupId

            updates the tournament status for all members of a season
        */
        /*************************************************************************************************/
        BlazeRpcError FifaCupsDb::updateSeasonsActiveCupId(SeasonId seasonId, uint32_t cupId)
        {
            PreparedStatement *statement = getStatementFromId(mCmdUpdateSeasonActiveCupId);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, cupId);
                statement->setInt32(1, seasonId);

                DbResultPtr dbResult;
                executeStatementInTxn(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve one or more rows.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() == 0)
                    {
                        // season not found
                        ERR_LOG("[FifaCupsDb].updateSeasonsActiveCupId() - unable to update ActiveCupId for season "<<seasonId <<" ");
                        mStatementError = FIFACUPS_ERR_INVALID_PARAMS;
                    }
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief updateCopyPendingDiv

            updates the qualifying division with the pending divison for all members of a season
        */
        /*************************************************************************************************/
		BlazeRpcError FifaCupsDb::updateCopyPendingDiv(SeasonId seasonId)
		{
			PreparedStatement *statement = getStatementFromId(mCmdUpdateCopyPendingDivision);
			if (NULL != statement && ERR_OK == mStatementError)
			{
				statement->setInt32(0, seasonId);

				DbResultPtr dbResult;
				executeStatementInTxn(statement, dbResult);
				if (ERR_OK == mStatementError)
				{
					// a successful edit will involve one or more rows.
					if (dbResult == NULL || dbResult->getAffectedRowCount() == 0)
					{
						// season not found
						ERR_LOG("[FifaCupsDb].updateClearPendingDiv() - unable to clear pending division for season "<<seasonId<<" ");
						mStatementError = FIFACUPS_ERR_INVALID_PARAMS;
					}
				}
			}

			return mStatementError;
		}

        /*************************************************************************************************/
        /*!
            \brief updateSeasonTournamentStatus

			clears the pending division with the passed in value for all members of a season
        */
        /*************************************************************************************************/
		BlazeRpcError FifaCupsDb::updateClearPendingDiv(SeasonId seasonId, uint32_t clearedValue)
		{
            PreparedStatement *statement = getStatementFromId(mCmdUpdateClearPendingDivision);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, clearedValue);
                statement->setInt32(1, seasonId);

                DbResultPtr dbResult;
                executeStatementInTxn(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve one or more rows.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() == 0)
                    {
                        // season not found
                        ERR_LOG("[FifaCupsDb].updateClearPendingDiv() - unable to clear pending division for season "<< seasonId<<" ");
                        mStatementError = FIFACUPS_ERR_INVALID_PARAMS;
                    }
                }
            }

            return mStatementError;
		}

        /*************************************************************************************************/
        /*!
            \brief getStatementFromId

            Helper which returns the statement from the dbConn (if available).
        */
        /*************************************************************************************************/
        PreparedStatement* FifaCupsDb::getStatementFromId(PreparedStatementId id)
        {
            PreparedStatement* thisStatement = NULL;
            if (mDbConn != NULL)
            {
                thisStatement = mDbConn->getPreparedStatement(id);
                if (NULL == thisStatement)
                {
                    ASSERT(false && "[FifaCupsDb].getStatementFromId() no valid statement found");
                    mStatementError = FIFACUPS_ERR_DB;
                }
            }
            else
            {
                ASSERT(false && "[FifaCupsDb].getStatementFromId() no dbConn specified!");
                mStatementError = ERR_SYSTEM;
            }
            return thisStatement;
        }

        /*************************************************************************************************/
        /*!
            \brief executeStatement

            Helper which attempts to execute the prepared statement.
        */
        /*************************************************************************************************/
        void FifaCupsDb::executeStatement(PreparedStatement* statement, DbResultPtr& result)
        {
            mStatementError = ERR_OK;

            if (mDbConn != NULL)
            {
                if (NULL != statement)
                {
                    eastl::string queryDisplayBuf;
                    TRACE_LOG("[FifaCupsDb].executeStatement() trying statement "<<statement<<" @ dbConn "<<mDbConn->getName() <<":");
                    TRACE_LOG("\t"<<statement->toString(queryDisplayBuf) <<" ");

                    BlazeRpcError dbError = mDbConn->executePreparedStatement(statement, result);
                    switch (dbError)
                    {
                        case DB_ERR_OK:
                            break;
                        case DB_ERR_DISCONNECTED:
                        case DB_ERR_NO_SUCH_TABLE:
                        case DB_ERR_NOT_SUPPORTED:
                        case DB_ERR_INIT_FAILED:
                        case DB_ERR_TRANSACTION_NOT_COMPLETE:
                            ERR_LOG("[FifaCupsDb].executeStatement() a database error has occured");
                            mStatementError = FIFACUPS_ERR_DB;
                            break;
                        default:
                            ERR_LOG("[FifaCupsDb].executeStatement() a general error has occured");
                            mStatementError = FIFACUPS_ERR_GENERAL;
                            break;
                    }
                }
                else
                {
                    ASSERT(false && "[FifaCupsDb].executeStatement() - invalid prepared statement!");
                    mStatementError = FIFACUPS_ERR_INVALID_PARAMS;
                }
            }
            else
            {
                ERR_LOG("[FifaCupsDb].executeStatement() no database connection available");
                mStatementError = FIFACUPS_ERR_DB;
            }

            if (ERR_OK == mStatementError)
            {
                TRACE_LOG("[FifaCupsDb].executeStatement() statement "<< statement<<" @ dbConn "<<mDbConn->getName()<<" successful!");
            }
        }

        /*************************************************************************************************/
        /*!
            \brief executeStatement

            Helper which attempts to execute the prepared statement registered by the supplied id.
        */
        /*************************************************************************************************/
        void FifaCupsDb::executeStatement(PreparedStatementId statementId, DbResultPtr& result)
        {
            PreparedStatement* statement = getStatementFromId(statementId);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                executeStatement(statement, result);
            }
        }

        /*************************************************************************************************/
        /*!
            \brief executeStatementInTxn

            Helper which attempts to execute the prepared statement in a transaction
        */
        /*************************************************************************************************/
        void FifaCupsDb::executeStatementInTxn(PreparedStatement* statement, DbResultPtr& result)
        {
            if (mDbConn != NULL)
            {
                // start the database transaction
                BlazeRpcError dbError = mDbConn->startTxn();
                if (dbError == DB_ERR_OK)
                {
                    // execute the prepared statement
                    executeStatement(statement, result);

                    if (ERR_OK == mStatementError)
                    {
                        // try committing the transaction
                        dbError = mDbConn->commit();
                        if (dbError != DB_ERR_OK)
                        {
                            mDbConn->rollback();
                        }
                    }
                    else
                    {
                        // roll back if mStatementError doesn't indicate things are good
                        mDbConn->rollback();
                    }
                }
                else
                {
                    ERR_LOG("[FifaCupsDb].executeStatementInTxn() unable to start db transaction");
                }
            }
            else
            {
                ERR_LOG("[FifaCupsDb].executeStatementInTxn() no database connection available");
                mStatementError = FIFACUPS_ERR_DB;
            }
        }

    } // FifaCups
} // Blaze
