///////////////////////////////////////////////////////////////////////////////
// InjectHook.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef INJECT_H
#define INJECT_H

#include "IGO.h"
namespace OriginIGO {

    #if defined(ORIGIN_PC)

    class InjectHook
    {
    private:
        static bool mIsQuitting;
    public:
        static bool TryHook();
        static void Cleanup(bool forcedUnload = false);
        static bool IsQuitting();
    };

    bool InjectCurrentDLL(HANDLE hProcess, HANDLE hThread = NULL);

    #endif
}
#endif
