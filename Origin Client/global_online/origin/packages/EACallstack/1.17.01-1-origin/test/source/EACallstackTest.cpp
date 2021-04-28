/////////////////////////////////////////////////////////////////////////////
// EACallstackTest.cpp
//
// Copyright (c) 2004, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>
#include <EACallstack/internal/Config.h>
#include <EACallstack/EACallstack.h>
#include <EACallstack/Allocator.h>
#include <EACallstack/EAAddressRep.h>
#include <EACallstack/MapFile.h>
#include <EACallstack/DWARF2File.h>
#include <EACallstack/PDBFile.h>
#include <EACallstack/EASymbolUtil.h>
#include <EACallstackTest/EACallstackTest.h>
#include <eathread/eathread.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EATextUtil.h>
#include <EAStdC/EAProcess.h>
#include <EATest/EATest.h>
#include <EATest/EATestMain.inl>
#include <EAIO/FnEncode.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/Allocator.h>
#include <EAIO/PathString.h>
#include <EAIO/EAFileDirectory.h>
#include <PPMalloc/EAGeneralAllocator.h>
#include <PPMalloc/EAGeneralAllocatorDebug.h>
#include <MemoryMan/CoreAllocatorGeneral.h>
#include <EAAssert/eaassert.h>

//Prevents false positive memory leaks on GCC platforms (such as PS3)
#if defined(EA_COMPILER_GNUC)
    #define EA_MEMORY_GCC_USE_FINALIZE
#endif
#include <MemoryMan/MemoryMan.inl>

#include <MemoryMan/CoreAllocator.inl>
#if defined(EA_PLATFORM_MICROSOFT) && !defined(EA_PLATFORM_XENON)
    #pragma warning(push, 0)
    #include <Windows.h>
    #pragma warning(pop)
#endif


#ifdef _MSC_VER
    // Disable inlining. This is significant for the testing of callstack generation and traversal.
    #pragma inline_depth(0)
    #pragma warning(disable: 4740) // flow in or out of inline asm code suppresses global optimization
#endif


