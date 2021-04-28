/*************************************************************************************************/
/*!
    \file   greeterslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GreeterSlaveImpl

    Implements the slave portion of the greeter component.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "greeter/greeterslaveimpl.h"

namespace studio
{
namespace title
{
namespace greeter
{
namespace v1alpha
{

/*** Public Methods ******************************************************************************/
// static
GreeterSlave* GreeterSlave::createImpl()
{
    return BLAZE_NEW_NAMED("GreeterSlaveImpl") GreeterSlaveImpl();
}

/*************************************************************************************************/
/*!
    \brief ~GreeterSlaveImpl

    Destroys the slave greeter component.
*/
/*************************************************************************************************/


bool GreeterSlaveImpl::onConfigure()
{
    return true;
}

bool GreeterSlaveImpl::onReconfigure()
{
    return onConfigure();
}

void GreeterSlaveImpl::obfuscatePlatformInfo(Blaze::ClientPlatformType platform, EA::TDF::Tdf& tdf) const
{
    if (tdf.getTdfId() == GreetResponse::TDF_ID)
    {
        EA::TDF::TdfString& greeting = static_cast<GreetResponse*>(&tdf)->getGreetingAsTdfString();
        eastl::string currentGreeting = greeting.c_str();
        currentGreeting.append_sprintf(" GreeterSlaveImpl::obfuscatePlatformInfo has been invoked on this response -- the caller is on platform '%s'.", Blaze::ClientPlatformTypeToString(platform));
        greeting.set(currentGreeting.c_str(), currentGreeting.length());
    }
}

} // namespace v1alpha
} // namespace greeter
} // namespace title
} // namespace studio
