/*************************************************************************************************/
/*!
	\file   proclubscustomdb.h


	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/


#ifndef BLAZE_proclubscustom_DB_H
#define BLAZE_proclubscustom_DB_H

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"

#include "proclubscustom/tdf/proclubscustomtypes.h"
#include "framework/database/preparedstatement.h"
#include "framework/database/dbconn.h"

namespace Blaze
{
    class DbConn;
    class DbResult;
    class Query;
    class PreparedStatement;

    namespace proclubscustom
    {
        class ProClubsCustomMaster;
        class ProclubsCustomSlaveImpl;
		
		static const size_t NUM_MORPH_REGIONS = 8;

        class ProclubsCustomDb
        {
        public:
            ProclubsCustomDb(ProclubsCustomSlaveImpl* component);
            ProclubsCustomDb();
            virtual ~ProclubsCustomDb();

            static void initialize(uint32_t dbId);
			BlazeRpcError setDbConnection(DbConnPtr dbConn);

            BlazeRpcError getBlazeRpcError() {return mStatementError;}
            void clearBlazeRpcError() {mStatementError = ERR_OK;}

        public:
            // called by RPC commands		
			BlazeRpcError fetchSetting(uint64_t clubId, Blaze::proclubscustom::customizationSettings &settings);
			BlazeRpcError updateSetting(uint64_t clubId, const Blaze::proclubscustom::customizationSettings &settings);
			
			BlazeRpcError fetchAIPlayerCustomization(uint64_t clubId, EA::TDF::TdfStructVector < Blaze::proclubscustom::AIPlayerCustomization> &aiCustomization);
			BlazeRpcError updateAIPlayerCustomization(uint64_t clubId, int32_t slotId, const Blaze::proclubscustom::AIPlayerCustomization &aiPlayerCustomizationBulk, const Blaze::proclubscustom::AvatarCustomizationPropertyMap &modifiedItems);
			
			BlazeRpcError fetchCustomTactics(uint64_t clubId, EA::TDF::TdfStructVector <Blaze::proclubscustom::CustomTactics> &tactics);
			BlazeRpcError updateCustomTactics(uint64_t clubId, uint32_t slotId, const Blaze::proclubscustom::CustomTactics &tactics);
			BlazeRpcError resetAiPlayerNames(uint64_t clubId, eastl::string& statusMessage);
			BlazeRpcError fetchAiPlayerNames(uint64_t clubId, eastl::string &playerNames);

			BlazeRpcError fetchAvatarData(int64_t blazeId, Blaze::proclubscustom::AvatarData &avatarData, uint32_t maxAvatarNameResets);
			BlazeRpcError fetchAvatarName(int64_t blazeId, eastl::string &avatarFirstName, eastl::string &avatarLastName);
			BlazeRpcError insertAvatarName(int64_t blazeId, eastl::string &avatarFirstName, eastl::string &avatarLastName);
			BlazeRpcError flagProfaneAvatarName(int64_t blazeId);

        private:
            PreparedStatement* getStatementFromId(PreparedStatementId id);
            BlazeRpcError executeStatementInTxn(PreparedStatement* statement, DbResultPtr& result);
            BlazeRpcError executeStatement(PreparedStatement* statement, DbResultPtr& result);
            BlazeRpcError executeStatement(PreparedStatementId statementId, DbResultPtr& result);

			void appendToStatementForInsert(eastl::string& insertStr, eastl::string& valueStr, eastl::string& updateStr, const char* insertLabel, const int32_t value);
			void appendToStatementForInsert(eastl::string& insertStr, eastl::string& valueStr, eastl::string& updateStr, const char* insertLabel, const uint64_t value);
			void appendToStatementForInsert(eastl::string& insertStr, eastl::string& valueStr, eastl::string& updateStr, const char* insertLabel, const float value);
			void appendToStatementForInsert(eastl::string& insertStr, eastl::string& valueStr, eastl::string& updateStr, const char* insertLabel, const char8_t* value);

            proclubscustomMaster* mComponentMaster;
            DbConnPtr mDbConn;
            BlazeRpcError mStatementError;

        private:
            // static data           
			static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchSettings;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateSettings;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchAIPlayerCustomization;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdUpdateAIPlayerCustomization;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchCustomTactics;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdInsertCustomTactics;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdResetAiPlayerNames;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchAiPlayerNames;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchAvatarName;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdFetchAvatarData;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdInsertAvatarName;
			static EA_THREAD_LOCAL PreparedStatementId  mCmdFlagProfaneAvatarName;
        };

    } // VProGrowthUnlocks
} // Blaze

#endif // BLAZE_VPROGROWTHUNLOCKS_DB_H
