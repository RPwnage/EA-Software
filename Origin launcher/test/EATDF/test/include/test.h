/////////////////////////////////////////////////////////////////////////////
// test.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
// Written by Jacob Trakhtenberg.
/////////////////////////////////////////////////////////////////////////////

#ifndef EA_TDF_TEST_TEST_H
#define EA_TDF_TEST_TEST_H


#include <EABase/eabase.h>
#include "gtest/gtest.h"
#include <coreallocator/icoreallocator_interface.h>


///////////////////////////////////////////////////////////////////////////////
// CoreAllocatorMalloc
//
class CoreAllocatorMalloc : public EA::Allocator::ICoreAllocator
{
public:
    CoreAllocatorMalloc();
    CoreAllocatorMalloc(const CoreAllocatorMalloc&);
    ~CoreAllocatorMalloc();
    CoreAllocatorMalloc& operator=(const CoreAllocatorMalloc&);

    void* Alloc(size_t size, const char* /*name*/, unsigned /*flags*/);
    void* Alloc(size_t size, const char* /*name*/, unsigned /*flags*/,
        unsigned /*align*/, unsigned /*alignOffset*/);
    void Free(void* block, size_t /*size*/);
};

#endif

/*
Use TEST_ALL_TDFS to test all the tdfs in alltypes.tdf against a set of templated functions of the form

template <typename T>
int someTest() { return 0; }

Tests will be named after each function and the tdf its testing.  You can supply a different function for tdfs
unions.

*/

#define DECL_TEST_FUNC(TESTFUNC, NAME) \
    TEST(TESTFUNC, NAME) { ASSERT_EQ(0, TESTFUNC<NAME>()); } 

#define TEST_ALL_TDFS(_tdfTestName, _unionTestName) \
    DECL_TEST_FUNC(_tdfTestName, AllPrimitivesClass); \
    DECL_TEST_FUNC(_tdfTestName, AllComplexClass); \
    DECL_TEST_FUNC(_unionTestName, AllPrimitivesUnion); \
    DECL_TEST_FUNC(_unionTestName, AllPrimitivesUnionAllocInPlace); \
    DECL_TEST_FUNC(_unionTestName, AllComplexUnion); \
    DECL_TEST_FUNC(_unionTestName, AllComplexUnionAllocInPlace); \
    DECL_TEST_FUNC(_tdfTestName, AllListsPrimitivesClass); \
    DECL_TEST_FUNC(_tdfTestName, AllListsComplexClass); \
    DECL_TEST_FUNC(_unionTestName, AllListsPrimitivesUnion); \
    DECL_TEST_FUNC(_unionTestName, AllListsPrimitivesUnionAllocInPlace); \
    DECL_TEST_FUNC(_unionTestName, AllListsComplexUnion); \
    DECL_TEST_FUNC(_unionTestName, AllListsComplexUnionAllocInPlace); \
    DECL_TEST_FUNC(_tdfTestName, AllMapsPrimitivesClass); \
    DECL_TEST_FUNC(_tdfTestName, AllMapsComplexClass); \
    DECL_TEST_FUNC(_unionTestName, AllMapsPrimitivesUnion); \
    DECL_TEST_FUNC(_unionTestName, AllMapsPrimitivesUnionAllocInPlace); \
    DECL_TEST_FUNC(_unionTestName, AllMapsComplexUnion); \
    DECL_TEST_FUNC(_unionTestName, AllMapsComplexUnionAllocInPlace); 

