/*************************************************************************************************/
/*!
    \file   fifastatsmasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFASTATS_MASTERIMPL_H
#define BLAZE_FIFASTATS_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "fifastats/rpc/fifastatsmaster_stub.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace FifaStats
{

class FifaStatsMasterImpl : public FifaStatsMasterStub
{
public:
	FifaStatsMasterImpl();
    ~FifaStatsMasterImpl();

private:
    bool onConfigure();
    PokeMasterError::Error processPokeMaster(const FifaStatsRequest &request, FifaStatsResponse &response, const Message *message);
};

} // FifaStats
} // Blaze

#endif  // BLAZE_FIFASTATS_MASTERIMPL_H
