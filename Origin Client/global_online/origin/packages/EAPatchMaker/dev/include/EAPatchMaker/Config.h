///////////////////////////////////////////////////////////////////////////////
// Config.h
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHMAKER_CONFIG_H
#define EAPATCHMAKER_CONFIG_H


#include <EABase/eabase.h>
#include <EAPatchMaker/Version.h>


///////////////////////////////////////////////////////////////////////////////
// EAPATCHMAKER_ALLOC_PREFIX
//
// Defined as a string literal. Defaults to this package's name.
// Can be overridden by the user by predefining it or by editing this file.
// This define is used as the default name used by this package for naming
// memory allocations and memory allocators.
//
// All allocations names follow the same naming pattern:
//     <package>/<module>[/<specific usage>]
// 
// Example usage:
//     void* p = pCoreAllocator->Alloc(37, EAPATCHMAKER_ALLOC_PREFIX, 0);
//
// Example usage:
//     gMessageServer.GetMessageQueue().get_allocator().set_name(EAPATCHMAKER_ALLOC_PREFIX "MessageSystem/Queue");
//
#ifndef EAPATCHMAKER_ALLOC_PREFIX
    #define EAPATCHMAKER_ALLOC_PREFIX "EAPatchMaker/"
#endif



#endif // Header include guard



