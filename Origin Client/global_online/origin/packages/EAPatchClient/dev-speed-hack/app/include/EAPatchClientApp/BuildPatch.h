/////////////////////////////////////////////////////////////////////////////
// EAPatchClient/BuildPatch.h
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include <EABase/eabase.h>


// Builds a patch, given a URL to a .eaPatchInfo file and a destination directory to patch.
// If pPatchLocalDirPath is non-NULL then it's assumed that pPatchLocalDirPath will be patched
// but the results will be written to pPatchDestinationDirPath.
bool BuildPatchFromInfoURL(const char8_t* pPatchInfoURL, const char8_t* pPatchDestinationDirPath, const char8_t* pPatchLocalDirPath = NULL);

// Builds a patch, given a file path to a .eaPatchInfo file and a destination directory to patch.
// If pPatchLocalDirPath is non-NULL then it's assumed that pPatchLocalDirPath will be patched
// but the results will be written to pPatchDestinationDirPath.
bool BuildPatchFromInfoFile(const char8_t* pPatchInfoFilePath, const char8_t* pPatchDestinationDirPath, const char8_t* pPatchLocalDirPath = NULL);

// Builds a patch, given a file path to a .eaPatchImpl file and a destination directory to patch.
// If pPatchLocalDirPath is non-NULL then it's assumed that pPatchLocalDirPath will be patched
// but the results will be written to pPatchDestinationDirPath.
bool BuildPatchFromImplURL(const char8_t* pPatchImplFilePath, const char8_t* pPatchDestinationDirPath, const char8_t* pPatchLocalDirPath = NULL);


// Cancels a patch which is partially built. Rolls back the patch directory 
// to its pre-patch state.
bool CancelPatch(const char8_t* pPatchDestinationDirPath);



