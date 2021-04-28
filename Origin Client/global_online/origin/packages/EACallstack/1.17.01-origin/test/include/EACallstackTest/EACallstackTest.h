/////////////////////////////////////////////////////////////////////////////
// EACallstackTest.h
//
// Copyright (c) 2009, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#ifndef EACALLSTACKTEST_H
#define EACALLSTACKTEST_H


#include <EAIO/EAStream.h>
#include <EASTL/list.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/fixed_string.h>
#include <coreallocator/icoreallocator_interface.h>
#include <EACallstack/EAAddressRep.h>



/// EACALLSTACK_NATIVE_FUNCTION_LOOKUP_AVAILABLE
///
/// Defined as 0 or 1.
/// Currently we support at least function name lookups via .map files.
///
#if !defined(EACALLSTACK_NATIVE_FUNCTION_LOOKUP_AVAILABLE)
    #if defined(EA_PLATFORM_DESKTOP)
        #define EACALLSTACK_NATIVE_FUNCTION_LOOKUP_AVAILABLE 1
    #else
        // Actually we can in fact do function name lookups via .map files, but 
        // this running test app needs to be able to open that .map file, which
        // means we need to find a way to copy that .map file to the right place
        // on startup. As of this writing we don't have that in place.
        #define EACALLSTACK_NATIVE_FUNCTION_LOOKUP_AVAILABLE 0
    #endif
#endif


/// EACALLSTACK_NATIVE_FILELINE_LOOKUP_AVAILABLE
///
/// Defined as 0 or 1.
///
#if !defined(EACALLSTACK_NATIVE_FILELINE_LOOKUP_AVAILABLE)
    #if defined(EA_COMPILER_MSVC) && defined(EA_PLATFORM_DESKTOP)  // Our XBox 360 reader can't read file/line info yet.
        #define EACALLSTACK_NATIVE_FILELINE_LOOKUP_AVAILABLE 1
    #else
        // Our ELF/Dwarf reader doesn't always work natively. We have to fix something
        // about it which is a non-trivial task and so we'll have to disable this
        // until we get it done.
        #define EACALLSTACK_NATIVE_FILELINE_LOOKUP_AVAILABLE 0
    #endif
#endif



// ReferenceStack types used by this unit test
typedef eastl::fixed_string<char, 32, true> InfoString;

struct CallstackEntryInfo
{
    void*       mpAddress;          // 
    InfoString  msFunction;         // 
    int         mnFunctionOffset;   // A value of -1 means we don't know.
    InfoString  msFile;             // 
    int         mnFileLine;         // 
    InfoString  msSource;           // 

    CallstackEntryInfo()
      : mpAddress(NULL), msFunction(), mnFunctionOffset(-1), msFile(), mnFileLine(-1), msSource(){}

    CallstackEntryInfo(void* pAddress, const char* pFunction, int nFunctionOffset, const char* pFile, int fileLine, const char* pSource)
      : mpAddress(pAddress), msFunction(pFunction ? pFunction : ""), mnFunctionOffset(nFunctionOffset), msFile(pFile ? pFile : ""), mnFileLine(fileLine), msSource(pSource ? pSource : ""){}

    void Clear()
        { mpAddress = NULL; msFunction.clear(); mnFunctionOffset = -1; msFile.clear(); mnFileLine = -1; msSource.clear(); }
};

typedef eastl::vector<CallstackEntryInfo> CallstackEntryInfoArray;



int  Vsnprintf8 (char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments);
int  Vsnprintf16(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments);
int  Vsnprintf32(char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments);
void VerifyCallstack(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& rc, int& nErrorCount);

#ifdef EA_PLATFORM_XENON
    // On Xenon these functions are declared as static so that the addresses of the functions are
    // the functions themselves and not their incremental link table trampoline address.
    #define EACALLSTACK_TEST_FUNCTION_LINKAGE static
#else
    #define EACALLSTACK_TEST_FUNCTION_LINKAGE
    EA_NO_INLINE void TestCallstack01(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount);
    EA_NO_INLINE void TestCallstack02(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount);
    EA_NO_INLINE void TestCallstack03(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount);
    EA_NO_INLINE void TestCallstack04(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount);
    EA_NO_INLINE void TestCallstack05(EA::Callstack::AddressRepCache& arc, CallstackEntryInfoArray& expectedEntries, int& nErrorCount);
#endif


int  TestCallstackRecorder();
int  TestPS3DumpFile();
int  TestDasm();
int  TestMapFile();
int  TestCallstackRecorder();
int  TestMisc();
int  TestOperatorNew();
int  TestSymbolUtil();
int  TestCallstack();
int  TestDLLCallstack();



namespace EA
{
    namespace IO
    {
        /// class StreamLog
        ///
        /// Implements a fixed-size write-only stream which traces-out via the EA_LOG macro
        /// and thus allows for logging to an IStream.
        ///
        /// This class is not inherently thread-safe. As a result, thread-safe usage
        /// between multiple threads requires higher level coordination, such as a mutex.
        ///
        class StreamLog : public EA::IO::IStream
        {
        public:
            enum { kTypeStreamLog = 0x32ea45ac };

            StreamLog();

            int AddRef();
            int Release();

            // IStream
            uint32_t    GetType() const { return kTypeStreamLog; }
            int         GetAccessFlags() const { return kAccessFlagWrite; }
            int         GetState() const { return 0; }
            bool        Close() { return true; }

            size_type   GetSize() const { return 0; }
            bool        SetSize(size_type) { return true; }

            off_type    GetPosition(PositionType) const { return 0; }
            bool        SetPosition(off_type, PositionType){ return true; }

            size_type   GetAvailable() const { return 0; }
            size_type   Read(void*, size_type) { return 0; }

            bool        Flush() { return true; }
            bool        Write(const void* pData, size_type nSize);

        protected:
            typedef     eastl::fixed_string<char, 256, true> LineType;

            LineType    mLine;
            int         mnRefCount;

        }; // class StreamLog

    }
}


///////////////////////////////////////////////////////////////////////////////
// CustomCoreAllocator
//
class CustomCoreAllocator : public EA::Allocator::ICoreAllocator 
{
public:
    CustomCoreAllocator()
      : mAllocationCount(0), mFreeCount(0), mAllocationVolume(0)
    { }

    CustomCoreAllocator(const CustomCoreAllocator&)
      : mAllocationCount(0), mFreeCount(0), mAllocationVolume(0)
    { }

    ~CustomCoreAllocator()
    { }

    CustomCoreAllocator& operator=(const CustomCoreAllocator&)
    { return *this; }

    void* Alloc(size_t size, const char* /*name*/, unsigned /*flags*/)
    {
        mAllocationCount++;
        mAllocationVolume += size;
        void* pMemory = malloc(size);
        return pMemory;
    }

    void* Alloc(size_t size, const char* /*name*/, unsigned /*flags*/, 
                unsigned /*align*/, unsigned /*alignOffset*/)
    {
        mAllocationCount++;
        mAllocationVolume += size;
        void* pMemory = malloc(size);
        return pMemory;
    }

    void Free(void* block, size_t size)
    {
        mFreeCount++;
        mAllocationVolume -= size;  // It turns out that size is often zero (unspecified), so this is almost useless.
        free(block);
    }

public:
    int32_t mAllocationCount;
    int32_t mFreeCount;
    int64_t mAllocationVolume;
};



// Global variables
extern void*                        gpAddress;
extern CustomCoreAllocator          gCustomCoreAllocator;
extern eastl::list<eastl::wstring>  gTestFileList;
extern const char*                  gpApplicationPath;



#endif // Header include guard
