///////////////////////////////////////////////////////////////////////////////
// GHSFile.cpp
//
// Copyright (c) 2012 Electronic Arts Inc.
// Created by Paul Pedriana
//
// Implements reading of Green Hills ELF files, which have differing debug 
// information than DWARF-based ELF files.
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/GHSFile.h>

#if EACALLSTACK_GREENHILLS_GHSFILE_ENABLED

#include <EACallstack/EAAddressRep.h>
#include <EACallstack/EASymbolUtil.h>
#include <EACallstack/Allocator.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileDirectory.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/PathString.h>
#include <EAStdC/EAProcess.h>
#include <EAStdC/EASprintf.h>
#include EA_ASSERT_HEADER

#if defined(EA_PLATFORM_WINDOWS)
    #include <EACallstack/internal/Win32Runner.h>
#endif


namespace EA
{
namespace Callstack
{

///////////////////////////////////////////////////////////////////////////////
// GHSFile
//
GHSFile::GHSFile(EA::Allocator::ICoreAllocator* pCoreAllocator)
  : mbInitialized(false),\
    mpCoreAllocator(pCoreAllocator ? pCoreAllocator : EA::Callstack::GetAllocator()),
    msGHSFilePath(),
    mbOfflineLookup(true),  // Currently we support only offline lookups.
    mbRelocated(false),
    mBaseAddress(kBaseAddressUnspecified)
{
}


///////////////////////////////////////////////////////////////////////////////
// ~GHSFile
//
GHSFile::~GHSFile()
{
    Shutdown();
}


///////////////////////////////////////////////////////////////////////////////
// SetAllocator
//
void GHSFile::SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator)
{
    mpCoreAllocator = pCoreAllocator;
    // If there are any members that need to see this, tell them here.
}


///////////////////////////////////////////////////////////////////////////////
// Init
//
bool GHSFile::Init(const wchar_t* pGHSFilePath, bool /*bDelayLoad*/)
{
    if(!mbInitialized)
    {
        #if defined(EA_PLATFORM_WINDOWS) // Our running of gaddr2line.exe works only on Windows, as that's the only platform Green Hills provides a utility for.
            EA_ASSERT(pGHSFilePath);

            // Path to GHS .elf, .rpx, or .dnm file. // First four bytes of .dnm file must be: 0x7f 0x47 0x48 0x53
            msGHSFilePath = pGHSFilePath;

            if(EA::StdC::Strcmp(EA::IO::Path::GetFileExtension(pGHSFilePath), L".dnm") != 0) // If the extension is not .dnm (e.g. .elf or .rpx), change it. Lookups are done via a .dnm file only with Green Hills.
            {
                msGHSFilePath.assign(pGHSFilePath, EA::IO::Path::GetFileExtension(pGHSFilePath)); // Assing the path up to, but not including, the file extension.
                msGHSFilePath += L".dnm";
                // We will catch errors related to the .dnm file being non-existant below.
            }

            // Make sure we have an instance of gaddr2line.exe to use.
            {
                char16_t path16[EA::IO::kMaxPathLength];

                if(msAddr2LinePath.empty())
                {
                    // Look in this executable's directory.
                    EA::StdC::GetCurrentProcessDirectory(path16);
                    EA::StdC::Strlcat(path16, L"gaddr2line.exe", EAArrayCount(path16));

                    if(EA::IO::File::Exists(path16))
                        msAddr2LinePath = path16;
                }

                if(msAddr2LinePath.empty()) // If the above didn't find it...
                {
                    // Look recursively in the C:\ghs directory. (to-do: Look recursively in any ghs directory on the computer).
                    if(EA::IO::Directory::Exists("C:\\ghs\\"))
                    {
                        EA::IO::DirectoryIterator di;
                        EA::IO::DirectoryIterator::EntryList entryList;

                        // Recursively search for the gaddr2line.exe file.
                        size_t foundCount = di.ReadRecursive(L"C:\\ghs\\", entryList, L"gaddr2line.exe", EA::IO::kDirectoryEntryFile, false, true, 1, false);

                        if(foundCount)
                            msAddr2LinePath = entryList.front().msName.c_str();
                    }
                }

                if(msAddr2LinePath.empty()) // If the above didn't find it...
                {
                    // Look for {CAFE_SDK_ROOT}\ghs\multi539\gaddr2line.exe
                    size_t expectedStrlen = EA::StdC::GetEnvironmentVar(L"CAFE_SDK_ROOT", path16, EAArrayCount(path16));

                    if(expectedStrlen != (size_t)-1)
                    {
                        EA::IO::DirectoryIterator di;
                        EA::IO::DirectoryIterator::EntryList entryList;

                        // Recursively search for the gaddr2line.exe file.
                        EA::IO::Path::EnsureTrailingSeparator(path16, EAArrayCount(path16));
                        EA::StdC::Strlcat(path16, L"ghs\\", EAArrayCount(path16));
                        size_t foundCount = di.ReadRecursive(path16, entryList, L"gaddr2line.exe", EA::IO::kDirectoryEntryFile, false, true, 1, false);

                        if(foundCount)
                            msAddr2LinePath = entryList.front().msName.c_str();
                    }
                }

                if(msAddr2LinePath.empty()) // If the above didn't find it...
                {
                    // Look in this executable's current working directory.
                    EA::IO::Directory::GetCurrentWorkingDirectory(path16);
                    EA::StdC::Strlcat(path16, L"gaddr2line.exe", EAArrayCount(path16));

                    if(EA::IO::File::Exists(path16))
                        msAddr2LinePath = path16;
                }
            }

            if(EA::IO::File::Exists(msAddr2LinePath.c_str()) && EA::IO::File::Exists(msGHSFilePath.c_str()))
                mbInitialized = true;
            else
            {
              //msGHSFilePath.clear();      It's useful for debugging or possibly for user feedback to leave this value as-is.
                msAddr2LinePath.clear();
            }
        #else
            EA_UNUSED(pGHSFilePath);
        #endif
    }

    return mbInitialized;
}


///////////////////////////////////////////////////////////////////////////////
// Shutdown
//
bool GHSFile::Shutdown()
{
    if(mbInitialized)
    {
      //mpCoreAllocator = NULL;
        msGHSFilePath.clear();
        msAddr2LinePath.clear();
      //mbOfflineLookup = false;
      //mbRelocated     = false;
        mBaseAddress    = kBaseAddressUnspecified;
        mbInitialized   = false;
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// AsInterface
//
void* GHSFile::AsInterface(int id)
{
    if(id == GHSFile::kTypeId)
        return static_cast<GHSFile*>(this); // The cast isn't necessary, but makes the intent more clear.
    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// SetBaseAddress
//
void GHSFile::SetBaseAddress(uint64_t baseAddress)
{
    mBaseAddress = baseAddress;
}


///////////////////////////////////////////////////////////////////////////////
// GetBaseAddress
//
uint64_t GHSFile::GetBaseAddress() const
{
    return mBaseAddress;
}


///////////////////////////////////////////////////////////////////////////////
// GetDatabasePath
//
const wchar_t* GHSFile::GetDatabasePath() const
{
    return msGHSFilePath.c_str();
}


///////////////////////////////////////////////////////////////////////////////
// SetOption
//
void GHSFile::SetOption(int option, int value)
{
    if(option == kOptionOfflineLookup)
        mbOfflineLookup = (value != 0);

    // No other options are supported.
    // To consider: provide an option to set mbRelocated.
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressRep
//
int GHSFile::GetAddressRep(int addressRepTypeFlags, uint64_t nAddress, FixedString* pRepArray, int* pIntValueArray)
{
    int returnFlags = 0;

    #if defined(EA_PLATFORM_WINDOWS)
        if(mbInitialized)
        {
            // We don't want to adjust the address downward unless the module is relocatable (i.e. .rpx, .dll).
            // The reason is that nonrelocatable ELFs have the .text segment base address already accounted for in 
            // their map file information. 
            if(mbRelocated)
            {
                uint64_t baseAddressToUse = ((mBaseAddress == kBaseAddressUnspecified) ? 0 : mBaseAddress);
                nAddress -= baseAddressToUse;
            }

            returnFlags = GetAddressRepSpawn(addressRepTypeFlags, nAddress, pRepArray, pIntValueArray);
        }
    #else
        EA_UNUSED(addressRepTypeFlags);
        EA_UNUSED(nAddress);
        EA_UNUSED(pRepArray);
        EA_UNUSED(pIntValueArray);
    #endif

    return returnFlags;
}

///////////////////////////////////////////////////////////////////////////////
// GetAddressRep
//
int GHSFile::GetAddressRep(int addressRepTypeFlags, const void* pAddress, FixedString* pRepArray, int* pIntValueArray)
{
    return GetAddressRep(addressRepTypeFlags, (uint64_t)(uintptr_t)pAddress, pRepArray, pIntValueArray);
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressRepSpawn
//
// It is expected that the input nAddress has already been adjusted by 
// mBaseAddress by the caller.
//
int GHSFile::GetAddressRepSpawn(int addressRepTypeFlags, uint64_t nAddress, FixedString* pRepArray, int* pIntValueArray)
{
    int returnFlags = 0;

    #if defined(EA_PLATFORM_WINDOWS)

        // At this point msAddr2LinePath points to gaddr2line.exe. We use our Win32Runner
        // to run it with appropriate arguments, and look at the results. The app runs like
        // addr2line.exe and to get file/offset and file/line info we run it like so:
        //     gaddr2ine.exe -f -e <elf path> [0xAddr [0xAddr] ...] 
        // It prints output like so:
        //    EA::Messaging::Server::ProcessQueue()
        //    C:\Packages\EAMessage\dev\source\EAMessage.cpp:790
        // If it can't succeed, it prints output like so:
        //    ??
        //    ??:0
        // or if there is no .dnm file or an invalid .dnm file to go with the .elf file:
        //    gaddr2line: error: <libst>: failed while reading "C:\projects\EAOS\Cafe\build\EAStdC\dev-cafe\cafe-ghs-dev-debug\bin\test\EAStdCTest.dnm"
        //    gaddr2line: fatal: Could not open debug information for "C:\projects\EAOS\Cafe\build\EAStdC\dev-cafe\cafe-ghs-dev-debug\bin\test\EAStdCTest.elf"

        EA::Callstack::Win32Runner runner;
        int result;

        if(runner.Init())
        {
            EA::Callstack::CAString8 sOutput;
            sOutput.get_allocator().set_allocator(EA::Callstack::GetAllocator());
            sOutput.get_allocator().set_name(EACALLSTACK_ALLOC_PREFIX "EACallstack/Dwarf2File");

            wchar_t argumentString[512];

            if(nAddress <= 0xffffffff)
                EA::StdC::Sprintf(argumentString, L" -f -e \"%ls\" 0x%08x", msGHSFilePath.c_str(), (uint32_t)nAddress);
            else
                EA::StdC::Sprintf(argumentString, L" -f -e \"%ls\" 0x%016I64x", msGHSFilePath.c_str(), nAddress);

            runner.SetTargetProcessPath(msAddr2LinePath.c_str());
            runner.SetArgumentString(argumentString);
            runner.SetResponseTimeout(1500);
            runner.SetUnilateralTimeout(3000);
            result = runner.Run();
            sOutput.clear();
            runner.GetOutput(sOutput); 
            runner.Shutdown();

            if(result != 0)
                result = 0; // What can we do better than this?

            bool fileFormatRecognized = (sOutput.find("File format not recognized") == EA::Callstack::CAString8::npos);
            bool dnmReadSuccessful    = (sOutput.find("error: ") == EA::Callstack::CAString8::npos) && (sOutput.find(": Bad value") == EA::Callstack::CAString8::npos);

            if((result == 0) && fileFormatRecognized && dnmReadSuccessful)
            {
                // Process the output
                eastl_size_t i, iColon;

                // Replace mistaken "\/" sequences with just "\"
                while((i = sOutput.find("\\/")) != EA::Callstack::CAString8::npos)
                    sOutput.replace(i, 2, "\\");

                // If the output begins with a '.', simply remove it. Having it will prevent demangling of the following text.
                if(sOutput[0] == '.')
                    sOutput.erase(0, 1);

                // Get Function/Offset
                pRepArray[kARTFunctionOffset].clear();
                pIntValueArray[kARTFunctionOffset] = 0; // We don't seem to be able to get this.
                for(i = 0; sOutput[i] && (sOutput[i] != '\r') && (sOutput[i] != '\n'); i++)
                    pRepArray[kARTFunctionOffset] += sOutput[i];

                if(pRepArray[kARTFunctionOffset][0] && (pRepArray[kARTFunctionOffset][0] != '?'))
                    returnFlags |= (1 << kARTFunctionOffset);

                // Move to the next line of sOutput.
                while((sOutput[i] == '\r') || (sOutput[i] == '\n'))
                    i++;

                // Get File/Line
                pRepArray[kARTFileLine].clear();
                pIntValueArray[kARTFileLine] = 0;
                for(; sOutput[i] && (sOutput[i] != '\r') && (sOutput[i] != '\n'); i++)
                    pRepArray[kARTFileLine] += sOutput[i];
                iColon = pRepArray[kARTFileLine].rfind(':');
                if(iColon == FixedString::npos)
                    iColon = i;
                if(pRepArray[kARTFileLine].size() > iColon) // Sanity check.
                    pIntValueArray[kARTFileLine] = (int)EA::StdC::AtoI32(&pRepArray[kARTFileLine][iColon + 1]);
                pRepArray[kARTFileLine].resize(iColon);

                if(pRepArray[kARTFileLine][0] && (pRepArray[kARTFileLine][0] != '?'))
                    returnFlags |= (1 << kARTFileLine);
            }
        }

    #else
        EA_UNUSED(addressRepTypeFlags);
        EA_UNUSED(nAddress);
        EA_UNUSED(pRepArray);
        EA_UNUSED(pIntValueArray);
    #endif

    return returnFlags;
}



} // namespace Callstack
} // namespace EA


#endif // EACALLSTACK_GREENHILLS_GHSFILE_ENABLED





