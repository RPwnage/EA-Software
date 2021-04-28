/////////////////////////////////////////////////////////////////////////////
// main.cpp
//
// Copyright (c) 2018, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//
//
// This program just depends on the Framework (and proxy components which Framework itself depends on).
// The idea is that if someone adds some code in the Framework that is depending on the components(say Gamemanager), this program will fail to build.
// So if you are seeing build error with this program, fix your dependencies (Framework and Proxy Components should not depend on the other components(core/custom). 
// 
// Rationale: 
// We want to ensure that teams running a custom cluster without any core components can upgrade their Blaze Framework version any time and they don't have to 
// pull in core components in the process. It also makes general sense that Framework does not depend on any core components. 
// 
// Framework itself accesses external REST based services using Proxy Components. So we ensure that Proxy components do not need component code either. This is fine
// because almost all the time, such usage is a layering mistake (after all, the proxy component is outlining an external interface beyond Blaze's control and it'd
// be a nice coincidence if you happen to have a data structure that just 'fits'). 
// The existing violations in proxy components were of trivial nature so while this change might break some custom proxy components, fixes should be of very trivial
// nature (and in turn, keep the layering clean).
//////////////////////////////////////////////////////////////////////////////



#include <EABase/eabase.h>
#include "framework/blaze.h"

// Empty stubs to satisfy the linker for quirkiness around Component DB.
namespace Blaze
{

void BlazeRpcComponentDb::initialize(uint16_t componentId)
{
        
}

size_t BlazeRpcComponentDb::getTotalComponentCount()
{
    return 0;
}

size_t BlazeRpcComponentDb::getComponentCategoryIndex(ComponentId componentId)
{
    return 0;
}

size_t BlazeRpcComponentDb::getComponentAllocGroups(size_t componentCategoryIndex)
{
    return 0;
}

} // Blaze

int main(int argc, char **argv)
{
    return 0;
}






