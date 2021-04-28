/*************************************************************************************************/
/*!
    \file   fifagroupsmasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFAGROUPS_MASTERIMPL_H
#define BLAZE_FIFAGROUPS_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "fifagroups/rpc/fifagroupsmaster_stub.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace FifaGroups
{

class FifaGroupsMasterImpl : public FifaGroupsMasterStub
{
public:
    FifaGroupsMasterImpl();
    ~FifaGroupsMasterImpl();

private:
    bool onConfigure();
    PokeMasterError::Error processPokeMaster(const FifaGroupsRequest &request, FifaGroupsResponse &response, const Message *message);
};

} // FifaGroups
} // Blaze

#endif  // BLAZE_FIFAGROUPS_MASTERIMPL_H
