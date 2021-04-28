///////////////////////////////////////////////////////////////////////////////
// EACallstackChoose.cpp
//
// Copyright (c) 2003-2005 Electronic Arts Inc.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <eathread/internal/config.h>
#include <EABase/eabase.h>

#if defined(EA_PLATFORM_APPLE)
    #include "../Apple/GetModuleInfoApple.cpp"
#endif

// The implementation of the callstack fetch code and callstack context code 
// has been moved to EAThread past version 1.17.02.
// TODO: Remove old callstack implementation code by June, 2014.
#if EATHREAD_VERSION_N <= 11702

    // The purpose of this source file is to automatically compile the right callstack code 
    // implementation in the case that the build system is incapable of knowing which one to
    // compile. This can arise because the conditions below can be complicated and new platforms
    // may have CPUs that the build system doesn't explicitly recognize but which the code
    // below can recognize.

    #include <EACallstack/EACallstack.h>


    #if defined(EA_PLATFORM_XENON)
        #include "../Xenon/EACallstack_Xenon.cpp"
    #elif defined(EA_PLATFORM_MICROSOFT) && defined(EA_PROCESSOR_X86) && !defined(__CYGWIN__)
        #include "../Win32/EACallstack_Win32.cpp"
    #elif defined(EA_PLATFORM_MICROSOFT) && defined(EA_PROCESSOR_X86_64) && !defined(__CYGWIN__)
        #include "../Win64/EACallstack_Win64.cpp"
    #elif defined(EA_PLATFORM_PLAYSTATION2)
        #include "../PS2/EACallstack_PS2.cpp"
    #elif defined(EA_PLATFORM_PS3_SPU)
        #include "../PS3/EACallstack_PS3_SPU.cpp"
    #elif defined(EA_PLATFORM_PS3)
        #include "../PS3/EACallstack_PS3.cpp"
    #elif defined(EA_PLATFORM_GAMECUBE) || defined(EA_PLATFORM_REVOLUTION)
        #include "../GameCube/EACallstack_GameCube.cpp"
    #elif defined(EA_PLATFORM_CAFE)
        #include "../Cafe/EACallstack_Cafe.cpp"
    #elif defined(EA_PLATFORM_APPLE) // OSX, iPhone, iPhone Simulator
        #include "../Apple/EACallstack_Apple.cpp"
    #elif defined(EA_PROCESSOR_ARM)
        #include "../arm/EACallstack_arm.cpp"
    #elif (defined(EA_PLATFORM_LINUX) || defined(__CYGWIN__)) && (defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_X86_64))
        #include "../x86/EACallstack_x86.cpp"
    #elif defined(__GNUC__) || defined(EA_COMPILER_CLANG)
        #include "../glibc/EACallstack_glibc.cpp"
    #endif


#endif





