/*************************************************************************************************/
/*!
    \file   fifagroupsslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFAGROUPS_SLAVEIMPL_H
#define BLAZE_FIFAGROUPS_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "fifagroups/rpc/fifagroupsslave_stub.h"
#include "fifagroups/rpc/fifagroupsmaster.h"
#include "fifagroups/tdf/fifagroupstypes.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace FifaGroups
{

class FifaGroupsSlaveImpl : 
	public FifaGroupsSlaveStub
{
public:
    FifaGroupsSlaveImpl();
    ~FifaGroupsSlaveImpl();

	BlazeRpcError processMergeGroupInstances(MergeGroupInstancesRequest &request);

private:
	virtual bool onConfigure();
	virtual bool onReconfigure();
	virtual void onShutdown();

	//! Called to perform tasks common to onConfigure and onReconfigure
	bool configureHelper();

	void mergeGroups(Blaze::UserSessionId userSessionId, eastl::string serviceName, BlazeId userId, MergeGroupInstancesRequest request);

};

} // FifaGroups
} // Blaze

#endif // BLAZE_FIFAGROUPS_SLAVEIMPL_H

