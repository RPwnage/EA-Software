/////////////////////////////////////////////////////////////////////////////
// TestMapFile.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
/////////////////////////////////////////////////////////////////////////////


#include <EACallstackTest/EACallstackTest.h>
#include <EACallstack/MapFile.h>
#include <EACallstack/EACallstack.h>
#include <EACallstack/Allocator.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAScanf.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAStreamAdapter.h>
#include <EAIO/EAStreamBuffer.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EATest/EATest.h>


namespace EA
{
    namespace Callstack
    {
        const int kExpectedResultsCount      = 8;
        const int kExpectedResultsBufferSize = 1024;

        struct ExpectedResults
        {
            SymbolInfoType  mExpectedSymbolInfoType;
            TargetOS        mExpectedTargetOS;
            TargetProcessor mExpectedTargetProcessor;
            int             mResultsCount;
            uint64_t        mAddressesToLookUp[kExpectedResultsCount];
            int             mExpectedLookupResult[kExpectedResultsCount];   // These are boolean 0 or 1.
            char            mExpectedLookupName[kExpectedResultsCount][kExpectedResultsBufferSize];
        };

        // We use globals here because they can be large, unlike local variables. And are pre-allocated and thus don't risk hitting an empty heap.
        ExpectedResults gExpectedResults;
        const size_t    kLargeLineBufferCapacity = 4096;
        char            gLargeLineBuffer[kLargeLineBufferCapacity]; // We make this global because its size might be too much for some platforms' stacks.
        const size_t    kLargeNameCapacity = 4096;
        char            gLargeName[kLargeNameCapacity];
    }
}


///////////////////////////////////////////////////////////////////////////////
// ReadMapResultsFile
//
// Not much error checking is done; the .mapResults files are expected to simply 
// be propertly written, else the test can fail.
// See EACallstack/test/data/*.mapResults for examples.
// Map results file format, by example:
//    // Can have initial C++-like comment lines, like this.
//    // Can't have any blank lines between these and the text.
//    GCC                       // SymbolInfoType. Comments to the right like this are valid but not required.
//    Linux                     // TargetOS
//    x86                       // mExpectedTargetProcessor
//    0x00000000 0              // Intentionally invalid address. Success in looking it up should be 0 (failed).
//    0x12345678 1 strlen       // Valid address. Success in looking it up is 1 (succeeded). Function name has strlen in it.
//    0x23456789 1 strcpy       // Valid address. Success in looking it up is 1 (succeeded). Function name has strcpy in it.
//    . . .
///////////////////////////////////////////////////////////////////////////////

