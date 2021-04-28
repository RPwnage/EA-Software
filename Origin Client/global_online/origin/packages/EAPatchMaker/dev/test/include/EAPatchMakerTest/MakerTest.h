/////////////////////////////////////////////////////////////////////////////
// EAPatchMakerTest/MakerTest.h
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHMAKER_EAPATCHMAKERTEST_MAKERTEST_H
#define EAPATCHMAKER_EAPATCHMAKERTEST_MAKERTEST_H


#include <EAPatchClient/Config.h>
#include <EAIO/PathString.h>
#include <EASTL/list.h>


extern EA::IO::Path::PathStringW gDataDirectory;
extern EA::IO::Path::PathStringW gTempDirectory;

typedef eastl::list<EA::IO::Path::PathStringW> PathList;
extern PathList gPatchDirectoryList;

void ValidateHeap();


#endif // Header include guard
