/////////////////////////////////////////////////////////////////////////////
// main.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
// Written by Jacob Trakhtenberg.
/////////////////////////////////////////////////////////////////////////////

#include <test.h>
#include <EATDF/tdffactory.h>
#include <EABase/eabase.h>

#include <MemoryMan/MemoryMan.inl>
#include <MemoryMan/CoreAllocator.inl>

#include "gtest/gtest.h"

CoreAllocatorMalloc::CoreAllocatorMalloc()
{
}

CoreAllocatorMalloc::CoreAllocatorMalloc(const CoreAllocatorMalloc &)
{
}

CoreAllocatorMalloc::~CoreAllocatorMalloc()
{
}

CoreAllocatorMalloc& CoreAllocatorMalloc::operator=(const CoreAllocatorMalloc&)
{
    return *this;
}

void* CoreAllocatorMalloc::Alloc(size_t size, const char* /*name*/, unsigned /*flags*/)
{
    // NOTE: this 'new' gets forwarded to EAGeneralAllocatorDebug by the EATest harness
    char8_t* pMemory = new char8_t[size];
    return pMemory;
}

void* CoreAllocatorMalloc::Alloc(size_t size, const char* /*name*/, unsigned /*flags*/,
    unsigned /*align*/, unsigned /*alignOffset*/)
{
    // NOTE: this 'new' gets forwarded to EAGeneralAllocatorDebug by the EATest harness
    char8_t* pMemory = new char8_t[size];
    return pMemory;
}

void CoreAllocatorMalloc::Free(void* block, size_t /*size*/)
{
    // NOTE: this 'delete' gets forwarded to EAGeneralAllocatorDebug by the EATest harness
    delete[](char8_t*)block;
}


int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    EA::TDF::TdfFactory::fixupTypes();

    int retVal = RUN_ALL_TESTS();
    
    EA::TDF::TdfFactory::cleanupTypeAllocations();

    return retVal;
}








