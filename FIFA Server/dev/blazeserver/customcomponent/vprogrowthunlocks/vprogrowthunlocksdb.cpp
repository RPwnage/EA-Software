/*************************************************************************************************/
/*!
	\file   vprogrowthunlocksdb.cpp


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "vprogrowthunlocks/vprogrowthunlocksdb.h"
#include "vprogrowthunlocks/vprogrowthunlocksqueries.h"

// global includes
#include "EASTL/string.h"
#include "EAStdC/EAString.h"

// framework includes
#include "EATDF/tdf.h"
#include "framework/connection/selector.h"

#include "framework/database/dbrow.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"

// vprogrowthunlocks includes
#include "vprogrowthunlocksslaveimpl.h"
#include "vprogrowthunlocks/tdf/vprogrowthunlockstypes.h"

namespace Blaze
{
    namespace VProGrowthUnlocks
    {
        // Statics
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdFetchXPEarned = 0;
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdFetchLoadOuts = 0;
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdResetLoadOuts = 0;
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdResetSingleLoadOut = 0;
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdInsertLoadOut = 0;
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdUpdateLoadOutPeripherls = 0;
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdUpdateLoadOutPeripherlsWithoutName = 0;
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdUpdateLoadOutUnlocks = 0;
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdUpdatePlayerGrowth = 0;
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdUpdateAllSPGranted = 0;
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdUpdateMatchSPGranted = 0;
		EA_THREAD_LOCAL PreparedStatementId VProGrowthUnlocksDb::mCmdUpdateObjectiveSPGranted = 0;

        /*** Public Methods ******************************************************************************/

        /*************************************************************************************************/
        /*!
            \brief constructor

            Creates a db handler from the slave component.
        */
        /*************************************************************************************************/
        VProGrowthUnlocksDb::VProGrowthUnlocksDb(VProGrowthUnlocksSlaveImpl* component) : 
            mDbConn(),
            mStatementError(ERR_OK)
        {
            if (nullptr != component)
            {
                mComponentMaster = component->getMaster();
            }
            else
            {
				ASSERT_LOG("[VProGrowthUnlocksDb] - unable to determine master instance from slave!");
            }
        }

        /*************************************************************************************************/
        /*!
            \brief constructor

            Creates a db handler from the master component.
        */
        /*************************************************************************************************/
        VProGrowthUnlocksDb::VProGrowthUnlocksDb() : 
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
        VProGrowthUnlocksDb::~VProGrowthUnlocksDb()
        {
        }

        /*************************************************************************************************/
        /*!
            \brief initialize

            Registers prepared statements for the specified dbId.
        */
        /*************************************************************************************************/
        void VProGrowthUnlocksDb::initialize(uint32_t dbId)
        {
            INFO_LOG("[VProGrowthUnlocksDb].initialize() - dbId = "<<dbId <<" ");

            mCmdFetchXPEarned			= gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_fetch_xpEarned",		VPROGROWTHUNLOCKS_FETCH_XPEARNED);
            mCmdFetchLoadOuts			= gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_fetch",				VPROGROWTHUNLOCKS_FETCH_LOADOUTS);
			mCmdResetLoadOuts			= gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_reset",				VPROGROWTHUNLOCKS_RESET_LOADOUTS);
			mCmdResetSingleLoadOut		= gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_single_reset",		VPROGROWTHUNLOCKS_RESET_SINGLE_LOADOUT);
			mCmdInsertLoadOut			= gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_insert",				VPROGROWTHUNLOCKS_INSERT_LOADOUT);
			mCmdUpdateLoadOutPeripherls	= gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_update_peripherls",	VPROGROWTHUNLOCKS_UPDATE_LOADOUT_PERIPHERALS);
			mCmdUpdateLoadOutUnlocks	= gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_update_unlocks",		VPROGROWTHUNLOCKS_UPDATE_LOADOUT_UNLOCKS);
			mCmdUpdatePlayerGrowth		= gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_update_playerGrowth",   VPROGROWTHUNLOCKS_UPDATE_PLAYER_GROWTH);
			mCmdUpdateAllSPGranted		= gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_update_allspGranted",   VPROGROWTHUNLOCKS_UPDATE_ALL_SPGRANTED);
			mCmdUpdateMatchSPGranted	= gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_update_matchspGranted", VPROGROWTHUNLOCKS_UPDATE_MATCH_SPGRANTED);
			mCmdUpdateObjectiveSPGranted = gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_update_objectivespGranted", VPROGROWTHUNLOCKS_UPDATE_OBJECTIVE_SPGRANTED);
			mCmdUpdateLoadOutPeripherlsWithoutName	= gDbScheduler->registerPreparedStatement(dbId, "vprogrowthunlocks_update_peripherls_wo_name",	VPROGROWTHUNLOCKS_UPDATE_LOADOUT_PERIPHERALS_WITHOUT_NAME);
       }
        
        /*************************************************************************************************/
        /*!
            \brief setDbConnection

            Sets the database connection to a new instance.
        */
        /*************************************************************************************************/
        BlazeRpcError VProGrowthUnlocksDb::setDbConnection(DbConnPtr dbConn)
        { 
			BlazeRpcError error = ERR_OK;
            if (dbConn == nullptr) 
            {
				TRACE_LOG("[VProGrowthUnlocksDb].setDbConnection() - NULL dbConn passed in");
				error = VPROGROWTHUNLOCKS_ERR_DB;
            }
            else
            {
                mDbConn = dbConn;
            }

			return error;
        }
		
		/*************************************************************************************************/
		/*!
			\brief fetchXPEarned

			Retrieves XP earned for a single user
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::fetchXPEarned(BlazeId userId, uint32_t loadOutId, uint32_t &xpEarned)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdFetchXPEarned);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setInt64(0, userId);
				statement->setUInt32(1, loadOutId);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);
				if (ERR_OK == executeStatementError && dbResult != nullptr && !dbResult->empty())
				{
					DbRow* row = *dbResult->begin();
					xpEarned = row->getUInt("xpEarned");
				}
				else
				{
					TRACE_LOG("[VProGrowthUnlocksDb].fetchXPEarned() - No load out found for user Id " << userId << ", loadOutId " << loadOutId);
				}
			}
			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
			\brief fetchLoadOuts

			Retrieves vpro load outs for a multiple users
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::fetchLoadOuts(Blaze::VProGrowthUnlocks::UserIdList idList, Blaze::VProGrowthUnlocks::VProLoadOutList &loadOutList, uint32_t &count)
		{
			count = 0;
			BlazeRpcError executeStatementError = ERR_OK;

			if (mDbConn != nullptr)
			{
				QueryPtr query = DB_NEW_QUERY_PTR(mDbConn);
				if (query != nullptr)
				{
					query->append(VPROGROWTHUNLOCKS_FETCH_LOADOUTS_MULTI_USERS);

					Blaze::VProGrowthUnlocks::UserIdList::const_iterator it = idList.begin();
					while (it != idList.end())
					{
	                    query->append("$D", *it++);
		                if (it == idList.end())
			                query->append(")");
				        else
					        query->append(",");
					}

					DbResultPtr dbResult;
					executeStatementError = mDbConn->executeQuery(query, dbResult);
					if (ERR_OK == executeStatementError && dbResult != nullptr && !dbResult->empty())
					{
						DbResult::const_iterator itr = dbResult->begin();
						DbResult::const_iterator itr_end = dbResult->end();
						for ( ; itr != itr_end; ++itr)
						{
							count++;												
							DbRow* thisRow = (*itr);
						
							Blaze::VProGrowthUnlocks::VProLoadOut* obj = BLAZE_NEW Blaze::VProGrowthUnlocks::VProLoadOut();
							obj->setUserId(thisRow->getUInt64("entityId"));
							obj->setLoadOutId(thisRow->getUInt("loadoutId"));
							obj->setLoadOutName(thisRow->getString("loadoutName"));
							obj->setXPEarned(thisRow->getUInt("xpEarned"));
							obj->setSPGranted(thisRow->getUInt("matchspgranted")+ thisRow->getUInt("objectivespgranted"));
							obj->setSPConsumed(thisRow->getUInt("spconsumed"));
							obj->setUnlocks1(thisRow->getUInt64("unlocks1"));
							obj->setUnlocks2(thisRow->getUInt64("unlocks2"));
							obj->setUnlocks3(thisRow->getUInt64("unlocks3"));
							obj->setVProHeight(thisRow->getUInt("vpro_height"));						
							obj->setVProWeight(thisRow->getUInt("vpro_weight"));						
							obj->setVProPosition(thisRow->getUInt("vpro_position"));						
							obj->setVProFoot(thisRow->getUInt("vpro_foot"));			

							eastl::string assignedPerks = thisRow->getString("assignedPerks");
							eastl_size_t start = 0, end = 0;
							while ((start = assignedPerks.find_first_not_of(',', end)) != eastl::string::npos)
							{
								end = assignedPerks.find(',', start);
								int32_t perkId = EA::StdC::AtoI32(assignedPerks.substr(start, end - start).c_str());
								obj->getPerksAssignment().push_back(perkId);
							}

							loadOutList.push_back(obj);
						}
		            }
	                else
		            {
			            TRACE_LOG("[VProGrowthUnlocksDb].fetchLoadOuts() - Multi User Error " << executeStatementError <<" ");
				    }
				}				
			}			
			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
			\brief fetchLoadOuts

			Retrieves vpro load outs for a single user
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::fetchLoadOuts(BlazeId userId, Blaze::VProGrowthUnlocks::VProLoadOutList &loadOutList, uint32_t &count)
        {
			count = 0;
			BlazeRpcError executeStatementError = ERR_OK;
			
            PreparedStatement *statement = getStatementFromId(mCmdFetchLoadOuts);
			executeStatementError = mStatementError;
            if (nullptr != statement && ERR_OK == executeStatementError)
            {
                statement->setInt64(0, userId);

                DbResultPtr dbResult;
                executeStatementError = executeStatement(statement, dbResult); 
                if (ERR_OK == executeStatementError && dbResult != nullptr && !dbResult->empty())
                {
					DbResult::const_iterator itr = dbResult->begin();
					DbResult::const_iterator itr_end = dbResult->end();
					for ( ; itr != itr_end; ++itr)
					{
						count++;												
						DbRow* thisRow = (*itr);
						
						Blaze::VProGrowthUnlocks::VProLoadOut* obj = BLAZE_NEW Blaze::VProGrowthUnlocks::VProLoadOut();
						obj->setUserId(userId);
						obj->setLoadOutId(thisRow->getUInt("loadoutId"));
						obj->setLoadOutName(thisRow->getString("loadoutName"));
						obj->setXPEarned(thisRow->getUInt("xpEarned"));
						obj->setSPGranted(thisRow->getUInt("matchspgranted") + thisRow->getUInt("objectivespgranted"));
						obj->setSPConsumed(thisRow->getUInt("spconsumed"));
						obj->setUnlocks1(thisRow->getUInt64("unlocks1"));
						obj->setUnlocks2(thisRow->getUInt64("unlocks2"));
						obj->setUnlocks3(thisRow->getUInt64("unlocks3"));
						obj->setVProHeight(thisRow->getUInt("vpro_height"));						
						obj->setVProWeight(thisRow->getUInt("vpro_weight"));						
						obj->setVProPosition(thisRow->getUInt("vpro_position"));						
						obj->setVProFoot(thisRow->getUInt("vpro_foot"));						

						eastl::string assignedPerks = thisRow->getString("assignedPerks");
						eastl_size_t start = 0, end = 0;
						while ((start = assignedPerks.find_first_not_of(',', end)) != eastl::string::npos)
						{
							end = assignedPerks.find(',', start);
							int32_t perkId = EA::StdC::AtoI32(assignedPerks.substr(start, end - start).c_str());
							obj->getPerksAssignment().push_back(perkId);
						}

						loadOutList.push_back(obj);
					}
                }
                else
                {
                    TRACE_LOG("[VProGrowthUnlocksDb].fetchLoadOuts() - No load out found for user Id "<<userId <<" ");
                }
            }
            return executeStatementError;
        }

		/*************************************************************************************************/
		/*!
			\brief resetLoadOuts

			Reset vpro load outs
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::resetLoadOuts(BlazeId userId)
        {
			BlazeRpcError executeStatementError = ERR_OK;

            PreparedStatement *statement = getStatementFromId(mCmdResetLoadOuts);
			executeStatementError = mStatementError;
            if (nullptr != statement && ERR_OK == executeStatementError)
            {
				statement->setInt64(0, userId);				

                DbResultPtr dbResult;
                executeStatementError = executeStatement(statement, dbResult); 
            }
            return executeStatementError;
        }

		/*************************************************************************************************/
		/*!
			\brief resetSingleLoadOut

			Reset single vpro load out
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::resetSingleLoadOut(BlazeId userId, uint32_t loadOutId)
        {
			BlazeRpcError executeStatementError = ERR_OK;

            PreparedStatement *statement = getStatementFromId(mCmdResetSingleLoadOut);
			executeStatementError = mStatementError;
            if (nullptr != statement && ERR_OK == executeStatementError)
            {
				statement->setInt64(0, userId);				
				statement->setUInt32(1, loadOutId);

                DbResultPtr dbResult;
                executeStatementError = executeStatement(statement, dbResult); 
            }
            return executeStatementError;
        }

		/*************************************************************************************************/
		/*!
			\brief insertLoadOut

			Insert vpro load out
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::insertLoadOut(BlazeId userId, uint32_t loadOutId, const char* loadOutName, uint32_t xpEarned, uint32_t matchspgranted, uint32_t objectivespgranted, uint32_t spConsumed, uint32_t height, uint32_t weight, uint32_t position, uint32_t foot)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdInsertLoadOut);

			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setInt64(0, userId);
				statement->setUInt32(1, loadOutId);
				statement->setString(2, loadOutName);
				statement->setUInt32(3, xpEarned);
				statement->setUInt32(4, matchspgranted);
				statement->setUInt32(5, objectivespgranted);
				statement->setUInt32(6, spConsumed);
				statement->setUInt32(7, height);
				statement->setUInt32(8, weight);
				statement->setUInt32(9, position);
				statement->setUInt32(10, foot);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);
			}
			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
			\brief updateLoadOutPeripherals

			Update vpro load out peripherals
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::updateLoadOutPeripherals(BlazeId userId, uint32_t loadOutId, const char* loadOutName, uint32_t height, uint32_t weight, uint32_t position, uint32_t foot)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdUpdateLoadOutPeripherls);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setString(0, loadOutName);
				statement->setUInt32(1, height);
				statement->setUInt32(2, weight);
				statement->setUInt32(3, position);
				statement->setUInt32(4, foot);
				statement->setInt64(5, userId);								
				statement->setUInt32(6, loadOutId);
				
				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);
			}

			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
			\brief updateLoadOutPeripheralsWithoutName

			Update vpro load out peripherals without loadout name
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::updateLoadOutPeripheralsWithoutName(BlazeId userId, uint32_t loadOutId, uint32_t height, uint32_t weight, uint32_t position, uint32_t foot)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdUpdateLoadOutPeripherlsWithoutName);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setUInt32(0, height);
				statement->setUInt32(1, weight);
				statement->setUInt32(2, position);
				statement->setUInt32(3, foot);
				statement->setInt64(4, userId);								
				statement->setUInt32(5, loadOutId);
				
				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);
			}

			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
			\brief updateLoadOutUnlocks

			Update vpro load out unlocks
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::updateLoadOutUnlocks(BlazeId userId, Blaze::VProGrowthUnlocks::VProLoadOut &loadOut)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdUpdateLoadOutUnlocks);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				eastl::string assignedPerks;
				for (auto iter = loadOut.getPerksAssignment().begin(); iter != loadOut.getPerksAssignment().end(); ++iter)
				{
					assignedPerks.append_sprintf("%d", *iter);
					if (iter != loadOut.getPerksAssignment().end() - 1)
					{
						assignedPerks.push_back(',');
					}
				}

				statement->setUInt32(0, loadOut.getSPConsumed());
				statement->setUInt64(1, loadOut.getUnlocks1());
				statement->setUInt64(2, loadOut.getUnlocks2());
				statement->setUInt64(3, loadOut.getUnlocks3());
				statement->setString(4, assignedPerks.c_str());
				statement->setInt64(5, userId);								
				statement->setUInt32(6, loadOut.getLoadOutId());
				
				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);
			}

			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
			\brief updatePlayerGrowth

			Update (increase) XP earned and match skill points granted
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::updatePlayerGrowth(BlazeId userId, uint32_t xpEarned, uint32_t matchSpGranted)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdUpdatePlayerGrowth);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setUInt32(0, xpEarned);
				statement->setUInt32(1, matchSpGranted);
				statement->setInt64(2, userId);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);
			}
			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
			\brief updateAllSPGranted

			update (increase) all sp granted
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::updateAllSPGranted(BlazeId userId, uint32_t matchSpGranted, uint32_t objectiveSpGranted)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdUpdateAllSPGranted);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setUInt32(0, matchSpGranted);
				statement->setUInt32(1, objectiveSpGranted);
				statement->setInt64(2, userId);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);
			}
			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
			\brief updateMatchSPGranted

			update (increase) match sp granted
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::updateMatchSPGranted(BlazeId userId, uint32_t matchSpGranted)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdUpdateMatchSPGranted);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setUInt32(0, matchSpGranted);
				statement->setInt64(1, userId);

				DbResultPtr dbResult;
				executeStatementError = executeStatement(statement, dbResult);
			}
			return executeStatementError;
		}

		/*************************************************************************************************/
		/*!
			\brief updateObjectiveSPGranted

			update (increase) objective sp granted
		*/
		/*************************************************************************************************/
		BlazeRpcError VProGrowthUnlocksDb::updateObjectiveSPGranted(BlazeId userId, uint32_t objectiveSpGranted)
		{
			BlazeRpcError executeStatementError = ERR_OK;

			PreparedStatement *statement = getStatementFromId(mCmdUpdateObjectiveSPGranted);
			executeStatementError = mStatementError;
			if (nullptr != statement && ERR_OK == executeStatementError)
			{
				statement->setUInt32(0, objectiveSpGranted);
				statement->setInt64(1, userId);

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
        PreparedStatement* VProGrowthUnlocksDb::getStatementFromId(PreparedStatementId id)
        {
            PreparedStatement* thisStatement = nullptr;
            if (mDbConn != nullptr)
            {
                thisStatement = mDbConn->getPreparedStatement(id);
                if (nullptr == thisStatement)
                {
					ASSERT_LOG("[VProGrowthUnlocksDb].getStatementFromId() no valid statement found");
                    mStatementError = VPROGROWTHUNLOCKS_ERR_DB;
                }
            }
            else
            {
				ASSERT_LOG("[VProGrowthUnlocksDb].getStatementFromId() no dbConn specified!");
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
        BlazeRpcError VProGrowthUnlocksDb::executeStatement(PreparedStatement* statement, DbResultPtr& result)
        {
            BlazeRpcError statementError = ERR_OK;

            if (mDbConn != nullptr)
            {
                if (nullptr != statement)
                {
                    eastl::string queryDisplayBuf;
                    TRACE_LOG("[VProGrowthUnlocksDb].executeStatement() trying statement "<<statement<<" @ dbConn "<<mDbConn->getName() <<":");
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
                            ERR_LOG("[VProGrowthUnlocksDb].executeStatement() a database error: " << dbError << " has occured");
                            statementError = VPROGROWTHUNLOCKS_ERR_DB;
                            break;
                        default:
                            ERR_LOG("[VProGrowthUnlocksDb].executeStatement() a general error has occured");
                            statementError = VPROGROWTHUNLOCKS_ERR_UNKNOWN;
                            break;
                    }
                }
                else
                {
					ASSERT_LOG("[VProGrowthUnlocksDb].executeStatement() - invalid prepared statement!");
                    statementError = VPROGROWTHUNLOCKS_ERR_UNKNOWN;
                }
            }
            else
            {
                ERR_LOG("[VProGrowthUnlocksDb].executeStatement() no database connection available");
                statementError = VPROGROWTHUNLOCKS_ERR_DB;
            }

            if (ERR_OK == statementError)
            {
                TRACE_LOG("[VProGrowthUnlocksDb].executeStatement() statement "<< statement<<" @ dbConn "<<mDbConn->getName()<<" successful!");
            }

			return statementError;
        }

        /*************************************************************************************************/
        /*!
            \brief executeStatement

            Helper which attempts to execute the prepared statement registered by the supplied id.
        */
        /*************************************************************************************************/
        BlazeRpcError VProGrowthUnlocksDb::executeStatement(PreparedStatementId statementId, DbResultPtr& result)
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
        BlazeRpcError VProGrowthUnlocksDb::executeStatementInTxn(PreparedStatement* statement, DbResultPtr& result)
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
                            dbError = mDbConn->rollback();
							if (dbError != DB_ERR_OK)
							{
								ERR_LOG("[VProGrowthUnlocksDb].executeStatementInTxn() unable to roll back after fail to commit");
							}
                        }
                    }
                    else
                    {
                        // roll back if statementError doesn't indicate things are good
                        dbError = mDbConn->rollback();
						if (dbError != DB_ERR_OK)
						{
							ERR_LOG("[VProGrowthUnlocksDb].executeStatementInTxn() unable to roll back");
						}
                    }
                }
                else
                {
                    ERR_LOG("[VProGrowthUnlocksDb].executeStatementInTxn() unable to start db transaction");
                }
            }
            else
            {
                ERR_LOG("[VProGrowthUnlocksDb].executeStatementInTxn() no database connection available");
                statementError = VPROGROWTHUNLOCKS_ERR_DB;
            }
			
			return statementError;
        }

    } // VProGrowthUnlocksDb
} // Blaze