static int ReadMapResultsFile(const eastl::wstring& sMapResultsFilePath, EA::Callstack::ExpectedResults& expectedResults)
{
    using namespace EA::IO;
    using namespace EA::Callstack;

    int nErrorCount = 0;

    memset(&expectedResults, 0, sizeof(expectedResults));

    FileStream fileStream(sMapResultsFilePath.c_str());
    fileStream.AddRef(); // Stack instances should be AddRef'd with no matching Release.

    if(fileStream.Open())
    {
        StreamBuffer streamBuffer(512, 0, &fileStream);
        char         buffer[384] = {};

        // Read past any initial comment lines (which begin with //).
        while(EA::IO::ReadLine(&streamBuffer, buffer, EAArrayCount(buffer)) < EA::IO::kSizeTypeDone) // While there were more lines...
        {
            if((buffer[0] != '/') || buffer[1] != '/')
                break;
            memset(buffer, 0, sizeof(buffer));
        }

        // Read mExpectedSymbolInfoType
        // EA::IO::ReadLine(&streamBuffer, buffer, EAArrayCount(buffer)); No need to read, as last ReadLine above did it for us.

        if(EA::StdC::Stristr(buffer, "None"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypeNone;
        else if(EA::StdC::Stristr(buffer, "VC"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypeMapVC;
        else if(EA::StdC::Stristr(buffer, "GCC"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypeMapGCC;
        else if(EA::StdC::Stristr(buffer, "SN"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypeMapSN;
        else if(EA::StdC::Stristr(buffer, "CW"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypeMapCW;
        else if(EA::StdC::Stristr(buffer, "GH"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypeMapGHS;
        else if(EA::StdC::Stristr(buffer, "Apple"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypeMapApple;
        else if(EA::StdC::Stristr(buffer, "PDB7"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypePDB7;
        else if(EA::StdC::Stristr(buffer, "PDB8"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypePDB8;
        else if(EA::StdC::Stristr(buffer, "Dwarf"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypeDwarf2;
        else if(EA::StdC::Stristr(buffer, "GHS"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypeGHS;           // This is currently in conflict with kSymbolInfoTypeMapGH above.
        else if(EA::StdC::Stristr(buffer, "Stabs"))
            expectedResults.mExpectedSymbolInfoType = kSymbolInfoTypeStabs;

        // Read mExpectedTargetOS
        EA::IO::ReadLine(&streamBuffer, buffer, EAArrayCount(buffer));

        if(EA::StdC::Stristr(buffer, "None"))
            expectedResults.mExpectedTargetOS = kTargetOSNone;
        else if(EA::StdC::Stristr(buffer, "360"))
            expectedResults.mExpectedTargetOS = kTargetOSXBox360;
        else if(EA::StdC::Stristr(buffer, "PS3CoreDump"))
            expectedResults.mExpectedTargetOS = kTargetOSPS3CoreDump;
        else if(EA::StdC::Stristr(buffer, "PS3"))
            expectedResults.mExpectedTargetOS = kTargetOSPS3;
        else if(EA::StdC::Stristr(buffer, "SPU"))
            expectedResults.mExpectedTargetOS = kTargetOSSPU;
        else if(EA::StdC::Stristr(buffer, "Wii"))
            expectedResults.mExpectedTargetOS = kTargetOSWii;
        else if(EA::StdC::Stristr(buffer, "Win32"))
            expectedResults.mExpectedTargetOS = kTargetOSWin32;
        else if(EA::StdC::Stristr(buffer, "Win64"))
            expectedResults.mExpectedTargetOS = kTargetOSWin64;
        else if(EA::StdC::Stristr(buffer, "MicrosoftMobile"))
            expectedResults.mExpectedTargetOS = kTargetOSMicrosoftMobile;
        else if(EA::StdC::Stristr(buffer, "MicrosoftConsole"))
            expectedResults.mExpectedTargetOS = kTargetOSMicrosoftConsole;
        else if(EA::StdC::Stristr(buffer, "Linux"))
            expectedResults.mExpectedTargetOS = kTargetOSLinux;
        else if(EA::StdC::Stristr(buffer, "OSX"))
            expectedResults.mExpectedTargetOS = kTargetOSOSX;
        else if(EA::StdC::Stristr(buffer, "IOS"))
            expectedResults.mExpectedTargetOS = kTargetOSIOS;
        else if(EA::StdC::Stristr(buffer, "Android"))
            expectedResults.mExpectedTargetOS = kTargetOSAndroid;
        else if(EA::StdC::Stristr(buffer, "Palm"))
            expectedResults.mExpectedTargetOS = kTargetOSPalm;
        else if(EA::StdC::Stristr(buffer, "Playbook"))
            expectedResults.mExpectedTargetOS = kTargetOSPlaybook;
        else if(EA::StdC::Stristr(buffer, "New"))
            expectedResults.mExpectedTargetOS = kTargetOSNew;

        // Read mExpectedTargetProcessor
        EA::IO::ReadLine(&streamBuffer, buffer, EAArrayCount(buffer));

        if(EA::StdC::Stristr(buffer, "None"))
            expectedResults.mExpectedTargetProcessor = kTargetProcessorNone;
        else if(EA::StdC::Stristr(buffer, "PowerPC32"))
            expectedResults.mExpectedTargetProcessor = kTargetProcessorPowerPC32;
        else if(EA::StdC::Stristr(buffer, "PowerPC64"))
            expectedResults.mExpectedTargetProcessor = kTargetProcessorPowerPC64;
        else if(EA::StdC::Stristr(buffer, "SPU"))
            expectedResults.mExpectedTargetProcessor = kTargetProcessorSPU;
        else if(EA::StdC::Stristr(buffer, "x86"))
            expectedResults.mExpectedTargetProcessor = kTargetProcessorX86;
        else if(EA::StdC::Stristr(buffer, "x64"))
            expectedResults.mExpectedTargetProcessor = kTargetProcessorX64;
        else if(EA::StdC::Stristr(buffer, "ARM"))
            expectedResults.mExpectedTargetProcessor = kTargetProcessorARM;

        // Read mAddressesToLookUp[8], mExpectedLookupResult[8], mExpectedLookupName[8][1024]
        int i = 0;
        while((i < kExpectedResultsCount) &&                                                              // While we have slots to read into and 
              (EA::IO::ReadLine(&streamBuffer, buffer, EAArrayCount(buffer)) < EA::IO::kSizeTypeDone) &&  // while there were more lines in the file to read from...
              (EA::StdC::Strlen(buffer) > 8))
        {
            EA::StdC::Sscanf(buffer, "%I64x %d %s", &expectedResults.mAddressesToLookUp[i], &expectedResults.mExpectedLookupResult[i], &expectedResults.mExpectedLookupName[i][0]);
            i++;
        }
        expectedResults.mResultsCount = i;

        fileStream.Close();
    }
    else
    {
        EA::UnitTest::Report("TestMapFile: Unable to open file %ls\n", sMapResultsFilePath.c_str());
        nErrorCount++;
    }

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestIndividualMapFile
//
static int TestIndividualMapFile(const eastl::wstring& sMapFilePath, 
                                 const eastl::wstring& sMapResultsFilePath, 
                                 EA::Allocator::ICoreAllocator* pCoreAllocator)
{
    using namespace EA::Callstack;

    int nErrorCount = ReadMapResultsFile(sMapResultsFilePath, gExpectedResults);

    if(nErrorCount == 0)
    {
        TargetOS        determinedTargetOS;
        TargetProcessor determinedTargetProcessor;
        SymbolInfoType  determinedType = GetSymbolInfoTypeFromDatabase(sMapFilePath.c_str(), &determinedTargetOS, &determinedTargetProcessor);

        EATEST_VERIFY(determinedTargetOS        == gExpectedResults.mExpectedTargetOS);
        EATEST_VERIFY(determinedTargetProcessor == gExpectedResults.mExpectedTargetProcessor);
        EATEST_VERIFY(determinedType            == gExpectedResults.mExpectedSymbolInfoType);

        if(determinedType == gExpectedResults.mExpectedSymbolInfoType)
        {
            for(int optionCount = 0; optionCount < 2; optionCount++) // We do test lookups under various options.
            {
                IAddressRepLookup* pLookup = MakeAddressRepLookupFromDatabase(sMapFilePath.c_str(), pCoreAllocator, determinedType);

                #if defined(EA_PLATFORM_DESKTOP)    // Only on desktop platforms are all types unilaterally supported.
                    EATEST_VERIFY(pLookup != NULL);
                #endif

                if(pLookup)
                {
                     pLookup->SetOption(IAddressRepLookup::kOptionOfflineLookup, 1);

                    // We test with the low memory usage option both off and on.
                    if(optionCount == 0)
                        pLookup->SetOption(IAddressRepLookup::kOptionLowMemoryUsage, 0);
                    else
                        pLookup->SetOption(IAddressRepLookup::kOptionLowMemoryUsage, 1);
    
                    bool bResult = pLookup->Init(sMapFilePath.c_str());
                    EATEST_VERIFY(bResult);

                    if(bResult)
                    {
                        FixedString strResults[kARTCount];
                        int         intResults[kARTCount];
                        bool        b64BitAddresses = false;
                        int         i;

                        for(i = 0; i < gExpectedResults.mResultsCount && !b64BitAddresses; i++)
                            b64BitAddresses = (gExpectedResults.mAddressesToLookUp[i] > 0xffffffff);

                        for(i = 0; i < gExpectedResults.mResultsCount; i++)
                        {
                            if(b64BitAddresses)
                                EA::UnitTest::ReportVerbosity(1, "Address: 0x%016I64x\n", (uint64_t)gExpectedResults.mAddressesToLookUp[i]);
                            else
                                EA::UnitTest::ReportVerbosity(1, "Address: 0x%08x\n", (uint32_t)gExpectedResults.mAddressesToLookUp[i]);

                            int count = pLookup->GetAddressRep(kARTFlagFunctionOffset | kARTFlagFileLine, gExpectedResults.mAddressesToLookUp[i], strResults, intResults);

                            if(count)
                            {
                                if(!strResults[kARTFunctionOffset].empty())
                                    EA::UnitTest::ReportVerbosity(1, "Function/Offset: %s %d\n", strResults[kARTFunctionOffset].c_str(), intResults[kARTFunctionOffset]);

                                if(!strResults[kARTFileLine].empty())
                                    EA::UnitTest::ReportVerbosity(1, "File/Line: %s %d\n\n", strResults[kARTFileLine].c_str(), intResults[kARTFileLine]);
                            }

                            EA::UnitTest::ReportVerbosity(1, "\n");

                            EATEST_VERIFY((count == 0) == (gExpectedResults.mExpectedLookupResult[i] == 0)); // We check like this because count is [0, 1, 2, 3] while mExpectedLookupResult is [0 or 1].
                            EATEST_VERIFY(EA::StdC::Strstr(strResults[kARTFunctionOffset].c_str(), gExpectedResults.mExpectedLookupName[i]) != NULL);

                            for(size_t j = 0; j < EAArrayCount(strResults); j++)
                            {
                                strResults[j].clear();
                                intResults[j] = 0;
                            }
                        }
                    }

                    DestroyAddressRepLookup(pLookup, pCoreAllocator);
                }
            }
        }
    }

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestMapFile
//
int TestMapFile()
{
    EA::UnitTest::ReportVerbosity(1, "\nTestMapFile\n");

    int nErrorCount = 0;

    {
        // Do data-driven tests.
        for(eastl::list<eastl::wstring>::iterator it = gTestFileList.begin(); it != gTestFileList.end(); ++it)
        {
            const eastl::wstring& sMapResultsFilePath = *it;

            if(sMapResultsFilePath.find(EA_WCHAR(".mapResults")) != eastl::wstring::npos) // For each .mapResults file...
            {
                const eastl::wstring sMapFilePath(sMapResultsFilePath, 0, sMapResultsFilePath.size() - 7); // Strip the trailing "Results" from the end of the name, leaving just .map

                if(EA::IO::File::Exists(sMapFilePath.c_str())) // This should always be true.
                {
                    // Test with default allocator
                    nErrorCount += TestIndividualMapFile(sMapFilePath, sMapResultsFilePath, EA::Allocator::ICoreAllocator::GetDefaultAllocator());

                    // Test with custom allocator, which does some additional verifications for us.
                    EATEST_VERIFY(gCustomCoreAllocator.mAllocationCount == 0);
                    EATEST_VERIFY(gCustomCoreAllocator.mFreeCount       == 0);
                    EA::Callstack::SetAllocator(&gCustomCoreAllocator);
                    nErrorCount += TestIndividualMapFile(sMapFilePath, sMapResultsFilePath, &gCustomCoreAllocator);
                    EATEST_VERIFY(gCustomCoreAllocator.mAllocationCount == gCustomCoreAllocator.mFreeCount);
                    gCustomCoreAllocator.mAllocationCount = 0;
                    gCustomCoreAllocator.mFreeCount       = 0;
                }
            }
        }
    }

    gCustomCoreAllocator.mAllocationCount = 0;
    gCustomCoreAllocator.mFreeCount       = 0;

    return nErrorCount;
}



#if 0
    #if 1 // defined(AUTHOR_PPEDRIANA) // defined(AUTHOR_PPEDRIANA) && defined(EA_PLATFORM_WINDOWS) // Disabled until we can have a more generic way of testing this.
    {
        IAddressRepLookup* pLookup      = NULL;
        bool               result       = false;
        const uint32_t     kEndValue    = 0xfefefefe;
        uint64_t           addrArray[8] = { kEndValue };

        #if 0 // EACALLSTACK_MSVC_MAPFILE_ENABLED
            MapFileMSVC mapFileMSVC;
            mapFileMSVC.SetOption(IAddressRepLookup::kOptionLowMemoryUsage, 1);
            pLookup = &mapFileMSVC;
            /*
            result  = pLookup->Init(L"D:\\Projects\\EAOS\\UTF\\build\\EAStdC\\dev\\xenon-vc-dev-debug\\bin\\test\\EAStdCTest.map");
            addrArray[0] = 0x82098770;
            addrArray[1] = 0x82186490;
            addrArray[2] = 0x821d0000;
            addrArray[3] = 0x00000000;
            addrArray[4] = 0xffffffff;
            addrArray[5] = kEndValue;
            */
            /*
            result  = pLookup->Init(L"D:\\Projects\\EAOS\\UTF\\build\\EAStdC\\dev\\pc64-vc-dev-debug\\bin\\test\\EAStdCTest.map");
            addrArray[0] = 0x0000000140002bb0;
            addrArray[1] = 0x0000000140143580;
            addrArray[2] = 0x00000001400e53f0;
            addrArray[3] = 0x000000014014a000;
            addrArray[4] = 0x0000000000000000;
            addrArray[5] = 0xffffffffffffffff;
            addrArray[6] = kEndValue;
            */
            result  = pLookup->Init(L"D:\\Projects\\EAOS\\UTF\\build\\EAStdC\\dev\\pc-vc-dev-debug\\bin\\test\\EAStdCTest.map");
            addrArray[0] = 0x00000000;
            addrArray[1] = 0xffffffff;
            addrArray[2] = 0x00402b10;
            addrArray[3] = 0x004dd248;
            addrArray[4] = 0x00502000;
            addrArray[5] = 0x00486280;
            addrArray[6] = kEndValue;
        #endif

        #if 0 // EACALLSTACK_GCC_MAPFILE_ENABLED
            MapFileGCC3 mapFileGCC;
            mapFileGCC.SetOption(IAddressRepLookup::kOptionLowMemoryUsage, 1);
            pLookup = &mapFileGCC;
            /*
            result  = pLookup->Init(L"D:\\Projects\\EAOS\\UTF\\build\\EAStdC\\dev\\cyg-gcc-dev-debug\\bin\\test\\EAStdCTest.map");
            addrArray[0] = 0x00000000;
            addrArray[1] = 0xffffffff;
            addrArray[2] = 0x004017bc;
            addrArray[3] = 0x00479940;
            addrArray[4] = 0x0048a000;
            addrArray[5] = 0x0044d56a;
            addrArray[6] = kEndValue;
            */
            result  = pLookup->Init(L"D:\\Projects\\EAOS\\UTF\\build\\EAStdC\\dev\\unix64-gcc-dev-debug\\bin\\test\\EAStdCTest.map");
            addrArray[0] = 0x0000000000000000;
            addrArray[1] = 0xffffffffffffffff;
            addrArray[2] = 0x00000000004028a0;
            addrArray[3] = 0x000000000046aa40;
            addrArray[4] = 0x0000000000403e4e;
            addrArray[5] = 0x000000000047ba40;
            addrArray[6] = kEndValue;
        #endif

        #if 0 // EACALLSTACK_SN_MAPFILE_ENABLED
            MapFileSN mapFileSN;
            mapFileSN.SetOption(IAddressRepLookup::kOptionLowMemoryUsage, 1);
            pLookup = &mapFileSN;
            result  = pLookup->Init(L"D:\\Projects\\EAOS\\UTF\\build\\EAStdC\\dev\\ps3-gcc-dev-debug\\bin\\test\\EAStdCTest.map");
            addrArray[0] = 0x00000000;
            addrArray[1] = 0xffffffff;
            addrArray[2] = 0x00011288;
            addrArray[3] = 0x002ad580;
            addrArray[4] = 0x002be300;
            addrArray[5] = 0x0024d1f8;
            addrArray[6] = kEndValue;
        #endif

        #if 0 // EACALLSTACK_CODEWARRIOR_MAPFILE_ENABLED
            MapFileCodeWarrior mapFileCW;
            mapFileCW.SetOption(IAddressRepLookup::kOptionLowMemoryUsage, 1);
            pLookup = &mapFileCW;
            result  = pLookup->Init(L"D:\\Projects\\EAOS\\UTF\\build\\EAStdC\\dev\\rev-mw-dev-debug\\bin\\test\\EAStdCTest.map");
            addrArray[0] = 0x8005d970;
            addrArray[1] = 0x8005d960;
            addrArray[2] = 0x80023140;
            addrArray[3] = 0x8009608f;
            addrArray[4] = 0x00000000;
            addrArray[5] = 0xffffffff;
            addrArray[6] = kEndValue;
        #endif

        #if EACALLSTACK_CODEWARRIOR_MAPFILE_ENABLED
            MapFileGreenHills mapFileGH;
            mapFileGH.SetOption(IAddressRepLookup::kOptionLowMemoryUsage, 0);
            pLookup = &mapFileGH;
            result  = pLookup->Init(L"C:\\projects\\EAOS\\UTF\\DL\\UTFDiagnostics\\EACallstack\\dev\\test\\data\\GreenHillsOptNumeric.map");
            addrArray[0] = 0x02000768; // Vsnprintf8__FPcUiPCcP10_va_struct
            addrArray[1] = 0x02006688; // WiiUWPADAlloc
            addrArray[2] = 0x02008258; // LastFunction
            addrArray[3] = 0x02000024; // FirstFunction
            addrArray[4] = 0x00000000;
            addrArray[5] = 0xffffffff;
            addrArray[6] = kEndValue;
        #endif

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
            FixedString strResults[kARTCount];
            int         intResults[kARTCount];
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

                count = pLookup->GetAddressRep(kARTFlagAddress | kARTFlagFunctionOffset | kARTFlagFileLine | kARTFlagSource, (void*)(uintptr_t)addrArray[i], strResults, intResults);

                if(count)
                {
                    if(!strResults[kARTFunctionOffset].empty())
                        EA::UnitTest::ReportVerbosity(1, "Function/Offset: %s %d\n", strResults[kARTFunctionOffset].c_str(), intResults[kARTFunctionOffset]);

                    if(!strResults[kARTFileLine].empty())
                        EA::UnitTest::ReportVerbosity(1, "File/Line: %s %d\n\n", strResults[kARTFileLine].c_str(), intResults[kARTFileLine]);
                }

                EA::UnitTest::ReportVerbosity(1, "\n");

                for(size_t j = 0; j < sizeof(strResults)/sizeof(strResults[0]); j++)
                {
                    strResults[j].clear();
                    intResults[j] = 0;
                }
            }
        }
    }
    #endif 
*/
#endif


