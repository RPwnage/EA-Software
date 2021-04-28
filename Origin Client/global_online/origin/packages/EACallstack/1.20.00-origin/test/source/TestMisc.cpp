/////////////////////////////////////////////////////////////////////////////
// TestMisc.cpp
//
// Copyright (c) 2008, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#include <EACallstackTest/EACallstackTest.h>
#include <EACallstack/EACallstack.h>
#include <EACallstack/Allocator.h>
#include <EACallstack/MapFile.h>
#include <EACallstack/DWARF2File.h>
#include <EACallstack/PDBFile.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EATest/EATest.h>


EA_DISABLE_VC_WARNING(4740) // flow in or out of inline asm code suppresses global optimization.


///////////////////////////////////////////////////////////////////////////////
// TestOperatorNew
//
int TestOperatorNew()
{
    using namespace EA::Callstack;

    int nErrorCount = 0;

    EA::Callstack::SetAllocator(&gCustomCoreAllocator);
    // EA::Allocator::ICoreAllocator* pCoreAllocator = gCustomCoreAllocator;

    #if EACALLSTACK_PDBFILE_ENABLED
        PDBFileCustom* pPDB = new PDBFileCustom;
        delete pPDB;
    #endif

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestModule
//
static int TestModule()
{
    using namespace EA::Callstack;

    int nErrorCount = 0;

    #if EACALLSTACK_MODULE_INFO_SUPPORTED
        // uint64_t GetModuleBaseAddress(const wchar_t* pModuleName = NULL);
        // uint64_t GetModuleBaseAddress(const char* pModuleName = NULL);
        uint64_t baseAddress = GetModuleBaseAddress(L"EACallstackTest");
        EATEST_VERIFY((baseAddress == 0) || (baseAddress != kBaseAddressInvalid));

        baseAddress = GetModuleBaseAddress("EACallstackTest");
        EATEST_VERIFY((baseAddress == 0) || (baseAddress != kBaseAddressInvalid));


        // uint64_t GetModuleBaseAddressByHandle(ModuleHandle moduleHandle = 0);
        baseAddress = GetModuleBaseAddressByHandle((ModuleHandle)0);
        EATEST_VERIFY((baseAddress == 0) || (baseAddress != kBaseAddressInvalid));


        // uint64_t GetModuleBaseAddressByAddress(const void* pCodeAddress);
        void* pInstruction;
        EAGetInstructionPointer(pInstruction);
        baseAddress = GetModuleBaseAddressByAddress(pInstruction);
        EATEST_VERIFY((baseAddress == 0) || (baseAddress != kBaseAddressInvalid));


        // size_t GetModuleInfo(ModuleInfo* pModuleInfoArray, size_t moduleArrayCapacity);
        #if defined(EA_PLATFORM_APPLE)
        const unsigned int kMaxNumModules = 512;
        #else
        const unsigned int kMaxNumModules = 64;
        #endif
        ModuleInfo moduleInfoArray[kMaxNumModules];
        size_t numModules = GetModuleInfo(moduleInfoArray, EAArrayCount(moduleInfoArray));
        EATEST_VERIFY(numModules != 0);

        // size_t GetModuleInfo(ModuleInfo* pModuleInfoArray, size_t moduleArrayCapacity);
        const int kNotFound = -1;
        int moduleIndex = kNotFound;
        
        //To Consider: Create a module that can be built and linked to any platform
        #if defined(EA_PLATFORM_WINDOWS)
            const char* moduleName = "kernel32";
        #elif defined(EA_PLATFORM_XENON)
            const char* moduleName = "xboxkrnl";
        #elif defined(EA_PLATFORM_LINUX)
            const char* moduleName = "libc.so.6";
        #elif defined(EA_PLATFORM_PS3)
            const char* moduleName = "cellSysmodule_Library";
        #elif defined(EA_PLATFORM_CAPILANO)
            const char* moduleName = "KERNELX";
        #elif defined(EA_PLATFORM_KETTLE)
            const char* moduleName = "libkernel.sprx";
        #elif defined(EA_PLATFORM_APPLE)
            const char* moduleName = "libdyld.dylib";
        #else
            #error Test Module Name not determined for this platform
        #endif

        for(size_t i = 0; i < numModules; i++)
        {
            //EA::EAMain::Report("Module Name: %s\n",moduleInfoArray[i].mName.c_str());
            if(EA::StdC::Strnicmp(moduleInfoArray[i].mName.c_str(),moduleName,EA::StdC::Strlen(moduleName)+1) == 0) //+1 for Null Terminator
            {
                moduleIndex = (int)i;
                break;
            }
        }
        EATEST_VERIFY(moduleIndex != kNotFound);
        ModuleInfo* testModule = &moduleInfoArray[moduleIndex];
        EATEST_VERIFY(testModule != NULL);
        ModuleInfo moduleInfo;

        EATEST_VERIFY(GetModuleInfoByAddress((void*)(intptr_t)testModule->mBaseAddress, moduleInfo, moduleInfoArray, numModules));
        EATEST_VERIFY(moduleInfo == *testModule);

        EATEST_VERIFY(GetModuleInfoByHandle(testModule->mModuleHandle, moduleInfo, moduleInfoArray, numModules));
        EATEST_VERIFY(moduleInfo == *testModule);

        EATEST_VERIFY(GetModuleInfoByName(testModule->mName.c_str(), moduleInfo, moduleInfoArray, numModules));
        EATEST_VERIFY(moduleInfo == *testModule);
        
    #endif

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestMiscImpl
//
static int TestMiscImpl(EA::Allocator::ICoreAllocator* /*pCoreAllocator*/)
{
    using namespace EA::Callstack;

    int nErrorCount = 0;

    #if 0 // defined(AUTHOR_PPEDRIANA) && defined(EA_PLATFORM_WINDOWS) // Disabled until we can have a more generic way of testing this.
    {
        IAddressRepLookup* pLookup      = NULL;
        bool               result       = false;
        const uint32_t     kEndValue    = 0xfefefefe;
        uint64_t           addrArray[8] = { kEndValue };

        #if 0 // EACALLSTACK_DWARFFILE_ENABLED
            DWARF2File dwarfFile;
            IAddressRepLookup* pLookup = &dwarfFile;
            dwarfFile.SetOption(DWARF2File::kOptionCopyFileToMemory, 0);
            dwarfFile.SetOption(DWARF2File::kOptionSymbolCacheLevel, 0);
            bool result = pLookup->Init(L"C:\\Temp\\DICE PS3 SN EACallstack\\Frost.Game_Ps3_Final.self");
            bool result = pLookup->Init(L"D:\\Temp\\DICE PS3 SN EACallstack\\LeakAnalyze\\Data\\game.self");
            bool result = pLookup->Init(L"D:\\Temp\\EALA PS3 SN EACallstack\\MOHAGame-PS3Release.elf");
            bool result = pLookup->Init(L"C:\\Temp\\FromDaniel\\Frost.Game_Ps3_Release.self"); 0x00f122c8
            bool result = pLookup->Init(L"C:\\Projects\\EAOS\\UTF\\build\\EACallstack\\dev\\ps3-gcc-dev-debug\\bin\\test\\EACallstackTest.self"); 0x00012018
        #endif

        #if 0 // EACALLSTACK_PDBFILE_ENABLED
            //PDBFileMS pdbFile;
            PDBFileCustom pdbFile; 
            pdbFile.SetOption(IAddressRepLookup::kOptionSymbolCacheLevel, 1);
            pLookup = &pdbFile;

            /*
            pLookup->SetBaseAddress(0x82000000);
            result  = pLookup->Init(L"D:\\Projects\\EAOS\\UTF\\build\\EAStdC\\dev\\xenon-vc-dev-debug\\bin\\test\\EAStdCTest.pdb");
            addrArray[0] = 0x00000000;
            addrArray[1] = 0xffffffff;
            addrArray[2] = 0x82098770;
            addrArray[3] = 0x821d0000;
            addrArray[4] = 0x82186490;
            addrArray[5] = kEndValue;
            */

            /*
            pLookup->SetBaseAddress(0x0000000140000000i64);
            result  = pLookup->Init(L"D:\\Projects\\EAOS\\UTF\\build\\EAStdC\\dev\\pc64-vc-dev-debug\\bin\\test\\EAStdCTest.pdb");
            addrArray[0] = 0x0000000140002bb0;
            addrArray[1] = 0x0000000140143580;
            addrArray[2] = 0x00000001400e53f0;
            addrArray[3] = 0x000000014014a000;
            addrArray[4] = 0x0000000000000000;
            addrArray[5] = 0xffffffffffffffff;
            addrArray[6] = kEndValue;
            */

            /*
            pLookup->SetBaseAddress(0x00400000);
            result  = pLookup->Init(L"D:\\Projects\\EAOS\\UTF\\build\\EAStdC\\dev\\pc-vc-dev-debug\\bin\\test\\EAStdCTest.pdb");
            addrArray[0] = 0x00000000;
            addrArray[1] = 0xffffffff;
            addrArray[2] = 0x00402b10;
            addrArray[3] = 0x004dd248;
            addrArray[4] = 0x00502000;
            addrArray[5] = 0x00486280;
            addrArray[6] = kEndValue;
            */

            pLookup->SetBaseAddress(0x20000000);
            result  = pLookup->Init(L"D:\\Projects\\EAOS\\UTF\\build\\EAStdC\\dev\\pc-vc-dll-dev-debug\\build\\EAStdC\\EAStdC.pdb");
            addrArray[0] = 0x00000000;
            addrArray[1] = 0xffffffff;
            addrArray[2] = 0x20002a30;
            addrArray[3] = 0x20031150;
            addrArray[4] = 0x20039500;
            addrArray[5] = 0x2001e3d0;
            addrArray[6] = kEndValue;
        #endif

        if(result)
        {
            FixedString pStrResults[kARTCount];
            int         pIntResults[kARTCount];
            int         count;
            size_t      i;
            bool        b64Bit = false;

            for(i = 0; i < sizeof(addrArray)/sizeof(addrArray[0]) && (addrArray[i] != kEndValue) && !b64Bit; i++)
                b64Bit = (addrArray[i] > 0xffffffff);

            for(i = 0; i < sizeof(addrArray)/sizeof(addrArray[0]) && (addrArray[i] != kEndValue); i++)
            {
                if(b64Bit)
                    EA::UnitTest::ReportVerbosity(1, "Address: 0x%016I64x\n", (uint64_t)addrArray[i]);
                else
                    EA::UnitTest::ReportVerbosity(1, "Address: 0x%08x\n", (uint32_t)addrArray[i]);

                count = pLookup->GetAddressRep(kARTFlagAddress | kARTFlagFunctionOffset | kARTFlagFileLine | kARTFlagSource, (void*)(uintptr_t)addrArray[i], pStrResults, pIntResults);

                if(count)
                {
                    if(!pStrResults[kARTFunctionOffset].empty())
                        EA::UnitTest::ReportVerbosity(1, "Function/Offset: %s %d\n", pStrResults[kARTFunctionOffset].c_str(), pIntResults[kARTFunctionOffset]);

                    if(!pStrResults[kARTFileLine].empty())
                        EA::UnitTest::ReportVerbosity(1, "File/Line: %s %d\n\n", pStrResults[kARTFileLine].c_str(), pIntResults[kARTFileLine]);
                }

                EA::UnitTest::ReportVerbosity(1, "\n");

                for(size_t j = 0; j < sizeof(pStrResults)/sizeof(pStrResults[0]); j++)
                {
                    pStrResults[j].clear();
                    pIntResults[j] = 0;
                }
            }
        }
    }
    #endif 

    #if 0 // defined(AUTHOR_PPEDRIANA) // Disabled until we can have a more generic way of testing this.
    {
        #ifdef EA_PLATFORM_WINDOWS
            const char16_t* const pDBPath = L"D:\\Temp\\SSX5 360 PDB Vista64 Symmogifier fail\\ssxvd.pdb";
        #elif defined(EA_PLATFORM_XENON)
            const char16_t* const pDBPath = L"GAME:\\fnbd.pdb";
        #else
            const char16_t* const pDBPath = L"/app_home/fnbd.pdb";
        #endif

        PDBFile pdbFile;
        pdbFile.SetBaseAddress(0x82000000);
        bool result = pdbFile.Init(pDBPath, false);

        if(result)
        {
            FixedString pStrResults[kARTCount];
            int         pIntResults[kARTCount];
            int         count;

            count = pdbFile.GetAddressRep(kARTFlagFunctionOffset | kARTFlagFileLine, (void*)(uintptr_t)0x8279ee4c, pStrResults, pIntResults);

            if(count)
                EA::UnitTest::ReportVerbosity(1, "%s\n%s\n%s\n", pStrResults[0].c_str(), pStrResults[1].c_str(), pStrResults[2].c_str());
        }
    }
    #endif

    gCustomCoreAllocator.mAllocationCount = 0;
    gCustomCoreAllocator.mFreeCount       = 0;

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestStackBase
//
static int TestStackBase()
{
    using namespace EA::Callstack;

    int nErrorCount = 0;

    {
        // EACALLSTACK_API void  SetStackBase(void* pStackBase);
        // inline          void  SetStackBase(uintptr_t pStackBase){ SetStackBase((void*)pStackBase); }
        // EACALLSTACK_API void* GetStackBase();
        // EACALLSTACK_API void* GetStackLimit();
        // #define EASetStackBase()

        using namespace EA::Callstack;

        void* pBase;
        void* pLimit;
        void* pLocalVariable = &nErrorCount;

        EA::Callstack::SetStackBase((uintptr_t)(intptr_t)-1);
        pBase = EA::Callstack::GetStackBase();
        EATEST_VERIFY(pBase != NULL); // pBase will either be the true stack base as exposed by the platform SDK, or 0x00000001 if the former isn't possible.
        if((intptr_t)pBase != (intptr_t)-1) // If the true stack extents could be determined...
        {
            pLimit = EA::Callstack::GetStackLimit();

            EATEST_VERIFY((uintptr_t)pLimit         <  (uintptr_t)pBase);    // Every ABI of significance has a stack that grows downward instead of upward, and for purported security reasons this likely won't change.
            EATEST_VERIFY((uintptr_t)pLocalVariable <  (uintptr_t)pBase);    //
            EATEST_VERIFY((uintptr_t)pLocalVariable >  (uintptr_t)pLimit);   // 
        }

        pLimit = GetStackLimit();
        EATEST_VERIFY(pLimit != NULL);

        EASetStackBase(); // Sets the stack base to be the current stack pointer (in the case that the platform doesn't support reading the true stack extents.
        pBase = EA::Callstack::GetStackBase();
        EATEST_VERIFY(pBase != NULL);

        EA::Callstack::SetStackBase((void*)NULL); // Using NULL causes it to read the current stack pointer and use that.
        pBase = EA::Callstack::GetStackBase();
        EATEST_VERIFY(pBase != NULL);
    }

    return nErrorCount;
}



///////////////////////////////////////////////////////////////////////////////
// TestMisc
//
int TestMisc()
{
    using namespace EA::Callstack;

    EA::UnitTest::ReportVerbosity(1, "\nTestMisc\n");

    int nErrorCount = 0;

    nErrorCount += TestStackBase();
    nErrorCount += TestModule();

    {
        // Test with default allocator
        nErrorCount += TestMiscImpl(EA::Allocator::ICoreAllocator::GetDefaultAllocator());

        // Test with custom allocator, which does some additional verifications for us.
        EATEST_VERIFY(gCustomCoreAllocator.mAllocationCount == 0);
        EATEST_VERIFY(gCustomCoreAllocator.mFreeCount       == 0);
        EA::Callstack::SetAllocator(&gCustomCoreAllocator);
        nErrorCount += TestMiscImpl(&gCustomCoreAllocator);
        EATEST_VERIFY(gCustomCoreAllocator.mAllocationCount == gCustomCoreAllocator.mFreeCount);
        gCustomCoreAllocator.mAllocationCount = 0;
        gCustomCoreAllocator.mFreeCount       = 0;
    }

    return nErrorCount;
}



