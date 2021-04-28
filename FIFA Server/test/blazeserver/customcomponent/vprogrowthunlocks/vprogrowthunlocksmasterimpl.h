/*************************************************************************************************/
/*!
    \file   vprogrowthunlocksmasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_VPROGROWTHUNLOCKS_MASTERIMPL_H
#define BLAZE_VPROGROWTHUNLOCKS_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "vprogrowthunlocks/rpc/vprogrowthunlocksmaster_stub.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace VProGrowthUnlocks
{

class VProGrowthUnlocksMasterImpl : public VProGrowthUnlocksMasterStub
{
public:
    VProGrowthUnlocksMasterImpl();
    ~VProGrowthUnlocksMasterImpl();

private:
	virtual uint16_t  		getDbSchemaVersion() const	{ return 8; }

    bool onConfigure();

    PokeMasterError::Error processPokeMaster(const VProGrowthUnlocksRequest &request, VProGrowthUnlocksResponse &response, const Message *message);

	uint32_t mDbId;
};

} // VProGrowthUnlocks
} // Blaze

#endif  // BLAZE_VPROGROWTHUNLOCKS_MASTERIMPL_H
