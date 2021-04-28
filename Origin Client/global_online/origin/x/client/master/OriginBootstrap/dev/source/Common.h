///////////////////////////////////////////////////////////////////////////////
// Common.h
// 
// Copyright (c) 2014 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef COMMON_H
#define COMMON_H

#include <windows.h>

enum ClientStartupState
{
    kClientStartupNormal,
    kClientStartupMinimized,
    kClientStartupMaximized,
};

extern ClientStartupState gClientStartupState;

extern bool gIsDownloadSuccessful;
extern bool gIsOkWinHttpReadData;
extern bool gIsUpToDate;

extern int argc;
extern char** argv;

extern HWND hDownloadDialog;
extern HWND hUnpackDialog;

#endif //COMMON_H
