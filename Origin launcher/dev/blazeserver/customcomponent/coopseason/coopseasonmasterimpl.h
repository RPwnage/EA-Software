/*************************************************************************************************/
/*!
    \file   coopseasonmasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COOPSEASON_MASTERIMPL_H
#define BLAZE_COOPSEASON_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "coopseason/rpc/coopseasonmaster_stub.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace CoopSeason
{

class CoopSeasonMasterImpl : public CoopSeasonMasterStub
{
public:
	CoopSeasonMasterImpl();
    ~CoopSeasonMasterImpl();

private:
	virtual uint16_t  		getDbSchemaVersion() const	{ return 2; }

    bool onConfigure();

	PokeMasterError::Error processPokeMaster(const CoopSeasonRequest &request, CoopSeasonResponse &response, const Message *message);

	uint32_t mDbId;

};

} // CoopSeason
} // Blaze

#endif  // BLAZE_COOPSEASON_MASTERIMPL_H
