/*************************************************************************************************/
/*!
    \file   fifastatsslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFASTATS_SLAVEIMPL_H
#define BLAZE_FIFASTATS_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "fifastats/rpc/fifastatsslave_stub.h"
#include "fifastats/rpc/fifastatsmaster.h"
#include "fifastats/tdf/fifastatstypes.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace FifaStats
{

class FifaStatsSlaveImpl : 
	public FifaStatsSlaveStub
{
public:
	FifaStatsSlaveImpl();
    ~FifaStatsSlaveImpl();

private:
	virtual bool onConfigure();
	virtual bool onReconfigure();
	virtual void onShutdown();

	//! Called to perform tasks common to onConfigure and onReconfigure
	bool configureHelper();
};

} // FifaStats
} // Blaze

#endif // BLAZE_FIFASTATS_SLAVEIMPL_H

