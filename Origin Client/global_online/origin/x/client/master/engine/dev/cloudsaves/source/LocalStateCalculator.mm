// LocalStateCalculator.mm
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.

#include "engine/cloudsaves/LocalStateCalculator.h"

#import <Foundation/Foundation.h>

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    LocalAutoreleasePool::LocalAutoreleasePool()
    {
        pool = [[NSAutoreleasePool alloc] init];
    }
    
    LocalAutoreleasePool::~LocalAutoreleasePool()
    {
        NSAutoreleasePool* _pool = reinterpret_cast<NSAutoreleasePool*>(pool);
        [_pool release];
    }
}
}
}