#ifdef _MSC_VER
    /// DLLFunctionDecl
    ///
    /// This is a macro which implements the dynamic linking to a function in a DLL.
    /// It takes care of the boilerplate code that you need to do under Windows for
    /// this, such as LoadLibrary, GetProcedureAddress, etc.
    ///
    /// Example usage:
    ///    #include <Windows.h>
    ///
    ///    typedef WINBASEAPI VOID WINAPI (*OutputDebugStringType)(LPSTR);
    ///    DLLFunctionDecl("kernel32.dll", OutputDebugStringA, OutputDebugStringType);
    ///    
    ///    void DoSomething() {
    ///        if(pOutputDebugStringA)
    ///            pOutputDebugStringA()("hello world");
    ///    }
    ///

    #ifndef DLLFunctionDecl
        #define DLLFunctionDecl(dllName, functionName, functionType); \
        \
        namespace { \
            class DLL_##functionName \
            { \
            public: \
                DLL_##functionName() \
                { \
                    mhModule = LoadLibraryA(dllName); \
                    if(mhModule) \
                        mpFunction = (functionType)(void*)GetProcAddress(mhModule, #functionName); \
                    else \
                        mpFunction = NULL; \
                } \
                \
               ~DLL_##functionName() \
                    { FreeLibrary(mhModule); } \
                \
                functionType operator()()const \
                    { return mpFunction; } \
                \
                typedef functionType (DLL_##functionName::*bool_)() const; \
                operator bool_() const \
                { \
                    if(mpFunction) \
                        return &DLL_##functionName::operator(); \
                    return 0; \
                } \
                \
            protected: \
                HMODULE      mhModule; \
                functionType mpFunction; \
            }; \
            \
            DLL_##functionName p##functionName;\
            \
        }
    #endif
#endif


static intptr_t Abs(intptr_t x)
{
    return (x < 0 ? -x : x);
}


// Global variables
void*                        gpAddress = (void*)-1;
CustomCoreAllocator          gCustomCoreAllocator;
eastl::list<eastl::wstring>  gTestFileList;
const char*                  gpApplicationPath = NULL;
bool                         gbMissingDBReportedAlready = false;


///////////////////////////////////////////////////////////////////////////////
// StreamLog
///////////////////////////////////////////////////////////////////////////////

namespace EA
{
    namespace IO
    {
        StreamLog::StreamLog()
          : mnRefCount(0)
        {
        }

        int StreamLog::AddRef()
        {
            return ++mnRefCount;
        }

        int StreamLog::Release()
        {
            if(mnRefCount > 1)
                return --mnRefCount;
            delete this;
            return 0;
        }

        bool StreamLog::Write(const void* pData, size_type nSize)
        {
            mLine.assign((const char*)pData, (eastl_size_t)nSize);
            EA::UnitTest::Report("%s", mLine.c_str());
            return true;
        }

    } // namespace IO
} // namespace EA


///////////////////////////////////////////////////////////////////////////////
// Required by EASTL
///////////////////////////////////////////////////////////////////////////////

int Vsnprintf8(char8_t* pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}

int Vsnprintf16(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}

#if (EASTDC_VERSION_N >= 10600)
    int Vsnprintf32(char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments)
    {
        return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
    }
#endif



///////////////////////////////////////////////////////////////////////////////
// VerifyCallstack
//
void VerifyCallstack(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount)
{
    using namespace EA::Callstack;

    EA::UnitTest::ReportVerbosity(1, "\nCallstack test dump:\n");

    void*  pCallstack[32];        // The following are intentionally on the same line of code in order to facilitate testing.
    size_t nCallstackDepth = GetCallstack(pCallstack, 32, NULL); EA::UnitTest::NonInlinableFunction(); int nFirstCallstackLineNumber = __LINE__; // Get the line where GetCallstack is called.

    CallstackEntryInfo ei(pCallstack[0], __FUNCTION__, -1, __FILE__, nFirstCallstackLineNumber, NULL); // Add an entry into the reference stack for this function.
    expectedEntries.insert(expectedEntries.begin(), ei);

    CallstackEntryInfoArray reportedEntries;

    // Fill in reportedEntries
    for(eastl_size_t i = 0; (i < nCallstackDepth) && (i < expectedEntries.size()); ++i)
    {
        const char* pStrResults[kARTCount];
        int         pIntResults[kARTCount];
        int         nAddressRepFlags     = (kARTFlagFunctionOffset | kARTFlagFileLine | kARTFlagSource | kARTFlagAddress);
        int         nAddressRepReturnVal = arc.GetAddressRep(nAddressRepFlags, (uint64_t)(uintptr_t)pCallstack[i], pStrResults, pIntResults);

        // GetAddressRep should always return some info in this environment. Verify that it does.
        EATEST_VERIFY(nAddressRepReturnVal != 0);

        ei.Clear();
        ei.mpAddress = pCallstack[i]; 

        if(nAddressRepReturnVal & kARTFlagFunctionOffset)
        {
            ei.msFunction       = pStrResults[kARTFunctionOffset];
            ei.mnFunctionOffset = pIntResults[kARTFunctionOffset];
        }

        if(nAddressRepReturnVal & kARTFlagFileLine) // Give the actual expected line result a little slack, due to the way callstack reporting works.
        {
            ei.msFile     = pStrResults[kARTFileLine];
            ei.mnFileLine = pIntResults[kARTFileLine];
        }

        // Verify that the source result is at least non-NULL. This will fail if this build doesn't have debug symbols available.
        if(nAddressRepReturnVal & kARTFlagSource)
            EATEST_VERIFY(pStrResults[kARTSource] != NULL);

        reportedEntries.push_back(ei);
    }

    // Verify each entry in the reference call stack against the one returned by EACallstack.
    // We do so by comparing expectedEntries with reportedEntries.
    bool bSuccess = true;

    for(eastl_size_t i = 0; (i < reportedEntries.size()) && bSuccess && (nErrorCount == 0); ++i)
    {
        const CallstackEntryInfo& entryInfoReported = reportedEntries[i];
        const CallstackEntryInfo& entryInfoExpected = expectedEntries[i];

        // Test the instruction address.
        // Due to the nature of optimized machine code, the expected and reported addresses could be 
        // fairly far from each other in optimized builds. Even one C++ line could have machine instruction
        // representations hundreds of bytes apart and which aren't necessarily contiguous.
        const intptr_t addressExpected = (intptr_t)entryInfoExpected.mpAddress;
        const intptr_t addressReported = (intptr_t)entryInfoReported.mpAddress;
        const intptr_t addressDiff     = Abs(addressExpected - addressReported);

        #if defined(EA_PLATFORM_CAFE)
        const intptr_t kAddressDiffMax = 1200; // To do: Verify that the Cafe compiler really is producing this. A quick glance and the unit test debug trace seems to indicate so.
        #else
        const intptr_t kAddressDiffMax = 700; // This is a very large value, but in practice we haven't been able to make it much smaller, due to the way compilers generate code.
        #endif

        // Make sure the address difference isn't too large. But if the file line matches then all is good despite the addresses being far off.
        EATEST_VERIFY((addressDiff < kAddressDiffMax) || ((entryInfoExpected.mnFileLine > 1) && (abs(entryInfoExpected.mnFileLine - entryInfoReported.mnFileLine) < 5))); 
        if(!((addressDiff < kAddressDiffMax) || ((entryInfoExpected.mnFileLine > 1) && (abs(entryInfoExpected.mnFileLine - entryInfoReported.mnFileLine) < 5))))
        {
            bSuccess = false;
            EA::UnitTest::Report("Address error at callstack depth %d.\n    Expected: %p; reported: %p; diff of %I64d\n", i, entryInfoExpected.mpAddress, entryInfoReported.mpAddress, (int64_t)addressDiff);
        }

        // Test the function name.
        #if EACALLSTACK_NATIVE_FUNCTION_LOOKUP_AVAILABLE
            if(arc.GetAddressRepLookupSet().GetDatabaseFileCount())
            {
                // This could fail if the processor, its ABI, and/or the current compiler settings don't 
                // allow for reading stack frames. Under VC++ for x86, for example, it's important that this
                // test be compiled with stack frames enabled in optimized builds, stack reading fail.
                const char8_t* funcExpected = entryInfoExpected.msFunction.c_str();
                const char8_t* funcReported = entryInfoReported.msFunction.c_str();

                if(EA::StdC::Strstr(funcReported, funcExpected) == 0)
                {
                    #if defined(EA_COMPILER_CLANG)
                        // Clang tends to inconsistently omit stack frame information for builds in a way that makes 
                        // it hard for us to consistently unit test this. Typically, the failure here is that 
                        // funcReported is some parent of funcExpected. The GetCallstack function is doing the best
                        // it can, but the compiler is getting in the way. Perhaps we can have this code assert that
                        // at least funcReported is a recent parent.
                    #else
                        bSuccess = false;
                        EA::UnitTest::Report("Function error at callstack depth %d.\n    Expected: %s; reported: %s\n", i, funcExpected, funcReported);
                    #endif
                }
            }
            else
            {
                if(!gbMissingDBReportedAlready)
                {
                    gbMissingDBReportedAlready = true;
                    EA::UnitTest::Report("No DB (e.g. map) files found. Skipping symbol lookup tests.\n");
                }
            }
        #endif

        // Test the file/line file name.
        #if EACALLSTACK_NATIVE_FILELINE_LOOKUP_AVAILABLE
            if(arc.GetAddressRepLookupSet().GetDatabaseFileCount())
            {
                // Since fileExpected is derived from __FILE__ and fileReported is derived from the debug information, 
                // they might have a different string (e.g. case difference, relative vs. absolute path), yet refer to 
                // the same file. We compare just the file name in a case-insensitive way. That's probably a good enough test.
                const char8_t* fileExpected = EA::IO::Path::GetFileName(entryInfoExpected.msFile.c_str());
                const char8_t* fileReported = EA::IO::Path::GetFileName(entryInfoReported.msFile.c_str());

                EATEST_VERIFY(EA::StdC::Stricmp(fileExpected, fileReported) == 0);
                if(EA::StdC::Stricmp(fileExpected, fileReported) != 0)
                {
                    bSuccess = false;
                    EA::UnitTest::Report("File error at callstack depth %d.\n    Expected: %s; reported: %s\n", i, fileExpected, fileReported);
                }
            }
            else
            {
                if(!gbMissingDBReportedAlready)
                {
                    gbMissingDBReportedAlready = true;
                    EA::UnitTest::Report("No DB (e.g. map) files found. Skipping symbol lookup tests.\n");
                }
            }
        #endif

        // Test the file/line line.
        #if EACALLSTACK_NATIVE_FILELINE_LOOKUP_AVAILABLE
            if(arc.GetAddressRepLookupSet().GetDatabaseFileCount())
            {
                // The following can fail if any code that calls GetCallstack doesn't have the next line of 
                // executed code on the same or next line in this source file. And in optimized builds that 
                // next line of code might be optimized away. So we could conceivably have a hard time guaranteeing 
                // this test assertion succeeds.
                const int lineExpected = entryInfoExpected.mnFileLine;
                const int lineReported = entryInfoReported.mnFileLine;

                EATEST_VERIFY(abs(lineExpected - lineReported) < 5); 
                if(abs(lineExpected - lineReported) >= 5)
                {
                    bSuccess = false;
                    EA::UnitTest::Report("Line error at callstack depth %d.\n    Expected: %d; reported: %d\n", i, lineExpected, lineReported);
                }
            }
            else
            {
                if(!gbMissingDBReportedAlready)
                {
                    gbMissingDBReportedAlready = true;
                    EA::UnitTest::Report("No DB (e.g. map) files found. Skipping symbol lookup tests.\n");
                }
            }
        #endif

        // We don't test the source code, as we don't have a reference to compare to.
    }

    if(!bSuccess || (EA::UnitTest::GetVerbosity() > 0)) // If there was a failure or if the report verbosity is high enough...
    {
        EA::UnitTest::Report("Expected callstack info:\n");

        for(eastl_size_t i = 0; i < expectedEntries.size(); ++i)
        {
            const CallstackEntryInfo& entryInfo = expectedEntries[i];

            FixedString sAddress;
            AddressToString(entryInfo.mpAddress, sAddress, true);

            EA::UnitTest::Report("   %2d: Info for address %s\n", i, sAddress.c_str());
            EA::UnitTest::Report("       Func:\t%s\n",      entryInfo.msFunction.c_str());
            EA::UnitTest::Report("       Offset:\t%d\n",    entryInfo.mnFunctionOffset);
            EA::UnitTest::Report("       File:\t%s\n",      entryInfo.msFile.c_str());
            EA::UnitTest::Report("       Line:\t%d\n",      entryInfo.mnFileLine);
            EA::UnitTest::Report("       Source:\n%s\n\n",  entryInfo.msSource.c_str());
        }

        EA::UnitTest::Report("Reported callstack info:\n");

        for(eastl_size_t i = 0; i < reportedEntries.size(); ++i)
        {
            const CallstackEntryInfo& entryInfo = reportedEntries[i];

            FixedString sAddress;
            AddressToString(entryInfo.mpAddress, sAddress, true);

            EA::UnitTest::Report("   %2d: Info for address %s\n", i, sAddress.c_str());
            EA::UnitTest::Report("       Func:\t%s\n",      entryInfo.msFunction.c_str());
            EA::UnitTest::Report("       Offset:\t%d\n",    entryInfo.mnFunctionOffset);
            EA::UnitTest::Report("       File:\t%s\n",      entryInfo.msFile.c_str());
            EA::UnitTest::Report("       Line:\t%d\n",      entryInfo.mnFileLine);
            EA::UnitTest::Report("       Source:\n%s\n\n",  entryInfo.msSource.c_str());
        }
    }

    // Test EAGetInstructionPointer.
    void*  pAddress;
    EAGetInstructionPointer(pAddress);

    // Record what we get the first time for the instruction pointer address.
    if(gpAddress == (void*)-1)
        gpAddress = pAddress;

    // Verify that we get the same instruction pointer address each time.
    EATEST_VERIFY(gpAddress == pAddress);

    expectedEntries.erase(expectedEntries.begin()); // Remove the entry for this function but leave the others.
}


#if EA_CALLSTACK_GETCALLSTACK_SUPPORTED


    /// EACALLSTACK_TEST_GETCALLSTACK
    ///
    /// Defined as 0 or 1.
    /// If 1 then we test GetCallstack, else skip the test.
    ///
    #if !defined(EACALLSTACK_TEST_GETCALLSTACK)
        #if defined(EA_COMPILER_CLANG) && !defined(EA_DEBUG)
            // clang opt builds are removing TestCallstackXX and making it hard to unit test.
            // To do: Work around the clang code generator to allow this to be tested.
            #define EACALLSTACK_TEST_GETCALLSTACK 0
        #else
            #define EACALLSTACK_TEST_GETCALLSTACK 1
        #endif
    #endif


    #if EACALLSTACK_TEST_GETCALLSTACK

        /// EACALLSTACK_TEST_USE_FUNCTION_POINTERS
        ///
        /// Defined as 0 or 1.
        /// If 1 then TestCallstack uses functions called through function pointers as opposed to 
        /// direct functions. The reason we have these is that with the way some compilers work
        /// it's hard to construct reliable unit tests in the presence of compiler optimizations.
        ///
        #if (defined(EA_COMPILER_MSVC) && defined(EA_PROCESSOR_POWERPC))
            #define EACALLSTACK_TEST_USE_FUNCTION_POINTERS 0
        #else
            #define EACALLSTACK_TEST_USE_FUNCTION_POINTERS 1
        #endif


        #if EACALLSTACK_TEST_USE_FUNCTION_POINTERS
            // We make an array of function pointers in order to prevent the compiler 
            // from doing tricks to avoid stack frames from being generated for the  
            // above test functions (which optimizing compilers were in fact doing.
            // We might have to make this a little more obscure if the compiler  
            // optimization sees through what we are doing. 
            typedef void (*TestFunctionType)(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount);
            TestFunctionType gTestFunctionArray[5] = { NULL, NULL, NULL, NULL, NULL };

            #define TestCallstack01Impl gTestFunctionArray[0]
            #define TestCallstack02Impl gTestFunctionArray[1]
            #define TestCallstack03Impl gTestFunctionArray[2]
            #define TestCallstack04Impl gTestFunctionArray[3]
            #define TestCallstack05Impl gTestFunctionArray[4]
        #else
            #define TestCallstack01Impl TestCallstack01
            #define TestCallstack02Impl TestCallstack02
            #define TestCallstack03Impl TestCallstack03
            #define TestCallstack04Impl TestCallstack04
            #define TestCallstack05Impl TestCallstack05
        #endif


        EA_NO_INLINE EACALLSTACK_TEST_FUNCTION_LINKAGE void TestCallstack05(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount)
        {
            void* pAddress;
            EAGetInstructionPointer(pAddress);  // To do: Put more code within this function to differentiate it from the others.
            CallstackEntryInfo ei(pAddress, __FUNCTION__, -1, __FILE__, __LINE__, NULL); expectedEntries.insert(expectedEntries.begin(), ei); VerifyCallstack(arc, expectedEntries, nErrorCount); EA::UnitTest::NonInlinableFunction();
        }

        EA_NO_INLINE EACALLSTACK_TEST_FUNCTION_LINKAGE void TestCallstack04(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount)
        {
            void* pAddress;
            EAGetInstructionPointer(pAddress);  // To do: Put more code within this function to differentiate it from the others.
            CallstackEntryInfo ei(pAddress, __FUNCTION__, -1, __FILE__, __LINE__, NULL); expectedEntries.insert(expectedEntries.begin(), ei); VerifyCallstack(arc, expectedEntries, nErrorCount); TestCallstack05Impl(arc, expectedEntries, nErrorCount); EA::UnitTest::NonInlinableFunction();
        }

        EA_NO_INLINE EACALLSTACK_TEST_FUNCTION_LINKAGE void TestCallstack03(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount)
        {
            void* pAddress;
            EAGetInstructionPointer(pAddress);  // To do: Put more code within this function to differentiate it from the others.
            CallstackEntryInfo ei(pAddress, __FUNCTION__, -1, __FILE__, __LINE__, NULL); expectedEntries.insert(expectedEntries.begin(), ei); VerifyCallstack(arc, expectedEntries, nErrorCount); TestCallstack04Impl(arc, expectedEntries, nErrorCount); EA::UnitTest::NonInlinableFunction();
        }

        EA_NO_INLINE EACALLSTACK_TEST_FUNCTION_LINKAGE void TestCallstack02(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount)
        {
            void* pAddress;
            EAGetInstructionPointer(pAddress);  // To do: Put more code within this function to differentiate it from the others.
            CallstackEntryInfo ei(pAddress, __FUNCTION__, -1, __FILE__, __LINE__, NULL); expectedEntries.insert(expectedEntries.begin(), ei); VerifyCallstack(arc, expectedEntries, nErrorCount); TestCallstack03Impl(arc, expectedEntries, nErrorCount); EA::UnitTest::NonInlinableFunction();
        }

        EA_NO_INLINE EACALLSTACK_TEST_FUNCTION_LINKAGE void TestCallstack01(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount)
        {
            void* pAddress;
            EAGetInstructionPointer(pAddress);   // The code below is all on one line so that the __LINE__ value is as-expected.
            CallstackEntryInfo ei(pAddress, __FUNCTION__, -1, __FILE__, __LINE__, NULL); expectedEntries.insert(expectedEntries.begin(), ei); VerifyCallstack(arc, expectedEntries, nErrorCount); TestCallstack02Impl(arc, expectedEntries, nErrorCount); EA::UnitTest::NonInlinableFunction();
        }


        ///////////////////////////////////////////////////////////////////////////////
        // TestCallstackImpl
        //
        static int TestCallstackImpl(EA::Allocator::ICoreAllocator* pCoreAllocator)
        {
            using namespace EA::Callstack;

            int  nErrorCount = 0;
            bool result;

            // Test GetCallstackContext
            CallstackContext context;
            result = GetCallstackContext(context, (intptr_t)EA::Thread::GetThreadId());
            #if EA_CALLSTACK_OTHER_THREAD_CONTEXT_SUPPORTED
                EATEST_VERIFY(result);
            #else
                EA_UNUSED(result);
            #endif
            
            // Get the path to the symbol database for the executing module.
            wchar_t dbDirectory[EA::IO::kMaxPathLength] = { 0 };
            wchar_t dbPath[EA::IO::kMaxPathLength] = { 0 };

            if(gpApplicationPath)
            {
                EA::StdC::Strlcpy(dbPath,      gpApplicationPath, EA::IO::kMaxPathLength);
                EA::StdC::Strlcpy(dbDirectory, gpApplicationPath, EA::IO::kMaxPathLength);
                wchar_t* p = EA::IO::Path::GetFileName(dbDirectory);
                if(p)
                    *p = 0;
            }
            else
            {
                EA::StdC::GetCurrentProcessPath(dbPath);
                EA::StdC::GetCurrentProcessDirectory(dbDirectory);
            }
            
            // Initialize an address rep cache object with the symbol database.
            AddressRepCache arc;
            arc.SetAllocator(pCoreAllocator);

            // Normally in a real application you wouldn't save and restore these; 
            // you'd just create one for the application.
            AddressRepCache* const pARCSaved = SetAddressRepCache(&arc);

            // On Microsoft platforms we use the .pdb or .map file associated 
            // with the executable. On GCC-based platforms, the executable is 
            // an ELF file which has the debug information built into it.
            EA::IO::DirectoryIterator dir;
            EA::IO::DirectoryIterator::EntryList entryList;
            
            #if defined(_MSC_VER) && EACALLSTACK_MS_PDB_READER_ENABLED // Check for the MS PDB reader being enabled because PDBFileCustom no longer works right for the most recent VC++ versions. 
            {   // Add .pdb files.
                dir.ReadRecursive(dbDirectory, entryList, EA_WCHAR("*.pdb"), EA::IO::kDirectoryEntryFile, true, true);

                for(EA::IO::DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
                {
                    EA::IO::DirectoryIterator::Entry& entry  = *it;
                    arc.AddDatabaseFile(entry.msName.c_str());
                }
                
                entryList.clear();
            }
            #elif !defined(EA_PLATFORM_APPLE) // We aren't able to read Apple DWARF info.
            {   // Add ELF (DWARF) files
                if(EA::IO::File::Exists(dbPath))  // Add the ELF file. Not every compiler uses ELF+DWARF. The Green Hills compiler, for example, uses a propietary format.
                    arc.AddDatabaseFile(dbPath);
            }
            #endif
            
            {   // Add .map files.
                #if defined(EA_PLATFORM_APPLE)
                    const wchar_t* kMapFileExtension = EA_WCHAR("*.txt");
                #else
                    const wchar_t* kMapFileExtension = EA_WCHAR("*.map");
                #endif
                
                dir.ReadRecursive(dbDirectory, entryList, kMapFileExtension, EA::IO::kDirectoryEntryFile, true, true);

                for(EA::IO::DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
                {
                    EA::IO::DirectoryIterator::Entry& entry  = *it;
                    arc.AddDatabaseFile(entry.msName.c_str());
                }
            
                entryList.clear();
            }
            
            //#if defined(EA_PLATFORM_APPLE)
            //{   // Add .dSYM (standalone DWARF) files.
            //    dir.ReadRecursive(dbDirectory, entryList, EA_WCHAR("*.dSYM"), EA::IO::kDirectoryEntryFile, true, true);
            //
            //    for(EA::IO::DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
            //    {
            //        EA::IO::DirectoryIterator::Entry& entry  = *it;
            //        arc.AddDatabaseFile(entry.msName.c_str());
            //    }
            //    entryList.clear();
            //}
            //#endif

            // Add source file directories for source code lookups.
            wchar_t sourceFileDir[EA::IO::kMaxPathLength];
            EA::StdC::Strlcpy(sourceFileDir, __FILE__, EA::IO::kMaxPathLength); // You need to compile with /FC under VC++ for __FILE__ to contain the full path.
            *EA::IO::Path::GetFileName(sourceFileDir) = 0;

            // Add the directory of this source file.
            arc.AddSourceCodeDirectory(sourceFileDir);
            arc.AddSourceCodeDirectory(EA_WCHAR("C:\\"));       // For Win32
            arc.AddSourceCodeDirectory(EA_WCHAR("/app_home/")); // For PS3

            if(!gbMissingDBReportedAlready && !arc.GetAddressRepLookupSet().GetDatabaseFileCount())
            {
                gbMissingDBReportedAlready = true;
                EA::UnitTest::Report("No DB (e.g. map) were files found. Did you set the right working directory or copy needed DB files?\n");
            }
            
            // Keep some information about subsequent function calls that will be
            // expected from EACallstack.
            CallstackEntryInfoArray expectedEntries;

            // Do a test of the function symbol lookup facilities.
            #if EACALLSTACK_TEST_USE_FUNCTION_POINTERS
                gTestFunctionArray[0] = TestCallstack01;
                gTestFunctionArray[1] = TestCallstack02;
                gTestFunctionArray[2] = TestCallstack03;
                gTestFunctionArray[3] = TestCallstack04;
                gTestFunctionArray[4] = TestCallstack05;

                gTestFunctionArray[0](arc, expectedEntries, nErrorCount);
            #else
                TestCallstack01(arc, expectedEntries, nErrorCount);
            #endif

            // Restore old AddressRepCache.
            SetAddressRepCache(pARCSaved);

            return nErrorCount;
        }

    #endif // EACALLSTACK_TEST_GETCALLSTACK

#endif // EA_CALLSTACK_GETCALLSTACK_SUPPORTED


///////////////////////////////////////////////////////////////////////////////
// TestCallstack
//
int TestCallstack()
{
    using namespace EA::Callstack;

    EA::UnitTest::ReportVerbosity(1, "\nTestCallstack\n");

    int nErrorCount = 0;

    gCustomCoreAllocator.mAllocationCount = 0;
    gCustomCoreAllocator.mFreeCount       = 0;

    #if EA_CALLSTACK_GETCALLSTACK_SUPPORTED && EACALLSTACK_TEST_GETCALLSTACK
        // Test with custom allocator, which does some additional verifications for us.
        EA::Callstack::SetAllocator(&gCustomCoreAllocator);
        nErrorCount += TestCallstackImpl(&gCustomCoreAllocator);  
        EATEST_VERIFY(gCustomCoreAllocator.mAllocationCount != 0);
        EATEST_VERIFY(gCustomCoreAllocator.mAllocationCount == gCustomCoreAllocator.mFreeCount);

        // Test with default allocator.
        gCustomCoreAllocator.mAllocationCount = INT32_MAX;
        gCustomCoreAllocator.mFreeCount       = INT32_MAX;
        EA::Callstack::SetAllocator(EA::Allocator::ICoreAllocator::GetDefaultAllocator());
        nErrorCount += TestCallstackImpl(EA::Allocator::ICoreAllocator::GetDefaultAllocator());
        EATEST_VERIFY(gCustomCoreAllocator.mAllocationCount == INT32_MAX);
        EATEST_VERIFY(gCustomCoreAllocator.mFreeCount       == INT32_MAX);
        gCustomCoreAllocator.mAllocationCount = 0;
        gCustomCoreAllocator.mFreeCount       = 0;
    #endif

    return nErrorCount;
}




///////////////////////////////////////////////////////////////////////////////
// TestDLLCallstack function
///////////////////////////////////////////////////////////////////////////////

#if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) // This could probably be supported under additional platforms/APIs.

    typedef void (*FunctionType)();
    typedef void (*CallFunctionType)(FunctionType pFunction);
    DLLFunctionDecl("EACallstackTestDLL.dll", CallFunction, CallFunctionType);

    static void FunctionCalledByDLL()
    {
        // This is a function that is called by a DLL. Our job here is to 
        // verify that we are able to read the DLL symbol information.
        // In particular, we should be called by a function named "CallFunction"
        // in a source file named "EACallstackTestDLL.cpp".
        using namespace EA::Callstack;

        int nErrorCount = 0;

        AddressRepCache* const pARC = EA::Callstack::GetAddressRepCache();
        EATEST_VERIFY(pARC != NULL);

        if(pARC)
        {
            void*  pCallstack[32];
            size_t nCallstackDepth = EA::Callstack::GetCallstack(pCallstack, 32, NULL); EA::UnitTest::NonInlinableFunction();

            for(size_t i = 0; (i < nCallstackDepth) && (i < 3); ++i) // We need only check ourselves (this function), our caller (the DLL), and our caller's caller (from this test app).
            {
                const char* pStrResults[kARTCount] = { 0, 0, 0 };
                int         pIntResults[kARTCount] = { 0, 0, 0 };

                // GetAddressRep should always return some info in this environment.
                const int nAddressRepFlags     = (kARTFlagFunctionOffset | kARTFlagFileLine | kARTFlagSource | kARTFlagAddress);
                const int nAddressRepReturnVal = pARC->GetAddressRep(nAddressRepFlags, (uint64_t)(uintptr_t)pCallstack[i], pStrResults, pIntResults);

                EATEST_VERIFY(nAddressRepReturnVal != 0);

                if(nAddressRepReturnVal)
                {
                    if(nAddressRepReturnVal & kARTFlagFunctionOffset)
                    {
                        EATEST_VERIFY(pStrResults[kARTFunctionOffset] != NULL);
                        EATEST_VERIFY(pIntResults[kARTFunctionOffset] != 0);
                    }

                    if(nAddressRepReturnVal & kARTFlagFileLine)
                    {
                        EATEST_VERIFY(pStrResults[kARTFileLine] != NULL);
                        EATEST_VERIFY(pIntResults[kARTFileLine] != 0);
                    }

                    // Verify that the source result is at least non-NULL.
                    if(nAddressRepReturnVal & kARTFlagSource)
                    {
                        EATEST_VERIFY(pStrResults[kARTSource] != NULL);
                    }

                    FixedString sAddress;
                    AddressToString(pCallstack[i], sAddress, true);

                    eastl::string sUnmangledFuncName;
                    if(pStrResults[kARTFunctionOffset])
                        UnmangleSymbol(pStrResults[kARTFunctionOffset], sUnmangledFuncName);

                    EA::UnitTest::ReportVerbosity(1, "   %2d: Info for address %s\n", i, sAddress.c_str());
                    EA::UnitTest::ReportVerbosity(1, "       Function:  %s\n",    pStrResults[kARTFunctionOffset] ? pStrResults[kARTFunctionOffset] : "<unknown>");
                   if(!sUnmangledFuncName.empty())
                    EA::UnitTest::ReportVerbosity(1, "       Unmangled: %s\n",    sUnmangledFuncName.c_str());
                    EA::UnitTest::ReportVerbosity(1, "       Offset:    %d\n",    pIntResults[kARTFunctionOffset]);
                    EA::UnitTest::ReportVerbosity(1, "       File:      %s\n",    pStrResults[kARTFileLine] ? pStrResults[kARTFileLine] : "<unknown>");
                    EA::UnitTest::ReportVerbosity(1, "       Line:      %d\n",    pIntResults[kARTFileLine]);
                    EA::UnitTest::ReportVerbosity(1, "       Address:   %s\n",    pStrResults[kARTAddress] ? pStrResults[kARTAddress] : "<unknown>");
                    EA::UnitTest::ReportVerbosity(1, "       Source:\n  %s\n\n",  pStrResults[kARTSource] ? pStrResults[kARTFileLine] : "<unknown>");
                }
            }
        }

        EA::UnitTest::IncrementGlobalErrorCount(nErrorCount);
    }

#endif // #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)



///////////////////////////////////////////////////////////////////////////////
// TestDLLCallstack
///////////////////////////////////////////////////////////////////////////////

int TestDLLCallstack()
{
    using namespace EA::Callstack;

    EA::UnitTest::ReportVerbosity(1, "\nTestDLLCallstack\n");

    int nErrorCount = 0;

    #if EA_CALLSTACK_GETCALLSTACK_SUPPORTED

        #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)

            wchar_t dllPath[EA::IO::kMaxPathLength]    = { 0 };
            wchar_t dllPdbPath[EA::IO::kMaxPathLength] = { 0 };
            wchar_t dllMapPath[EA::IO::kMaxPathLength] = { 0 };

            #if defined(_MSC_VER)
                EA::StdC::Strlcpy(dllPath, gpApplicationPath, EA::IO::kMaxPathLength);
                EA::StdC::Strcpy(EA::IO::Path::GetFileName(dllPath), EA_WCHAR("EACallstackTestDLL.dll"));

                EA::StdC::Strlcpy(dllPdbPath, gpApplicationPath, EA::IO::kMaxPathLength);
                EA::StdC::Strcpy(EA::IO::Path::GetFileName(dllPdbPath), EA_WCHAR("EACallstackTestDLL.pdb"));

                EA::StdC::Strlcpy(dllMapPath, gpApplicationPath, EA::IO::kMaxPathLength);
                EA::StdC::Strcpy(EA::IO::Path::GetFileName(dllMapPath), EA_WCHAR("EACallstackTestDLL.map"));
            #endif

            if(pCallFunction)
            {
                const bool bDBFilesExist = EA::IO::File::Exists(dllPdbPath) && EA::IO::File::Exists(dllMapPath);
                EATEST_VERIFY(bDBFilesExist);

                if(bDBFilesExist)
                {
                    // Get the path to the symbol database for the executing module.
                    wchar_t dbPath[EA::IO::kMaxPathLength] = { 0 };

                    if(gpApplicationPath)
                    {
                        EA::StdC::Strlcpy(dbPath, gpApplicationPath, EA::IO::kMaxPathLength);
                        dbPath[EA::IO::kMaxPathLength - 1] = 0;
                    }
                    else
                        EA::StdC::GetCurrentProcessPath(dbPath);

                    // Initialize an address rep cache object with the symbol database.
                    AddressRepCache arc;
                    arc.SetAllocator(EA::Callstack::GetAllocator());

                    // Normally in a real application you wouldn't save and restore these; 
                    // you'd just create one for the application.
                    AddressRepCache* const pARCSaved = SetAddressRepCache(&arc);

                    // On Microsoft platforms we use the .pdb or .map file associated 
                    // with the executable. On GCC-based platforms, the executable is 
                    // an ELF file which has the debug information built into it.
                    #if defined(_MSC_VER)
                        EA::StdC::Strcpy(EA::IO::Path::GetFileExtension(dbPath), EA_WCHAR(".map"));
                        arc.AddDatabaseFile(dbPath);

                        EA::StdC::Strcpy(EA::IO::Path::GetFileExtension(dbPath), EA_WCHAR(".pdb"));
                        arc.AddDatabaseFile(dbPath);
                    #else
                        arc.AddDatabaseFile(dbPath);
                    #endif

                    // Add the EACallstackTestDLL.dll to the AddressRep database list.
                    arc.AddDatabaseFile(dllMapPath);
                    arc.AddDatabaseFile(dllPdbPath);

                    // To consider: Add source code directories here, including the DLL source directory.

                    // Call the DLL function CallFunction, which in turn should call FunctionCalledByDLL.
                    // We want to verify that we can read the symbols of the DLL while in FunctionCalledByDLL.
                    pCallFunction()(FunctionCalledByDLL);

                    // Restore old AddressRepCache.
                    SetAddressRepCache(pARCSaved);
                }
            }
            else
            {
                // Did you make sure EACallstack/test/bin/EACallstackTestDLL.dll, EACallstackTestDLL.pdb, 
                // and EACallstackTestDLL.map were copied to the folder with this executable? That needs to 
                // be done for this to succeed.

                // We write out a failure string but we don't flag the test as failed.
                // This is because we don't have an automated way to ensure the presence
                // of the test DLL in the executable directory.
                EA::UnitTest::ReportVerbosity(1, "Test DLL or its .pdb and .map files not found: %ls\n", dllPath);
            }

        #endif

        EA::UnitTest::IncrementGlobalErrorCount(nErrorCount);

    #endif // EA_CALLSTACK_GETCALLSTACK_SUPPORTED

    return nErrorCount;
}


// The PSVita plaform requires that you declare the following variable.
#ifdef EA_PLATFORM_PSP2
    unsigned int sceLibcHeapSize = 1024 * 1024;
#endif


///////////////////////////////////////////////////////////////////////////////
// SetupTestFileList
///////////////////////////////////////////////////////////////////////////////

static void SetupTestFileList(int argc, char** argv)
{
    using namespace EA::IO;

    EA::UnitTest::CommandLine commandLine(argc, argv);
    EA::IO::Path::PathStringW dataDirectory;
    eastl::string8            sResult;

    // Example: Text.exe -d:"C:\Dir\my data"
    if(commandLine.FindSwitch("-d", false, &sResult) >= 0)
    {
        // Convert from char8_t to wchar_t.
        int nRequiredStrlen = EA::StdC::Strlcpy((wchar_t*)NULL, sResult.c_str(), 0, sResult.length());
        dataDirectory.resize((eastl_size_t)nRequiredStrlen + 1);
        EA::StdC::Strlcpy(&dataDirectory[0], sResult.c_str(), dataDirectory.capacity(), sResult.length());
        EA::IO::Path::EnsureTrailingSeparator(dataDirectory);
    }
    else
    {
        #if defined (EA_PLATFORM_IPHONE)
            wchar_t homeDirectory[EA::IO::kMaxDirectoryLength] = { 0 };
            wchar_t appName[EA::IO::kMaxDirectoryLength] = { 0 };
            //EA::IO::appleGetHomeDirectory(homeDirectory, EA::IO::kMaxDirectoryLength); Disabled until we can test this.
            //EA::IO::appleGetAppName(appName, EA::IO::kMaxDirectoryLength);
            dataDirectory.assign(homeDirectory);
            dataDirectory += EA_WCHAR("/");
            dataDirectory += appName;
            dataDirectory += EA_WCHAR(".app/");
        #elif defined (EA_PLATFORM_ANDROID)
            dataDirectory.assign(EA_WCHAR("appbundle:/data/"));
        #elif defined(EA_PLATFORM_PLAYBOOK)                  
            dataDirectory.assign(EA_WCHAR("app/native/"));
		#elif defined(EA_PLATFORM_CAFE)
			dataDirectory.assign(EA_WCHAR("/vol/content/"));
        #else
            dataDirectory.resize(256);
            dataDirectory.resize((unsigned)EA::IO::Directory::GetCurrentWorkingDirectory(&dataDirectory[0]));
        #endif
    }

    if(!dataDirectory.empty())
    {
        DirectoryIterator            di;
        DirectoryIterator::EntryList entryList;

        di.ReadRecursive(dataDirectory.c_str(), entryList, EA_WCHAR("*"), kDirectoryEntryFile, true, true);

        for(DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
        {
            const DirectoryIterator::Entry& entry = *it;
            gTestFileList.push_back(entry.msName.c_str());
        }
    }

    if(gTestFileList.empty())
    {
        EA::UnitTest::Report("No data files were found.\n");
        EA::UnitTest::Report("You need to run with the working directory set to the full path to EACallstack/dev/test/source/data or\n");
        EA::UnitTest::Report("you need to run with the -d <path> command line argument\n");
        EA::UnitTest::Report("The current data directory is \"%ls\".\n", dataDirectory.c_str());
    }
}


///////////////////////////////////////////////////////////////////////////////
// EATestMain
//

int EATestMain(int argc, char** argv)
{
    using namespace EA::UnitTest;

    int nErrorCount = 0;

    bool fsInitSuccess = EA::IO::InitializeFileSystem(true);
    EA_UNUSED(fsInitSuccess);
    EA_ASSERT(fsInitSuccess);

    EA::UnitTest::PlatformStartup();

    gpApplicationPath = (argv ? argv[0] : NULL);
    #if defined(EA_DEBUG) && EA_MEMORY_PPMALLOC_ENABLED
        EA::Allocator::gpEAGeneralAllocatorDebug->SetDefaultDebugDataFlag(EA::Allocator::GeneralAllocatorDebug::kDebugDataIdCallStack);
        EA::Allocator::gpEAGeneralAllocatorDebug->SetDefaultDebugDataFlag(EA::Allocator::GeneralAllocatorDebug::kDebugDataIdGuard);
    #endif
    EA::IO::SetAllocator(EA::Allocator::ICoreAllocator::GetDefaultAllocator());
    EA::Thread::SetAllocator(EA::Allocator::ICoreAllocator::GetDefaultAllocator());
    EA::Thread::InitCallstack();
    SetupTestFileList(argc, argv);

    // Add the tests
    TestApplication testSuite("EACallstack Unit Tests", argc, argv);

    testSuite.AddTest("CallstackRecorder",  TestCallstackRecorder);
    testSuite.AddTest("PS3DumpFile",        TestPS3DumpFile);
    testSuite.AddTest("Dasm",               TestDasm);
    testSuite.AddTest("MapFile",            TestMapFile);
    testSuite.AddTest("CallstackRecorder",  TestCallstackRecorder);
    testSuite.AddTest("Misc",               TestMisc);
    testSuite.AddTest("OperatorNew",        TestOperatorNew);
    testSuite.AddTest("SymbolUtil",         TestSymbolUtil);
    testSuite.AddTest("Callstack",          TestCallstack);
    testSuite.AddTest("DLLCallstack",       TestDLLCallstack);

    nErrorCount += testSuite.Run();

    EATEST_VERIFY(gCustomCoreAllocator.mAllocationCount == 0);
    EATEST_VERIFY(gCustomCoreAllocator.mFreeCount == 0);

    // Add in the global errors.
    nErrorCount += EA::UnitTest::GetGlobalErrorCount();

    EA::Thread::ShutdownCallstack();
    EA::UnitTest::PlatformShutdown(nErrorCount);

    return nErrorCount;
}


