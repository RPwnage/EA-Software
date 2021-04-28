/*************************************************************************************************/
/*!
    \file frameworkresource.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/system/frameworkresource.h"
#include "framework/system/fiber.h"

#include "framework/component/blazerpc.h"

namespace Blaze
{


void FrameworkResource::release()
{
    // remove resource from owning fiber's list.
    clearOwningFiber();
    // invoke custom release code for framework resource.  this may delete our instance, so execute last.
    releaseInternal();          
}

void FrameworkResource::assign()
{
    //  first execute customized behavior
    assignInternal();

    clearOwningFiber();

    //  attach resource to current fiber
    if (Fiber::isCurrentlyBlockable())
    {
        Fiber::attachResourceToCurrentFiber(*this);
    }
}

void FrameworkResource::clearOwningFiber()
{
    if (mOwningFiber != nullptr)
    {
        // Remove the handle from the fiber's list since the conn has explicitly been released now
        mOwningFiber->detachResource(*this);
        // Clear out the owning fiber since resource is no longer attached to it
        mOwningFiber = nullptr;
    }
}

} // Blaze



