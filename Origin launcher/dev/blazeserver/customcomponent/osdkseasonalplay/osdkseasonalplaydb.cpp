/*************************************************************************************************/
/*!
    \file   osdkseasonalplaydb.cpp

    $Header: //gosdev/games/FIFA/2022/GenX/dev/blazeserver/customcomponent/osdkseasonalplay/osdkseasonalplaydb.cpp#1 $
    $Change: 1627358 $
    $DateTime: 2020/10/22 16:54:02 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class OSDKSeasonalPlayDb

    Central implementation of db operations for OSDK Seasonal Play.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdkseasonalplay/osdkseasonalplaydb.h"
#include "osdkseasonalplay/osdkseasonalplayqueries.h"

// global includes

// framework includes
#include "framework/connection/selector.h"

#include "framework/database/dbrow.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// osdkseasonalplay includes
#include "osdkseasonalplayslaveimpl.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"

namespace Blaze
{
    namespace OSDKSeasonalPlay
    {

        // Statics
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdFetchAwardsMemberAllSeasons     = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdFetchAwardsMemberSeasonNumber   = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdFetchAwardsSeasonAllSeasons     = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdFetchAwardsSeasonSeasonNumber   = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdFetchSeasonMembers              = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdFetchSeasonNumber               = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdFetchSeasonRegistration         = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdInsertAward                     = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdInsertNewSeason                 = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdInsertRegistration              = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdUpdateRegistration              = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdUpdateSeasonNumber              = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdUpdateMemberTournamentStatus    = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdUpdateSeasonTournamentStatus    = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdUpdateAwardMember               = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdDeleteAward                     = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdDeleteRegistration              = 0;
        EA_THREAD_LOCAL PreparedStatementId OSDKSeasonalPlayDb::mCmdLockSeason                      = 0;


        /*** Public Methods ******************************************************************************/

        /*************************************************************************************************/
        /*!
            \brief constructor

            Creates a db handler from the master component.
        */
        /*************************************************************************************************/
        OSDKSeasonalPlayDb::OSDKSeasonalPlayDb() :
            mDbConn(),
            mStatementError(ERR_OK)
        {
        }

        /*************************************************************************************************/
        /*!
            \brief constructor

            Creates a db handler with DB connection.
        */
        /*************************************************************************************************/
        OSDKSeasonalPlayDb::OSDKSeasonalPlayDb(uint32_t dbId) :
            mDbConn(),
            mStatementError(ERR_OK)
        {
            mDbConn = gDbScheduler->getConnPtr(dbId);
            if(mDbConn == NULL)
            {
                WARN_LOG("[OSDKSeasonalPlayDb:" << this << "].OSDKSeasonalPlayDb() failed to acquire a DB connection!");
                mStatementError = OSDKSEASONALPLAY_ERR_DB;
            }
        }

        /*************************************************************************************************/
        /*!
            \brief destructor

            Destroys a db handler.
        */
        /*************************************************************************************************/
        OSDKSeasonalPlayDb::~OSDKSeasonalPlayDb()
        {
        }

        /*************************************************************************************************/
        /*!
            \brief initialize

            Registers prepared statements for the specified dbId.
        */
        /*************************************************************************************************/
        void OSDKSeasonalPlayDb::initialize(uint32_t dbId)
        {
            INFO_LOG("[OSDKSeasonalPlayDb].initialize() - dbId = " << dbId);

            // Fetch
            mCmdFetchAwardsMemberAllSeasons = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_fetch_awards_member_allseasons", OSDKSEASONALPLAY_FETCH_AWARDS_MEMBER_ALLSEASONS);
            mCmdFetchAwardsMemberSeasonNumber = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_fetch_awards_member_seasonnumber", OSDKSEASONALPLAY_FETCH_AWARDS_MEMBER_SEASONNUMBER);
            mCmdFetchAwardsSeasonAllSeasons = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_fetch_awards_season_allseasons", OSDKSEASONALPLAY_FETCH_AWARDS_SEASON_ALLSEASONS);
            mCmdFetchSeasonMembers = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_fetch_season_members", OSDKSEASONALPLAY_FETCH_SEASON_MEMBERS);
            mCmdFetchAwardsSeasonSeasonNumber = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_fetch_awards_season_seasonnumber", OSDKSEASONALPLAY_FETCH_AWARDS_SEASON_SEASONNUMBER);
            mCmdFetchSeasonRegistration = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_fetch_registration", OSDKSEASONALPLAY_FETCH_REGISTRATION);
            mCmdFetchSeasonNumber = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_fetch_season_number", OSDKSEASONALPLAY_FETCH_SEASON_NUMBER);

            // Insert
            mCmdInsertAward = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_insert_award", OSDKSEASONALPLAY_INSERT_AWARD);
            mCmdInsertNewSeason = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_insert_new_season", OSDKSEASONALPLAY_INSERT_NEW_SEASON);
            mCmdInsertRegistration = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_insert_registration", OSDKSEASONALPLAY_INSERT_REGISTRATION);

            // Update
            mCmdUpdateRegistration = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_update_registration", OSDSEASONALPLAY_UPDATE_REGISTRATION);
            mCmdUpdateSeasonNumber = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_update_season_number", OSDKSEASONALPLAY_UPDATE_SEASON_NUMBER);
            mCmdUpdateMemberTournamentStatus = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_update_member_tournament_status", OSDKSEASONALPLAY_UPDATE_MEMBER_TOURNAMENT_STATUS);
            mCmdUpdateSeasonTournamentStatus = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_update_season_tournament_status", OSDKSEASONALPLAY_UPDATE_SEASON_TOURNAMENT_STATUS);
            mCmdUpdateAwardMember = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_update_award_member", OSDKSEASONALPLAY_UPDATE_AWARD_MEMBER);

            // Delete
            mCmdDeleteAward = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_delete_award", OSDKSEASONALPLAY_DELETE_AWARD);
            mCmdDeleteRegistration = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_delete_registration", OSDKSEASONALPLAY_DELETE_REGISTRATION);

            // Lock
            mCmdLockSeason = gDbScheduler->registerPreparedStatement(dbId,
                "osdkseasonalplay_lock_season", OSDKSEASONALPLAY_LOCK_SEASON);
       }
        
        /*************************************************************************************************/
        /*!
            \brief setDbConnection

            Sets the database connection to a new instance.
        */
        /*************************************************************************************************/
        void OSDKSeasonalPlayDb::setDbConnection(DbConnPtr dbConn)
        { 
            if (dbConn == NULL) 
            {
                WARN_LOG("[OSDKSeasonalPlayDb:" << this << "].setDbConnection() attempting to set connection to NULL dbConn!");
            }

            mDbConn = dbConn;
        }

        /*************************************************************************************************/
        /*!
            \brief fetchAwardsBySeason

            Retrieves the awards assigned in the specified season
        */
        /*************************************************************************************************/
        BlazeRpcError OSDKSeasonalPlayDb::fetchAwardsBySeason(SeasonId seasonId, uint32_t seasonNumber, AwardHistoricalFilter historicalFilter, AwardList &outAwardList)
        {
            PreparedStatement *statement = NULL;

            if (AWARD_ANY_SEASON_NUMBER == static_cast<int32_t>(seasonNumber))
            {
                // fetching all awards for a member regardless of season number
                statement = getStatementFromId(mCmdFetchAwardsSeasonAllSeasons);
            }
            else
            {
                // fetching the awards for a member with a specific season number
                statement = getStatementFromId(mCmdFetchAwardsSeasonSeasonNumber);
            }


            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, seasonId);
                if (AWARD_ANY_SEASON_NUMBER != static_cast<int32_t>(seasonNumber))
                {
                    statement->setInt32(1, seasonNumber);
                }

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError && dbResult != NULL && !dbResult->empty())
                {
                    // award historical value to filter against (default to true)
                    bool8_t bHistoricalValueToMatch = true;

                    if (SEASONALPLAY_AWARD_HISTORICAL_FALSE == historicalFilter)
                    {
                        bHistoricalValueToMatch = false;
                    }

                    DbResult::const_iterator itr = dbResult->begin();
                    DbResult::const_iterator itr_end = dbResult->end();
                    for ( ; itr != itr_end; ++itr)
                    {
                        DbRow* thisRow = (*itr);
                        
                        int32_t rowHistorical = thisRow->getUInt("historical");
                        bool8_t bRowHistoricalValue = (rowHistorical >= 1) ? true : false;

                        // only add the award to the output list if the historical filter allows it
                        if (SEASONALPLAY_AWARD_HISTORICAL_ANY == historicalFilter || bHistoricalValueToMatch == bRowHistoricalValue)
                        {
                            Award* thisAward = outAwardList.pull_back();

                            thisAward->setId(thisRow->getUInt("id"));
                            thisAward->setAwardId(thisRow->getUInt("awardId"));
                            thisAward->setMemberId(thisRow->getInt64("memberId"));
                            thisAward->setMemberType(static_cast<MemberType>(thisRow->getUInt("memberType")));
                            thisAward->setSeasonId(seasonId);
                            thisAward->setLeagueId(thisRow->getUInt("leagueId"));
                            thisAward->setSeasonNumber(thisRow->getUInt("seasonNumber"));
                            thisAward->setTimeStamp(thisRow->getInt64("UNIX_TIMESTAMP(`timestamp`)"));
                            thisAward->setHistorical(bRowHistoricalValue);
                            thisAward->setMetadata(thisRow->getString("metadata"));
                        }
                    }
                }
            }

            return mStatementError;
        }


        /*************************************************************************************************/
        /*!
        \brief fetchAwardsByMember

        Retrieves the awards assigned to the specified member for the the particular season. 
        */
        /*************************************************************************************************/
        BlazeRpcError OSDKSeasonalPlayDb::fetchAwardsByMember(MemberId memberId, MemberType memberType, SeasonId seasonId, uint32_t seasonNumber, AwardList &outAwardList)
        {
            PreparedStatement *statement = NULL;

            if (AWARD_ANY_SEASON_NUMBER == static_cast<int32_t>(seasonNumber))
            {
                // fetching all awards for a member regardless of season number
                statement = getStatementFromId(mCmdFetchAwardsMemberAllSeasons);
            }
            else
            {
                // fetching the awards for a member with a specific season number
                statement = getStatementFromId(mCmdFetchAwardsMemberSeasonNumber);
            }

            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt64(0, memberId);  
                statement->setInt32(1, memberType);
                statement->setInt32(2, seasonId);
                if (AWARD_ANY_SEASON_NUMBER != static_cast<int32_t>(seasonNumber))
                {
                    statement->setInt32(3, seasonNumber);
                }

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError && dbResult != NULL && !dbResult->empty())
                {
                    DbResult::const_iterator itr = dbResult->begin();
                    DbResult::const_iterator itr_end = dbResult->end();
                    for ( ; itr != itr_end; ++itr)
                    {
                        Award* thisAward = outAwardList.pull_back();
                        DbRow* thisRow = (*itr);

                        thisAward->setId(thisRow->getUInt("id"));
                        thisAward->setAwardId(thisRow->getUInt("awardId"));
                        thisAward->setMemberId(memberId);
                        thisAward->setMemberType(memberType);
                        thisAward->setSeasonId(seasonId);
                        thisAward->setLeagueId(thisRow->getUInt("leagueId"));
                        thisAward->setSeasonNumber(thisRow->getUInt("seasonNumber"));
                        thisAward->setTimeStamp(thisRow->getInt64("UNIX_TIMESTAMP(`timestamp`)"));

                        int32_t rowHistorical = thisRow->getUInt("historical");
                        bool8_t bRowHistoricalValue = (rowHistorical >= 1) ? true : false;

                        thisAward->setHistorical(bRowHistoricalValue);
                        thisAward->setMetadata(thisRow->getString("metadata"));

                    }
                }
            }

            return mStatementError;
        }



        /*************************************************************************************************/
        /*!
            \brief fetchSeasonId

            Retrieves the season id that the entity is registered in
        */
        /*************************************************************************************************/
        BlazeRpcError OSDKSeasonalPlayDb::fetchSeasonId(MemberId memberId, Blaze::OSDKSeasonalPlay::MemberType memberType, Blaze::OSDKSeasonalPlay::SeasonId &outSeasonId)
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
                    TRACE_LOG("[OSDKSeasonalPlayDb].fetchSeasonId() - unable to find a registration for member " << memberId << 
                              " of type " << MemberTypeToString(memberType) << "");
                    mStatementError = OSDKSEASONALPLAY_ERR_NOT_REGISTERED;
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
        BlazeRpcError OSDKSeasonalPlayDb::fetchSeasonMembers(SeasonId seasonId, eastl::vector<MemberId> &outMemberList)
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
            \brief fetchSeasonNumber

            Retrieve the season number for the current season
        */
        /*************************************************************************************************/
        BlazeRpcError OSDKSeasonalPlayDb::fetchSeasonNumber(SeasonId seasonId, uint32_t &outSeasonNumber, int32_t &outLastRolloverStatPeriodId)
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
                    outLastRolloverStatPeriodId = thisRow->getInt("lastRollOverStatPeriodId");
                }
                else
                {
                    TRACE_LOG("[OSDKSeasonalPlayDb].fetchSeasonNumber() - unable to find season with id " << seasonId << "");
                    mStatementError = OSDKSEASONALPLAY_ERR_SEASON_NOT_FOUND;
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
        BlazeRpcError OSDKSeasonalPlayDb::fetchTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus &outStatus)
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
                    TRACE_LOG("[OSDKSeasonalPlayDb].fetchTournamentStatus() - unable to find a registration for member " << memberId << 
                              " of type " << MemberTypeToString(memberType) << "");
                    mStatementError = OSDKSEASONALPLAY_ERR_NOT_REGISTERED;
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief insertAward

            Inserts a new award assignment into the awards table
        */
        /*************************************************************************************************/
        BlazeRpcError OSDKSeasonalPlayDb::insertAward(AwardId awardId, MemberId memberId, MemberType memberType, SeasonId seasonId, LeagueId leagueId, uint32_t seasonNumber, bool8_t historical, const char8_t *metadata)
        {
            PreparedStatement *statement = getStatementFromId(mCmdInsertAward);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, awardId);
                statement->setInt64(1, memberId);
                statement->setInt32(2, memberType);
                statement->setInt32(3, seasonId);
                statement->setInt32(4, leagueId);
                statement->setInt32(5, seasonNumber);
                statement->setInt32(6, static_cast<int32_t>(historical));
                statement->setString(7, metadata);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
            }

            return mStatementError;
        }


        /*************************************************************************************************/
        /*!
            \brief insertNewSeason

            Inserts a new season into the seasons table
        */
        /*************************************************************************************************/
        BlazeRpcError OSDKSeasonalPlayDb::insertNewSeason(SeasonId seasonId)
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
        BlazeRpcError OSDKSeasonalPlayDb::insertRegistration(MemberId memberId, MemberType memberType, SeasonId seasonId, LeagueId leagueId)
        {
            PreparedStatement *statement = getStatementFromId(mCmdInsertRegistration);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt64(0, memberId);
                statement->setInt32(1, memberType);
                statement->setInt32(2, seasonId);
                statement->setInt32(3, leagueId);

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
        BlazeRpcError OSDKSeasonalPlayDb::updateRegistration(MemberId memberId, MemberType memberType, SeasonId newSeasonId, LeagueId newLeagueId)
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
                        ERR_LOG("[OSDKSeasonalPlayDb].fetchSeasonNumber() - unable to update registration for member " << memberId << " of type "
                                << MemberTypeToString(memberType));
                        mStatementError = OSDKSEASONALPLAY_ERR_INVALID_PARAMS;
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
        BlazeRpcError OSDKSeasonalPlayDb::updateSeasonNumber(SeasonId seasonId, uint32_t seasonNumber, uint32_t lastStatPeriodId)
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
                        ERR_LOG("[OSDKSeasonalPlayDb].updateSeasonNumber() - unable to update season number for season " << seasonId);
                        mStatementError = OSDKSEASONALPLAY_ERR_DB;
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
        BlazeRpcError OSDKSeasonalPlayDb::updateMemberTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus status)
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
                        ERR_LOG("[OSDKSeasonalPlayDb].updateMemberTournamentStatus() - unable to update tournament status for member " << memberId <<
                                " of type " << MemberTypeToString(memberType));
                        mStatementError = OSDKSEASONALPLAY_ERR_INVALID_PARAMS;
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
        BlazeRpcError OSDKSeasonalPlayDb::updateSeasonTournamentStatus(SeasonId seasonId, TournamentStatus status)
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
                    // it's ok if no rows have been updated.
                    if (dbResult == NULL)
                    {
                        ERR_LOG("[OSDKSeasonalPlayDb].updateSeasonTournamentStatus() - unable to update tournament status for season " << seasonId);
                        mStatementError = OSDKSEASONALPLAY_ERR_INVALID_PARAMS;
                    }
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief updateAwardMember

            updates the member id for an award assignment
        */
        /*************************************************************************************************/
        BlazeRpcError OSDKSeasonalPlayDb::updateAwardMember(uint32_t assignmentId, MemberId memberId)
        {
            PreparedStatement *statement = getStatementFromId(mCmdUpdateAwardMember);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, assignmentId);
                statement->setInt64(1, memberId);

                DbResultPtr dbResult;
                executeStatementInTxn(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve one or more rows.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() == 0)
                    {
                        // award not updated
                        ERR_LOG("[OSDKSeasonalPlayDb].updateAwardMember() - unable to update award asssignment " << assignmentId <<
                                " to memberId  " << memberId);
                        mStatementError = OSDKSEASONALPLAY_ERR_INVALID_PARAMS;
                    }
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief deleteAward

            Deletes the award assignment
        */
        /*************************************************************************************************/
        BlazeRpcError OSDKSeasonalPlayDb::deleteAward(uint32_t assignmentId)
        {
            PreparedStatement *statement = getStatementFromId(mCmdDeleteAward);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, assignmentId);

                DbResultPtr dbResult;
                executeStatementInTxn(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve one or more rows.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() == 0)
                    {
                        // award not updated
                        ERR_LOG("[OSDKSeasonalPlayDb].updateAwardMember() - unable to delete award asssignment " << assignmentId);
                        mStatementError = OSDKSEASONALPLAY_ERR_INVALID_PARAMS;
                    }
                }
            }

            return mStatementError;
        }

        /*************************************************************************************************/
        /*!
            \brief deleteRegistration

            Deletes the member in the registration
        */
        /*************************************************************************************************/
        BlazeRpcError OSDKSeasonalPlayDb::deleteRegistration(MemberId memberId, MemberType memberType)
        {
            PreparedStatement *statement = getStatementFromId(mCmdDeleteRegistration);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt64(0, memberId);
                statement->setInt32(1, memberType);

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK == mStatementError)
                {
                    // a successful edit will involve one or more rows.
                    if (dbResult == NULL || dbResult->getAffectedRowCount() == 0)
                    {
                        ERR_LOG("[OSDKSeasonalPlayDb].deleteRegistration() - unable to delete registration Id = " << memberId << " memberType = " << memberType);
                        mStatementError = OSDKSEASONALPLAY_ERR_NOT_FOUND;
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
        PreparedStatement* OSDKSeasonalPlayDb::getStatementFromId(PreparedStatementId id)
        {
            mStatementError = ERR_OK;

            PreparedStatement* thisStatement = NULL;
            if (mDbConn != NULL)
            {
                thisStatement = mDbConn->getPreparedStatement(id);
                if (NULL == thisStatement)
                {
                    ERR_LOG("[OSDKSeasonalPlayDb].getStatementFromId() no valid statement found for statement id " << id);
                    mStatementError = OSDKSEASONALPLAY_ERR_DB;
                }
            }
            else
            {
                WARN_LOG("[OSDKSeasonalPlayDb].getStatementFromId() no dbConn specified!");
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
        void OSDKSeasonalPlayDb::executeStatement(PreparedStatement* statement, DbResultPtr& result)
        {
            mStatementError = ERR_OK;

            if (mDbConn != NULL)
            {
                if (NULL != statement)
                {
                    eastl::string queryDisplayBuf;
                    TRACE_LOG("[OSDKSeasonalPlayDb:" << this << "].executeStatement() trying statement " << statement << " @ dbConn " << mDbConn->getName() << ":");
                    TRACE_LOG("\t" << statement->toString(queryDisplayBuf) << "");

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
                            ERR_LOG("[OSDKSeasonalPlayDb:" << this << "].executeStatement() a database error has occured");
                            mStatementError = OSDKSEASONALPLAY_ERR_DB;
                            break;
                        default:
                            ERR_LOG("[OSDKSeasonalPlayDb:" << this << "].executeStatement() a general error has occured");
                            mStatementError = OSDKSEASONALPLAY_ERR_GENERAL;
                            break;
                    }
                }
                else
                {
                    WARN_LOG("[OSDKSeasonalPlayDb].executeStatement() - invalid prepared statement!");
                    mStatementError = OSDKSEASONALPLAY_ERR_INVALID_PARAMS;
                }
            }
            else
            {
                ERR_LOG("[OSDKSeasonalPlayDb:" << this << "].executeStatement() no database connection available");
                mStatementError = OSDKSEASONALPLAY_ERR_DB;
            }

            if (ERR_OK == mStatementError)
            {
                TRACE_LOG("[OSDKSeasonalPlayDb:" << this << "].executeStatement() statement " << statement << " @ dbConn " << mDbConn->getName() << " successful!");
            }
        }

        /*************************************************************************************************/
        /*!
            \brief executeStatement

            Helper which attempts to execute the prepared statement registered by the supplied id.
        */
        /*************************************************************************************************/
        void OSDKSeasonalPlayDb::executeStatement(PreparedStatementId statementId, DbResultPtr& result)
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
        void OSDKSeasonalPlayDb::executeStatementInTxn(PreparedStatement* statement, DbResultPtr& result)
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
                    ERR_LOG("[OSDKSeasonalPlayDb:" << this << "].executeStatementInTxn() unable to start db transaction");
                }
            }
            else
            {
                ERR_LOG("[OSDKSeasonalPlayDb:" << this << "].executeStatementInTxn() no database connection available");
                mStatementError = OSDKSEASONALPLAY_ERR_DB;
            }
        }

        /*************************************************************************************************/
        /*!
            \brief lock season

            Helper which attempts to lock season to prevent simultaneous modifying the same season
        */
        /*************************************************************************************************/
        BlazeRpcError OSDKSeasonalPlayDb::lockSeason(SeasonId seasonId)
        {
            PreparedStatement *statement = getStatementFromId(mCmdLockSeason);
            if (NULL != statement && ERR_OK == mStatementError)
            {
                statement->setInt32(0, seasonId);    

                DbResultPtr dbResult;
                executeStatement(statement, dbResult);
                if (ERR_OK != mStatementError)
                {
                    TRACE_LOG("[OSDKSeasonalPlayDb].lockSeason() - unable to find season with id " << seasonId << "");
                    mStatementError = OSDKSEASONALPLAY_ERR_SEASON_NOT_FOUND;
                }
            }

            return mStatementError;
        }
    } // OSDKSeasonalPlay
} // Blaze
