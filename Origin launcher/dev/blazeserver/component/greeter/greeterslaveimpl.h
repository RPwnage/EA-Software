/*************************************************************************************************/
/*!
    \file   greeterslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef STUDIO_TITLE_GREETER_V1ALPHA_GREETERSLAVEIMPL_H
#define STUDIO_TITLE_GREETER_V1ALPHA_GREETERSLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "greeter/tdf/greeter.h"
#include "greeter/rpc/greeterslave_stub.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace studio
{
namespace title
{
namespace greeter
{
namespace v1alpha
{

class GreeterSlaveImpl : public GreeterSlaveStub
{
public:
    GreeterSlaveImpl() {}
    virtual ~GreeterSlaveImpl() {}

private:
    bool onConfigure() override;
    bool onReconfigure() override;

public:
    void obfuscatePlatformInfo(Blaze::ClientPlatformType platform, EA::TDF::Tdf& tdf) const override;

}; //class GreeterSlaveImpl

} // namespace v1alpha
} // namespace greeter
} // namespace title
} // namespace studio

#endif // STUDIO_TITLE_GREETER_V1ALPHA_GREETERSLAVEIMPL_H
