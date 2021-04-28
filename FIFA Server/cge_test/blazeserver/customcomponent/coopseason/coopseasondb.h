/*************************************************************************************************/
/*!
    \file   coopseasondb.h

    $Header: //gosdev/games/FIFA/2013/Gen3/DEV/blazeserver/3.x/customcomponent/coopseason/coopseasondb.h#1 $
    $Change: 286819 $
    $DateTime: 2012/12/19 16:14:33 $

    \attention
    (c) Electronic Arts Inc. 2010
*/
/*************************************************************************************************/


#ifndef BLAZE_COOPSEASON_DB_H
#define BLAZE_COOPSEASON_DB_H

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

#include "coopseason/tdf/coopseasontypes.h"
#include "framework/database/preparedstatement.h"
#include "framework/database/dbconn.h"

namespace Blaze
{
    class DbConn;
    class DbResult;
    class Query;
    class PreparedStatement;

    namespace CoopSeason
    {
        class CoopSeasonMaster;
        class CoopSeasonSlaveImpl;

        class CoopSeasonDb
        {
        public:
            CoopSeasonDb(CoopSeasonSlaveImpl* component);
            CoopSeasonDb();
            virtual ~CoopSeasonDb();

            static void initialize(uint32_t dbId);
			BlazeRpcError setDbConnection(DbConnPtr dbConn);

            BlazeRpcError getBlazeRpcError() {return mStatementError;}
            void clearBlazeRpcError() {mStatementError = ERR_OK;}

        public:
            // called by RPC commands
            BlazeRpcError fetchAllCoopIds(BlazeId targetId, uint32_t &count, Blaze::CoopSeason::CoopPairDataList &coopPairDataList);
			BlazeRpcError fetchCoopIdDataByCoopId(uint64_t coopId, Blaze::CoopSeason::CoopPairData &coopPairData);
			BlazeRpcError fetchCoopIdDataByBlazeIds(BlazeId memberOneId, BlazeId memberTwoId, Blaze::CoopSeason::CoopPairData &coopPairData);
			BlazeRpcError removeCoopIdDataByCoopId(uint64_t coopId);
			BlazeRpcError removeCoopIdDataByBlazeIds(BlazeId memberOneId, BlazeId memberTwoId);
			BlazeRpcError insertCoopIdData(BlazeId memberOneId, BlazeId memberTwoId, const char8_t* metaData, Blaze::CoopSeason::CoopPairData &coopPairData);
			BlazeRpcError updateCoopIdData(BlazeId memberOneId, BlazeId memberTwoId, const char8_t* metaData);
			BlazeRpcError getCoopIdentities(const CoopIdList *coopId, CoopPairDataList *coopList);

        private:
            PreparedStatement* getStatementFromId(PreparedStatementId id);

            BlazeRpcError executeStatementInTxn(PreparedStatement* statement, DbResultPtr& result);
            BlazeRpcError executeStatement(PreparedStatement* statement, DbResultPtr& result);
            BlazeRpcError executeStatement(PreparedStatementId statementId, DbResultPtr& result);

            CoopSeasonMaster* mComponentMaster;
            DbConnPtr mDbConn;

            BlazeRpcError mStatementError;

        private:
            // static data
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchAllCoopIds;
            static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchCoopIdDataByCoopID;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchCoopIdDataByBlazeIDs;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdRemoveCoopIdDataByCoopID;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdRemoveCoopIdDataByBlazeIDs;

			static EA_THREAD_LOCAL PreparedStatementId  mCmdInsertCoopIdData;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateCoopIdData;
            
        };

    } // CoopSeason
} // Blaze

#endif // BLAZE_COOPSEASON_DB_H
