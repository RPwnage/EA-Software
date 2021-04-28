/*************************************************************************************************/
/*!
    \file   coopseasondb.cpp

    $Header: //gosdev/games/FIFA/2013/Gen3/DEV/blazeserver/3.x/customcomponent/coopseason/coopseasondb.cpp#1 $
    $Change: 286819 $
    $DateTime: 2012/12/19 16:14:33 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "coopseason/coopseasondb.h"
#include "coopseason/coopseasonqueries.h"

// global includes

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"

#include "framework/database/dbrow.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// coopseason includes
#include "coopseasonslaveimpl.h"
#include "coopseason/tdf/coopseasontypes.h"

namespace Blaze
{
    namespace CoopSeason
    {
        // Statics
        EA_THREAD_LOCAL PreparedStatementId CoopSeasonDb::mCmdFetchAllCoopIds = 0;
        EA_THREAD_LOCAL PreparedStatementId CoopSeasonDb::mCmdFetchCoopIdDataByCoopID = 0;
		EA_THREAD_LOCAL PreparedStatementId CoopSeasonDb::mCmdFetchCoopIdDataByBlazeIDs = 0;
		EA_THREAD_LOCAL PreparedStatementId CoopSeasonDb::mCmdRemoveCoopIdDataByCoopID = 0;
		EA_THREAD_LOCAL PreparedStatementId CoopSeasonDb::mCmdRemoveCoopIdDataByBlazeIDs = 0;

		EA_THREAD_LOCAL PreparedStatementId CoopSeasonDb::mCmdInsertCoopIdData = 0;
		EA_THREAD_LOCAL PreparedStatementId CoopSeasonDb::mCmdUpdateCoopIdData = 0;

        /*** Public Methods ******************************************************************************/

        /*************************************************************************************************/
        /*!
            \brief constructor

            Creates a db handler from the slave component.
        */
        /*************************************************************************************************/
        CoopSeasonDb::CoopSeasonDb(CoopSeasonSlaveImpl* component) : 
            mDbConn(),
            mStatementError(ERR_OK)
        {
            if (nullptr != component)
            {
                mComponentMaster = component->getMaster();
            }
            else
            {
                ASSERT(false && "[CoopSeasonDb] - unable to determine master instance from slave!");
            }
        }

        /*************************************************************************************************/
        /*!
            \brief constructor

            Creates a db handler from the master component.
        */
        /*************************************************************************************************/
        CoopSeasonDb::CoopSeasonDb() : 
            mComponentMaster(nullptr),
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
        CoopSeasonDb::~CoopSeasonDb()
        {
        }

        /*************************************************************************************************/
        /*!
            \brief initialize

            Registers prepared statements for the specified dbId.
        */
        /*************************************************************************************************/
        void CoopSeasonDb::initialize(uint32_t dbId)
        {
            INFO_LOG("[CoopSeasonDb].initialize() - dbId = "<<dbId <<" ");

            // Fetch
            mCmdFetchAllCoopIds = gDbScheduler->registerPreparedStatement(dbId, "coopseason_fetch_all_coop_ids", COOPSEASON_FETCH_ALL_COOP_IDS);
			mCmdFetchCoopIdDataByCoopID = gDbScheduler->registerPreparedStatement(dbId, "coopseason_fetch_coop_data_by_coopid", COOPSEASON_FETCH_COOP_DATA_BY_COOPID);
			mCmdFetchCoopIdDataByBlazeIDs = gDbScheduler->registerPreparedStatement(dbId, "coopseason_fetch_coop_data_by_blazeids", COOPSEASON_FETCH_COOP_DATA_BY_BLAZEIDS);
			//Remove
			mCmdRemoveCoopIdDataByCoopID = gDbScheduler->registerPreparedStatement(dbId, "coopseason_remove_coop_data_by_coopid", COOPSEASON_REMOVE_COOP_DATA_BY_COOPID);
			mCmdRemoveCoopIdDataByBlazeIDs = gDbScheduler->registerPreparedStatement(dbId, "coopseason_remove_coop_data_by_blazeids", COOPSEASON_REMOVE_COOP_DATA_BY_BLAZEIDS);
			//Insert
			mCmdInsertCoopIdData = gDbScheduler->registerPreparedStatement(dbId, "coopseason_insert_coop_data", COOPSEASON_INSERT_NEW_COOP_METADATA);
			//Update
			mCmdUpdateCoopIdData = gDbScheduler->registerPreparedStatement(dbId, "coopseason_update_coop_data", COOPSEASON_UPDATE_COOP_METADATA);


       }
        
        /*************************************************************************************************/
        /*!
            \brief setDbConnection

            Sets the database connection to a new instance.
        */
        /*************************************************************************************************/
        BlazeRpcError CoopSeasonDb::setDbConnection(DbConnPtr dbConn)
        { 
			BlazeRpcError error = ERR_OK;
            if (dbConn == nullptr)
            {
				TRACE_LOG("[CoopSeasonDb].setDbConnection() - NULL dbConn passed in");
				error = COOPSEASON_ERR_DB;
            }
            else
            {
                mDbConn = dbConn;
            }

			return error;
        }
		/*************************************************************************************************/
		/*!
		\brief insertCoopIdData

		Insert coop Id and metadata
		*/
		/*************************************************************************************************/
		BlazeRpcError CoopSeasonDb::insertCoopIdData(BlazeId memberOneId, BlazeId memberTwoId, const char8_t* metaData, Blaze::CoopSeason::CoopPairData &coopPairData)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdInsertCoopIdData);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				BlazeId largerId = (memberOneId > memberTwoId)? memberOneId:memberTwoId ;
				BlazeId smallerId = (memberOneId > memberTwoId)? memberTwoId:memberOneId ;

				statement->setInt64(0, largerId);
				statement->setInt64(1, smallerId);
				statement->setBinary(2, reinterpret_cast<const uint8_t*>(metaData), strlen(metaData) + 1);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);
				
				if (ERR_OK == executeStatementError)
				{
					coopPairData.setCoopId(static_cast<uint64_t>(dbResult->getLastInsertId()));
					TRACE_LOG("[CoopSeasonDb].insertCoopIdData() - Got an ID back from INSERT " << coopPairData.getCoopId());
				}
				else
				{
					TRACE_LOG("[CoopSeasonDb].insertCoopIdData() - Error is SELECT after INSERT");
				}
			}
			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
		\brief updateCoopIdData

		Update coop Id and metadata
		*/
		/*************************************************************************************************/
		BlazeRpcError CoopSeasonDb::updateCoopIdData(BlazeId memberOneId, BlazeId memberTwoId, const char8_t* metaData)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdUpdateCoopIdData);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				BlazeId largerId = (memberOneId > memberTwoId)? memberOneId:memberTwoId ;
				BlazeId smallerId = (memberOneId > memberTwoId)? memberTwoId:memberOneId ;

				statement->setBinary(0, reinterpret_cast<const uint8_t*>(metaData), strlen(metaData) + 1);
				statement->setInt64(1, largerId);
				statement->setInt64(2, smallerId);
				
				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);
			}

			return executeStatementError;

		}

		/*************************************************************************************************/
		/*!
		\brief getCoopIdentities

		Update coop Id and metadata
		*/
		/*************************************************************************************************/
		BlazeRpcError CoopSeasonDb::getCoopIdentities(const CoopIdList *coopId, CoopPairDataList *coopList)
		{       
			BlazeRpcError result = Blaze::ERR_SYSTEM;

			if (mDbConn != nullptr)
			{
				QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);

				if (query != nullptr)
				{
					query->append(COOPSEASON_GET_IDENTITY);

					CoopIdList::const_iterator it = coopId->begin();
					while (it != coopId->end())
					{
						query->append("$u", static_cast<uint32_t>(*it++));
						if (it == coopId->end())
							query->append(")");
						else
							query->append(",");
					}

					DbResultPtr dbresult;
					BlazeRpcError error = mDbConn->executeQuery(query, dbresult);
					if (error == DB_ERR_OK)
					{
						if (dbresult->empty())
						{
							result = COOPSEASON_ERR_PAIR_NOT_FOUND;
						}
						else
						{
							DbResult::const_iterator dbIt = dbresult->begin();
							for (dbIt = dbresult->begin(); dbIt != dbresult->end(); dbIt++)
							{
								const DbRow *dbrow = *dbIt;

								uint32_t col = 0;
								CoopPairData *coopPair = BLAZE_NEW CoopPairData();

								coopPair->setCoopId(static_cast<CoopId>(dbrow->getUInt(col++)));
								coopPair->setMemberOneBlazeId(dbrow->getUInt64(col++));
								coopPair->setMemberTwoBlazeId(dbrow->getUInt64(col++));

								coopList->push_back(coopPair);
							}

							result = ERR_OK;
						}
					}
				}
			}

			return result;
		}

		/*************************************************************************************************/
		/*!
			\brief fetchAllCoopIds

			Retrieves the all the coop id and friend id a target user have coop season with
		*/
		/*************************************************************************************************/
		BlazeRpcError CoopSeasonDb::fetchAllCoopIds(BlazeId targetId, uint32_t &count, Blaze::CoopSeason::CoopPairDataList &coopPairDataList)
        {
			BlazeRpcError executeStatementError = ERR_OK;

            PreparedStatement *statement = getStatementFromId(mCmdFetchAllCoopIds);
			executeStatementError = mStatementError;
            if (nullptr != statement && ERR_OK == executeStatementError)
            {
                statement->setInt64(0, targetId);
                statement->setInt64(1, targetId);
				count =0;

                DbResultPtr dbResult;
                executeStatementError = executeStatement(statement, dbResult); 
                if (ERR_OK == executeStatementError && dbResult != nullptr && !dbResult->empty())
                {
					DbResult::const_iterator itr = dbResult->begin();
					DbResult::const_iterator itr_end = dbResult->end();
					for ( ; itr != itr_end; ++itr)
					{						
						count++;

						Blaze::CoopSeason::CoopPairData* obj = BLAZE_NEW Blaze::CoopSeason::CoopPairData();
						//Blaze::CoopSeason::CoopPairData obj;

						DbRow* thisRow = (*itr);
						obj->setCoopId(thisRow->getInt64("coopId"));
						obj->setMemberOneBlazeId(thisRow->getInt64("memberOneBlazeId"));
						obj->setMemberTwoBlazeId(thisRow->getInt64("memberTwoBlazeId"));						
						//JLO: This rpc only returns the ids, if the metadata is needed as well, can activate the line below and update the sql at coopseasonqueries.h
						//size_t len;
						//obj->setMetadata(reinterpret_cast<const char8_t*>(thisRow->getBinary("metaData", &len)));																												
						coopPairDataList.push_back(obj);
					}
                }
                else
                {
                    TRACE_LOG("[CoopSeasonDb].fetchAllCoopIds() - No coop pair found for target Id "<<targetId <<" ");
                }
            }

            return executeStatementError;
        }

		/*************************************************************************************************/
		/*!
		\brief fetchCoopIdDataByCoopId

		Retrieves coop data by coop id
		*/
		/*************************************************************************************************/
		BlazeRpcError CoopSeasonDb::fetchCoopIdDataByCoopId(uint64_t coopId, Blaze::CoopSeason::CoopPairData &coopPairData)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdFetchCoopIdDataByCoopID);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setInt64(0, coopId);

				size_t len;
				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult); 
				if (ERR_OK == executeStatementError && dbResult != nullptr && !dbResult->empty())
				{
					DbRow* thisRow = *(dbResult->begin());
					coopPairData.setCoopId(thisRow->getInt64("coopId"));
					coopPairData.setMemberOneBlazeId(thisRow->getInt64("memberOneBlazeId"));
					coopPairData.setMemberTwoBlazeId(thisRow->getInt64("memberTwoBlazeId"));
					coopPairData.setMetadata(reinterpret_cast<const char8_t*>(thisRow->getBinary("metaData", &len)));
				}
				else
				{
					TRACE_LOG("[CoopSeasonDb].fetchCoopIdDataByCoopId() - Not coop pair found for coopId Id "<<coopId <<" ");
				}
			}
			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
		\brief fetchCoopIdDataByBlazeIds

		Retrieves coop data by blaze ids
		*/
		/*************************************************************************************************/
		BlazeRpcError CoopSeasonDb::fetchCoopIdDataByBlazeIds(BlazeId memberOneId, BlazeId memberTwoId, Blaze::CoopSeason::CoopPairData &coopPairData)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdFetchCoopIdDataByBlazeIDs);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				BlazeId largerId = (memberOneId > memberTwoId)? memberOneId:memberTwoId ;
				BlazeId smallerId = (memberOneId > memberTwoId)? memberTwoId:memberOneId ;

				statement->setInt64(0, largerId);
				statement->setInt64(1, smallerId);

				size_t len;
				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult); 
				if (ERR_OK == mStatementError && dbResult != nullptr && !dbResult->empty())
				{
					DbRow* thisRow = *(dbResult->begin());
					coopPairData.setCoopId(thisRow->getInt64("coopId"));
					coopPairData.setMemberOneBlazeId(thisRow->getInt64("memberOneBlazeId"));
					coopPairData.setMemberTwoBlazeId(thisRow->getInt64("memberTwoBlazeId"));
					coopPairData.setMetadata(reinterpret_cast<const char8_t*>(thisRow->getBinary("metaData", &len)));
				}
				else
				{
					TRACE_LOG("[CoopSeasonDb].fetchCoopIdDataByBlazeIds() - Not coop pair found for blaze Ids memberOne "<<memberOneId <<" and memberTwo " << memberTwoId <<" ");
				}
			}
			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
		\brief removeCoopIdDataByCoopId

		Remove coop data by coop id
		*/
		/*************************************************************************************************/
		BlazeRpcError CoopSeasonDb::removeCoopIdDataByCoopId(uint64_t coopId)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdRemoveCoopIdDataByCoopID);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setInt64(0, coopId);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult); 
			}
			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
		\brief removeCoopIdDataByBlazeIds

		Remove coop data by Blaze ids
		*/
		/*************************************************************************************************/
		BlazeRpcError CoopSeasonDb::removeCoopIdDataByBlazeIds(BlazeId memberOneId, BlazeId memberTwoId)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdRemoveCoopIdDataByBlazeIDs);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				BlazeId largerId = (memberOneId > memberTwoId)? memberOneId:memberTwoId ;
				BlazeId smallerId = (memberOneId > memberTwoId)? memberTwoId:memberOneId ;

				statement->setInt64(0, largerId);
				statement->setInt64(1, smallerId);
				
				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult); 
			}
			return executeStatementError;
		}


		/*************************************************************************************************/
        /*!
            \brief getStatementFromId

            Helper which returns the statement from the dbConn (if available).
        */
        /*************************************************************************************************/
        PreparedStatement* CoopSeasonDb::getStatementFromId(PreparedStatementId id)
        {
            PreparedStatement* thisStatement = nullptr;
            if (mDbConn != nullptr)
            {
                thisStatement = mDbConn->getPreparedStatement(id);
                if (nullptr == thisStatement)
                {
                    ASSERT(false && "[CoopSeasonDb].getStatementFromId() no valid statement found");
                    mStatementError = COOPSEASON_ERR_DB;
                }
            }
            else
            {
                ASSERT(false && "[CoopSeasonDb].getStatementFromId() no dbConn specified!");
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
        BlazeRpcError CoopSeasonDb::executeStatement(PreparedStatement* statement, DbResultPtr& result)
        {
            BlazeRpcError statementError = ERR_OK;

            if (mDbConn != nullptr)
            {
                if (nullptr != statement)
                {
                    eastl::string queryDisplayBuf;
                    TRACE_LOG("[CoopSeasonDb].executeStatement() trying statement "<<statement<<" @ dbConn "<<mDbConn->getName() <<":");
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
                            ERR_LOG("[CoopSeasonDb].executeStatement() a database error: " << dbError << " has occured");
                            statementError = COOPSEASON_ERR_DB;
                            break;
                        default:
                            ERR_LOG("[CoopSeasonDb].executeStatement() a general error has occured");
                            statementError = COOPSEASON_ERR_GENERAL;
                            break;
                    }
                }
                else
                {
                    ASSERT(false && "[CoopSeasonDb].executeStatement() - invalid prepared statement!");
                    statementError = COOPSEASON_ERR_GENERAL;
                }
            }
            else
            {
                ERR_LOG("[CoopSeasonDb].executeStatement() no database connection available");
                statementError = COOPSEASON_ERR_DB;
            }

            if (ERR_OK == statementError)
            {
                TRACE_LOG("[CoopSeasonDb].executeStatement() statement "<< statement<<" @ dbConn "<<mDbConn->getName()<<" successful!");
            }

			return statementError;
        }

        /*************************************************************************************************/
        /*!
            \brief executeStatement

            Helper which attempts to execute the prepared statement registered by the supplied id.
        */
        /*************************************************************************************************/
        BlazeRpcError CoopSeasonDb::executeStatement(PreparedStatementId statementId, DbResultPtr& result)
        {
			BlazeRpcError statementError = ERR_OK;

            PreparedStatement* statement = getStatementFromId(statementId);
            if (nullptr != statement && ERR_OK == mStatementError)
            {
                statementError = executeStatement(statement, result);
            }

			return statementError;
        }

        /*************************************************************************************************/
        /*!
            \brief executeStatementInTxn

            Helper which attempts to execute the prepared statement in a transaction
        */
        /*************************************************************************************************/
        BlazeRpcError CoopSeasonDb::executeStatementInTxn(PreparedStatement* statement, DbResultPtr& result)
        {
			BlazeRpcError statementError = ERR_OK;

            if (mDbConn != nullptr)
            {
                // start the database transaction
                BlazeRpcError dbError = mDbConn->startTxn();
                if (dbError == DB_ERR_OK)
                {
                    // execute the prepared statement
                    statementError = executeStatement(statement, result);

                    if (ERR_OK == statementError)
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
                        // roll back if statementError doesn't indicate things are good
                        mDbConn->rollback();
                    }
                }
                else
                {
                    ERR_LOG("[CoopSeasonDb].executeStatementInTxn() unable to start db transaction");
                }
            }
            else
            {
                ERR_LOG("[CoopSeasonDb].executeStatementInTxn() no database connection available");
                statementError = COOPSEASON_ERR_DB;
            }
			
			return statementError;
        }

    } // CoopSeason
} // Blaze
