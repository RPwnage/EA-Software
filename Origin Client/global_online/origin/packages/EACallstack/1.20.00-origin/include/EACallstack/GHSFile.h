///////////////////////////////////////////////////////////////////////////////
// GHSFile.h
//
// Copyright (c) 2012 Electronic Arts Inc.
// Created by Paul Pedriana
//
// Implements reading of Green Hills ELF files, which have differing debug 
// information than DWARF-based ELF files.
///////////////////////////////////////////////////////////////////////////////


#ifndef EACALLSTACK_GHSFILE_H
#define EACALLSTACK_GHSFILE_H


#include <EACallstack/internal/Config.h>

#if EACALLSTACK_GREENHILLS_GHSFILE_ENABLED

#include <EACallstack/internal/EASTLCoreAllocator.h>
#include <EACallstack/EAAddressRep.h>
#include <coreallocator/icoreallocator_interface.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
#endif


namespace EA
{
    namespace Callstack
    {
        /// GHSFile
        ///
        class EACALLSTACK_API GHSFile : public IAddressRepLookup
        {
        public:
            /// kTypeId
            /// Defines what type of IAddressRepLookup this is. This is used in conjunction
            /// with the AsInterface function to safely dynamically cast a base class 
            /// such as IAddressRepLookup to a subclass such as GHSFile.
            static const int kTypeId = 0x061b672e; // This is just some unique value.

            GHSFile(EA::Allocator::ICoreAllocator* pCoreAllocator = NULL);
           ~GHSFile();

            void SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator);

            bool Init(const wchar_t* pGHSFilePath, bool bDelayLoad = true);
            bool Shutdown();

            void*           AsInterface(int typeId);
            void            SetBaseAddress(uint64_t baseAddress);
            uint64_t        GetBaseAddress() const;
            int             GetAddressRep(int addressRepTypeFlags, const void* pAddress, FixedString* pRepArray, int* pIntValueArray);
            int             GetAddressRep(int addressRepTypeFlags, uint64_t address, FixedString* pRepArray, int* pIntValueArray);
            const wchar_t*  GetDatabasePath() const;
            void            SetOption(int option, int value);

            // Functionality specific to this class
            void SetAddr2LinePath(const wchar_t* pAddr2LinePath); // Set the path to gaddr2line.exe. Normally found in {CAFE_SDK_ROOT}\ghs\multi539\gaddr2line.exe

        protected:
            int GetAddressRepSpawn(int addressRepTypeFlags, uint64_t nAddress, FixedString* pRepArray, int* pIntValueArray);

        protected:
            bool                           mbInitialized;       /// 
            EA::Allocator::ICoreAllocator* mpCoreAllocator;     /// 
            FixedStringW                   msGHSFilePath;       /// 
            FixedStringW                   msAddr2LinePath;     /// Path to gaddr2line.exe. We expect it to be in certain places; and don't provide it ourselves.
            bool                           mbOfflineLookup;     /// True if we are to do offline address lookups, instead of lookups for the current app's addresses. Affects how we try to auto-determine module base addresses, which is important for relocatable DLLs on all platforms.
            bool                           mbRelocated;         /// True if this module is a relocatable module like a DLL.
            uint64_t                       mBaseAddress;        /// Address that executable is offset to. Usually this is zero unless we are dealing with a dynamic library.

        }; // class GHSFile

    } // namespace Callstack

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // EACALLSTACK_GREENHILLS_GHSFILE_ENABLED


#endif // Header include guard.


