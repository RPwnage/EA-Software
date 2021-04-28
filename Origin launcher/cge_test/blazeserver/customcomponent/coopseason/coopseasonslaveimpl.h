/*************************************************************************************************/
/*!
    \file   coopseasonslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COOPSEASON_SLAVEIMPL_H
#define BLAZE_COOPSEASON_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "coopseason/rpc/coopseasonslave_stub.h"
#include "coopseason/rpc/coopseasonmaster.h"
#include "framework/replication/replicationcallback.h"

#include "fifacups/fifacupsdb.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace CoopSeason
{

class CoopSeasonIdentityProvider;

class CoopSeasonSlaveImpl : public CoopSeasonSlaveStub
{
public:
    CoopSeasonSlaveImpl();
    ~CoopSeasonSlaveImpl();

	// Component Interface
	virtual uint16_t getDbSchemaVersion() const { return 2; }

	bool UseCache(){return mbUseCache;}
	void getAllCoopIds(BlazeId targetId, Blaze::CoopSeason::CoopPairDataList &outCoopPairDataList, uint32_t &outCount, BlazeRpcError &outError);
	void getCoopIdDataByCoopId(uint64_t coopId, Blaze::CoopSeason::CoopPairData &outCoopPairData, BlazeRpcError &outError);
	void getCoopIdDataByBlazeIds(BlazeId memberOneId, BlazeId memberTwoId, Blaze::CoopSeason::CoopPairData &outCoopPairData, BlazeRpcError &outError);
	void removeCoopIdDataByCoopId(uint64_t coopId, BlazeRpcError &outError);
	void removeCoopIdDataByBlazeIds(BlazeId memberOneId, BlazeId memberTwoId, BlazeRpcError &outError);
	void setCoopIdData(BlazeId memberOneId, BlazeId memberTwoId, const char8_t* metaData, Blaze::CoopSeason::CoopPairData &outCoopPairData, BlazeRpcError &outError);

private:
    bool onConfigure();
	void onShutdown();
	
	//! Called to perform tasks common to onConfigure and onReconfigure
	bool configureHelper();

   	//CoopSeasonMasterListener interface
	virtual void onVoidMasterNotification(UserSession *);

    CoopSeasonIdentityProvider* mIdentityProvider;

	uint32_t mDbId;
	bool mbUseCache;
};

} // CoopSeason
} // Blaze

#endif // BLAZE_COOPSEASON_SLAVEIMPL_H

