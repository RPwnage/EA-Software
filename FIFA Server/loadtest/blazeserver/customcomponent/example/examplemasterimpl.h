/*************************************************************************************************/
/*!
    \file   examplemasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_EXAMPLE_MASTERIMPL_H
#define BLAZE_EXAMPLE_MASTERIMPL_H

/*** Include files *******************************************************************************/
#include "example/rpc/examplemaster_stub.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Example
{

class ExampleMasterImpl : public ExampleMasterStub
{
public:
    ExampleMasterImpl();
    ~ExampleMasterImpl() override;

private:
    bool onConfigure() override;

    PokeMasterError::Error processPokeMaster(const ExampleRequest &request, ExampleResponse &response, const Message *message) override;
    RpcPassthroughMasterError::Error processRpcPassthroughMaster(const Message* message) override;

};

} // Example
} // Blaze

#endif  // BLAZE_EXAMPLE_MASTERIMPL_H
