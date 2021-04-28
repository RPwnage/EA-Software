/*************************************************************************************************/
/*!
	\file   vprogrowthunlocksdb.h


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#ifndef BLAZE_VPROGROWTHUNLOCKS_DB_H
#define BLAZE_VPROGROWTHUNLOCKS_DB_H

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

#include "vprogrowthunlocks/tdf/vprogrowthunlockstypes.h"
#include "framework/database/preparedstatement.h"
#include "framework/database/dbconn.h"

namespace Blaze
{
    class DbConn;
    class DbResult;
    class Query;
    class PreparedStatement;

    namespace VProGrowthUnlocks
    {
        class VProGrowthUnlocksMaster;
        class VProGrowthUnlocksSlaveImpl;

        class VProGrowthUnlocksDb
        {
        public:
            VProGrowthUnlocksDb(VProGrowthUnlocksSlaveImpl* component);
            VProGrowthUnlocksDb();
            virtual ~VProGrowthUnlocksDb();

            static void initialize(uint32_t dbId);
			BlazeRpcError setDbConnection(DbConnPtr dbConn);

            BlazeRpcError getBlazeRpcError() {return mStatementError;}
            void clearBlazeRpcError() {mStatementError = ERR_OK;}

        public:
            // called by RPC commands		
			BlazeRpcError fetchXPEarned(BlazeId userId, uint32_t loadOutId, uint32_t &xpEarned);
			BlazeRpcError fetchLoadOuts(Blaze::VProGrowthUnlocks::UserIdList idList, Blaze::VProGrowthUnlocks::VProLoadOutList &loadOutList, uint32_t &count);
			BlazeRpcError fetchLoadOuts(BlazeId userId, Blaze::VProGrowthUnlocks::VProLoadOutList &loadOutList, uint32_t &count);
			BlazeRpcError resetLoadOuts(BlazeId userId);		
			BlazeRpcError resetSingleLoadOut(BlazeId userId, uint32_t loadOutId);			
			BlazeRpcError insertLoadOut(BlazeId userId, uint32_t loadOutId, const char* loadOutName, uint32_t xpEarned, uint32_t matchspgranted, uint32_t objectivespgranted, uint32_t spConsumed, uint32_t height, uint32_t weight, uint32_t position, uint32_t foot);
			BlazeRpcError updateLoadOutPeripherals(BlazeId userId, uint32_t loadOutId, const char* loadOutName, uint32_t height, uint32_t weight, uint32_t position, uint32_t foot);
			BlazeRpcError updateLoadOutPeripheralsWithoutName(BlazeId userId, uint32_t loadOutId, uint32_t height, uint32_t weight, uint32_t position, uint32_t foot);
			BlazeRpcError updateLoadOutUnlocks(BlazeId userId, Blaze::VProGrowthUnlocks::VProLoadOut &loadOut);
			BlazeRpcError updatePlayerGrowth(BlazeId userId, uint32_t xpEarned, uint32_t matchSpGranted);
			BlazeRpcError updateAllSPGranted(BlazeId userId, uint32_t matchSpGranted, uint32_t objectiveSpGranted);
			BlazeRpcError updateMatchSPGranted(BlazeId userId, uint32_t matchSpGranted);
			BlazeRpcError updateObjectiveSPGranted(BlazeId userId, uint32_t objectiveSpGranted);

        private:
            PreparedStatement* getStatementFromId(PreparedStatementId id);
            BlazeRpcError executeStatementInTxn(PreparedStatement* statement, DbResultPtr& result);
            BlazeRpcError executeStatement(PreparedStatement* statement, DbResultPtr& result);
            BlazeRpcError executeStatement(PreparedStatementId statementId, DbResultPtr& result);

            VProGrowthUnlocksMaster* mComponentMaster;
            DbConnPtr mDbConn;
            BlazeRpcError mStatementError;

        private:
            // static data           
			static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchXPEarned;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchLoadOuts;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdResetLoadOuts;	
			static EA_THREAD_LOCAL PreparedStatementId  mCmdResetSingleLoadOut;						
			static EA_THREAD_LOCAL PreparedStatementId  mCmdInsertLoadOut;		
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateLoadOutPeripherls;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateLoadOutPeripherlsWithoutName;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateLoadOutUnlocks;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdatePlayerGrowth;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateAllSPGranted;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateMatchSPGranted;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateObjectiveSPGranted;

        };

    } // VProGrowthUnlocks
} // Blaze

#endif // BLAZE_VPROGROWTHUNLOCKS_DB_H
