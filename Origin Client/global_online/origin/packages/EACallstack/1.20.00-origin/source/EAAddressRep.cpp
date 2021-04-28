///////////////////////////////////////////////////////////////////////////////
// EAAddressRep.cpp
//
// Copyright (c) 2003-2005 Electronic Arts Inc.
// Created by Paul Pedriana
//
// This implementation is based on previous work by:
//     Vasyl Tsvirkunov
//     Avery Lee
//     Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/internal/Config.h>
#include <EACallstack/EAAddressRep.h>
#include <EACallstack/Allocator.h>
#include <EACallstack/MapFile.h>
#include <EACallstack/PDBFile.h>
#include <EACallstack/GHSFile.h>
#include <EACallstack/DWARF2File.h>
#include <EACallstack/EACallstack.h>
#include <EACallstack/EASymbolUtil.h>
#include <EACallstack/internal/ELF.h>
#include <coreallocator/icoreallocatormacros.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAScanf.h>
#include <EAStdC/EAMemory.h>
#include <EAStdC/EATextUtil.h>
#include <EAStdC/EAProcess.h>
#include <EAIO/PathString.h>
#include <EAIO/EAFileStream.h>
#include <EAIO/EAStreamAdapter.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileDirectory.h>
#include EA_ASSERT_HEADER
#include <stdio.h>
#include <new>

#if defined(EA_PLATFORM_XENON)
    #pragma warning(push, 0)
    #include <xtl.h>
    #if EA_XBDM_ENABLED
        #include <xbdm.h>
        // #pragma comment(lib, "xbdm.lib") The application is expected to provide this if needed. 
    #endif
    #pragma warning(pop)

#elif defined(EA_PLATFORM_MICROSOFT)
    #ifdef _MSC_VER
        #pragma warning(push, 0)
    #endif

    #include <Windows.h>

    #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
        #include <process.h>
        #include <Psapi.h>
        #include <ShellAPI.h>

        #ifdef _MSC_VER
            #pragma comment(lib, "dbghelp.lib")
            #pragma comment(lib, "psapi.lib")
            #pragma comment(lib, "shell32.lib") // Required for shellapi calls.
        #endif
    #endif

    #if defined(EA_PLATFORM_CONSOLE) && !defined(EA_PLATFORM_XENON)
        #define EACALLSTACK_MICROSOFT_MODULE_ENUMERATION_AVAILABLE 1

        #define EAGMI_DLL_NAME       L"toolhelpx.dll"               // GMI means GetModuleInformation
        #define EAGMI_ENUM_FUNC_NAME  "K32EnumProcessModules"
        #define EAGMI_GMI_FUNC_NAME   "K32GetModuleInformation"
        #define EAGMI_GMFN_FUNC_NAME  "K32GetModuleFileNameExW"
        #define EAGMI_GMBN_FUNC_NAME  "K32GetModuleBaseNameW"

        typedef struct _MODULEINFO { // This is normally defined in psapi.h for Windows
            LPVOID lpBaseOfDll;
            DWORD  SizeOfImage;
            LPVOID EntryPoint;
        } MODULEINFO, *LPMODULEINFO;
    
    #elif EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
        #define EACALLSTACK_MICROSOFT_MODULE_ENUMERATION_AVAILABLE 1

        #define EAGMI_DLL_NAME       L"psapi.dll"                   // GMI means GetModuleInformation
        #define EAGMI_ENUM_FUNC_NAME  "EnumProcessModules"
        #define EAGMI_GMI_FUNC_NAME   "GetModuleInformation"
        #define EAGMI_GMFN_FUNC_NAME  "GetModuleFileNameExW"
        #define EAGMI_GMBN_FUNC_NAME  "GetModuleBaseNameW"
    #endif

    typedef BOOL  (__stdcall *ENUMPROCESSMODULES)  (HANDLE hProcess, HMODULE* phModule, DWORD cb, LPDWORD lpcbNeeded);
    typedef DWORD (__stdcall *GETMODULEFILENAMEEX) (HANDLE hProcess, HMODULE   hModule, LPWSTR lpFilename, DWORD nSize);
    typedef DWORD (__stdcall *GETMODULEBASENAME)   (HANDLE hProcess, HMODULE   hModule, LPWSTR lpFilename, DWORD nSize);
    typedef BOOL  (__stdcall *GETMODULEINFORMATION)(HANDLE hProcess, HMODULE   hModule, MODULEINFO* pmi, DWORD nSize);

    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif

#elif defined(EA_PLATFORM_PS3)
    #include <sys/paths.h>
    #include <sys/prx.h>

#elif defined(EA_PLATFORM_KETTLE)
    #include <scebase_common.h>
    #include <kernel.h>

#elif (defined(__GNUC__) || defined(__clang__)) && (defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_OSX)) && !defined(__CYGWIN__) && !defined(EA_PLATFORM_ANDROID) && !defined(EA_PLATFORM_BADA) && !defined(EA_PLATFORM_PALM)
    #include <dlfcn.h>

    #define EACALLSTACK_GLIBC_DLFCN_AVAILABLE 1
#else
    #define EACALLSTACK_GLIBC_DLFCN_AVAILABLE 0
#endif

#if EACALLSTACK_GLIBC_BACKTRACE_AVAILABLE
    #include <signal.h>
    #include <execinfo.h>
#endif

#if defined(EA_PLATFORM_APPLE)
    #include <EACallstack/Apple/EACallstackApple.h>
#endif

#if defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_BSD)
    #include <sys/types.h>
    #include <unistd.h>
#endif

#if defined(EA_PLATFORM_OSX)
    // An alternative to _NSGetArgv which works on iOS in addition to OSX is NSProcessInfo.
    extern "C" char*** _NSGetArgv();
#elif defined(EA_PLATFORM_LINUX) && defined(EA_PLATFORM_DESKTOP)
    #include <stdio.h>
#endif


// EACALLSTACK_MICROSOFT_MODULE_ENUMERATION_AVAILABLE
//
// Defined as 0 or 1.
// Indicates whether Microsoft module enumeration and information functions are available for use.
//
#if !defined(EACALLSTACK_MICROSOFT_MODULE_ENUMERATION_AVAILABLE) // If not defined above...
    #define EACALLSTACK_MICROSOFT_MODULE_ENUMERATION_AVAILABLE 0
#endif


namespace EA
{
namespace Callstack
{

namespace ARLocal
{
    uint16_t SwizzleUint16(uint16_t x)
    {
        return (uint16_t) ((x >> 8) | (x << 8));
    }

    bool FileNamesMatch(const wchar_t* pFirst, const wchar_t* pSecond)
    {
        // The following is disabled because GetFileNameString is present only 
        // in EAIO 2.09.01 and later. Instead we use GetFileName below.
        //EA::IO::Path::PathString8 sFirst  = EA::IO::Path::GetFileNameString(pFirst);
        //EA::IO::Path::PathString8 sSecond = EA::IO::Path::GetFileNameString(pSecond);

        EA::IO::Path::PathStringW sFirst  = EA::IO::Path::GetFileName(pFirst);
        EA::IO::Path::PathStringW sSecond = EA::IO::Path::GetFileName(pSecond);

        sFirst.make_lower();
        sSecond.make_lower();

        sFirst.resize((eastl_size_t)(GetFileExtension(sFirst) - sFirst.begin()));
        sSecond.resize((eastl_size_t)(GetFileExtension(sSecond) - sSecond.begin()));

        return (sFirst == sSecond);
    }
}

///////////////////////////////////////////////////////////////////////////////
// GetCurrentProcessDirectory
//
EACALLSTACK_API size_t GetCurrentProcessDirectory(wchar_t* pDirectory)
{
    #if defined(EA_DEBUG)
        printf("EA::Callstack::GetCurrentProcessDirectory is deprecated. Please use EA::StdC::GetCurrentProcessDirectory instead. Contact EATechCoreSupport@ea.com if you have any questions.\n");
    #endif

    return EA::StdC::GetCurrentProcessDirectory(pDirectory);
}

///////////////////////////////////////////////////////////////////////////////
// GetCurrentProcessPath
//
EACALLSTACK_API size_t GetCurrentProcessPath(wchar_t* pPath)
{
    #if defined(EA_DEBUG)
        printf("EA::Callstack::GetCurrentProcessPath is deprecated. Please use EA::StdC::GetCurrentProcessPath instead. Contact EATechCoreSupport@ea.com if you have any questions.\n");
    #endif

    return EA::StdC::GetCurrentProcessPath(pPath);
}


///////////////////////////////////////////////////////////////////////////////
// AddressToString
//
EACALLSTACK_API CAString8& AddressToString(uint64_t address, CAString8& s, bool bLeading0x)
{
    const char* const pFormat64 = "0x%016" PRIx64;
    const char* const pFormat32 = "0x%08" PRIx64;
    const char*       pFormat   = (address > UINT32_MAX) ? pFormat64 : pFormat32;

    return static_cast<CAString8&>(s.sprintf(bLeading0x ? pFormat : pFormat + 2, address));
}


///////////////////////////////////////////////////////////////////////////////
// AddressToString
//
EACALLSTACK_API FixedString& AddressToString(uint64_t address, FixedString& s, bool bLeading0x)
{
    const char* const pFormat64 = "0x%016" PRIx64;
    const char* const pFormat32 = "0x%08" PRIx64;
    const char*       pFormat   = (address > UINT32_MAX) ? pFormat64 : pFormat32;

    return static_cast<FixedString&>(s.sprintf(bLeading0x ? pFormat : pFormat + 2, address));
}




///////////////////////////////////////////////////////////////////////////////
// AddressRepCache accessor
///////////////////////////////////////////////////////////////////////////////

static AddressRepCache* gpAddressRepCache = NULL;


///////////////////////////////////////////////////////////////////////////////
// GetAddressRepCache
//
AddressRepCache* GetAddressRepCache()
{
    return gpAddressRepCache;
}


///////////////////////////////////////////////////////////////////////////////
// SetAddressRepCache
//
AddressRepCache* SetAddressRepCache(AddressRepCache* pAddressRepCache)
{   
    AddressRepCache* const pAddressRepCacheSaved = gpAddressRepCache;
    gpAddressRepCache = pAddressRepCache;

    return pAddressRepCacheSaved;
}




///////////////////////////////////////////////////////////////////////////////
// AddressRepEntry
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// AddressRepEntry
//
AddressRepEntry::AddressRepEntry(EA::Allocator::ICoreAllocator* pCoreAllocator)
  : mARTCheckFlags(0),
    mFunctionOffset(0),
    mFileOffset(0)
{
    if(!pCoreAllocator)
        pCoreAllocator = EA::Callstack::GetAllocator();

    for(size_t i = 0; i < EAArrayCount(mAddressRep); i++)
    {
        mAddressRep[i].get_allocator().set_allocator(pCoreAllocator);
        mAddressRep[i].get_allocator().set_name(EACALLSTACK_ALLOC_PREFIX "EACallstack/AddressRepEntry");
    }
}


///////////////////////////////////////////////////////////////////////////////
// SetAllocator
//
void AddressRepEntry::SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator)
{
    for(size_t i = 0; i < EAArrayCount(mAddressRep); i++)
        mAddressRep[i].get_allocator().set_allocator(pCoreAllocator);
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleBaseAddress
//
EACALLSTACK_API uint64_t GetModuleBaseAddress(const wchar_t* pModuleName)
{
    #if EACALLSTACK_MICROSOFT_MODULE_ENUMERATION_AVAILABLE
        HANDLE  hProcess = GetCurrentProcess();
        HMODULE hPSAPI   = LoadLibraryW(EAGMI_DLL_NAME);

        if(hPSAPI)
        {
            ENUMPROCESSMODULES   pEnumProcessModules   = (ENUMPROCESSMODULES)  (void*)GetProcAddress(hPSAPI, EAGMI_ENUM_FUNC_NAME);
            GETMODULEFILENAMEEX  pGetModuleFileNameEx  = (GETMODULEFILENAMEEX) (void*)GetProcAddress(hPSAPI, EAGMI_GMFN_FUNC_NAME);
            GETMODULEBASENAME    pGetModuleBaseName    = (GETMODULEBASENAME)   (void*)GetProcAddress(hPSAPI, EAGMI_GMBN_FUNC_NAME);
            GETMODULEINFORMATION pGetModuleInformation = (GETMODULEINFORMATION)(void*)GetProcAddress(hPSAPI, EAGMI_GMI_FUNC_NAME);

            const size_t kModuleCapacity = 256;
            HMODULE      hModuleBuffer[kModuleCapacity];
            DWORD        cbNeeded = 0;
            wchar_t      pBaseName[MAX_PATH]; pBaseName[0] = 0;
            wchar_t      pFullName[MAX_PATH]; pFullName[0] = 0;
            MODULEINFO   mi;

            if(pEnumProcessModules(hProcess, hModuleBuffer, sizeof(hModuleBuffer), &cbNeeded))
            {
                const size_t moduleCount = ((cbNeeded / sizeof(HMODULE)) < kModuleCapacity) ? (cbNeeded / sizeof(HMODULE)) : kModuleCapacity;

                // Run the first loop checking for perfect full path matches
                for(size_t i = 0; i < moduleCount; i++)
                {
                    EA::StdC::Memset8(&mi, 0, sizeof(mi));

                    pGetModuleInformation(hProcess, hModuleBuffer[i], &mi, sizeof(mi));
                    pGetModuleBaseName   (hProcess, hModuleBuffer[i], pBaseName, EAArrayCount(pBaseName));
                    pGetModuleFileNameEx (hProcess, hModuleBuffer[i], pFullName, EAArrayCount(pFullName));

                    if(ARLocal::FileNamesMatch(pFullName, pModuleName))
                        return (uint64_t)(uintptr_t)mi.lpBaseOfDll;
                }

                // Run the second loop checking for just file name matches.
                for(size_t i = 0; i < moduleCount; i++)
                {
                    EA::StdC::Memset8(&mi, 0, sizeof(mi));

                    pGetModuleInformation(hProcess, hModuleBuffer[i], &mi, sizeof(mi));
                    pGetModuleBaseName   (hProcess, hModuleBuffer[i], pBaseName, EAArrayCount(pBaseName));
                    pGetModuleFileNameEx (hProcess, hModuleBuffer[i], pFullName, EAArrayCount(pFullName));

                    if(ARLocal::FileNamesMatch(pBaseName, pModuleName)) // If pModuleName is found in pBaseName...
                        return (uint64_t)(uintptr_t)mi.lpBaseOfDll;

                    if(ARLocal::FileNamesMatch(pFullName, pModuleName)) // If pModuleName is found in pFullName...
                        return (uint64_t)(uintptr_t)mi.lpBaseOfDll;
                }

                return kBaseAddressInvalid;
            }
        }

        // Try just the current module.
        return GetModuleBaseAddressByHandle(kModuleHandleInvalid);

    #else
        char moduleFileName8[EA::IO::kMaxPathLength];
        int  result = EA::StdC::Strlcpy(moduleFileName8, pModuleName, EAArrayCount(moduleFileName8));

        if((result >= 0) && (result < (int)EAArrayCount(moduleFileName8)))
            return GetModuleBaseAddress(moduleFileName8);

        return kBaseAddressInvalid;
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleBaseAddress
//
EACALLSTACK_API uint64_t GetModuleBaseAddress(const char* pModuleName)
{
    #if EACALLSTACK_MICROSOFT_MODULE_ENUMERATION_AVAILABLE
        wchar_t moduleFileNameW[EA::IO::kMaxPathLength];
        int     result = EA::StdC::Strlcpy(moduleFileNameW, pModuleName, EAArrayCount(moduleFileNameW));

        if((result >= 0) && result < EAArrayCount(moduleFileNameW))
            return GetModuleBaseAddress(moduleFileNameW);

        return kBaseAddressInvalid;
    #else
        if(!pModuleName)
            return GetModuleBaseAddressByHandle(kModuleHandleInvalid);

        // Get just the file name (without any extension) of pModuleName. 
        // If pModuleName is already a file name or looks like a file name 
        // then pModuleFileName will be the same as pModuleName.
        char pModuleFileName[EA::IO::kMaxPathLength];

        EA::StdC::Strcpy(pModuleFileName, EA::IO::Path::GetFileName(pModuleName));
        *EA::IO::Path::GetFileExtension(pModuleFileName) = 0;

        #if defined(EA_PLATFORM_XENON)

            // 0x82000000 is the default value for a Xenon executable. Xenon DLLs will have a 
            // different value and XBDM must be enabled in order to get the base address of DLLs.
            uint64_t result = 0x82000000; 

            #if EA_XBDM_ENABLED
                PDM_WALK_MODULES pWalkMod = NULL;
                DMN_MODLOAD      ModLoad;

                while(DmWalkLoadedModules(&pWalkMod, &ModLoad) == XBDM_NOERR)
                {
                    if(EA::StdC::Stristr(ModLoad.Name, pModuleFileName) ||  // If pModuleFileName is found in ModLoad.Name...
                       EA::StdC::Stristr(ModLoad.Name, pModuleName))
                    {
                        result = (uint64_t)(uintptr_t)ModLoad.BaseAddress;
                        break;
                    }
                }

                DmCloseLoadedModules(pWalkMod);
            #endif

            return result;

        #elif defined(EA_PLATFORM_PS3)

            // The PS3 module functions don't handle the main executable itself. So we need to add
            // special handling for the main executable. All other modules, including system modules like liblv2, are supported.
            sys_prx_id_t id = sys_prx_get_module_id_by_name(pModuleName, 0, NULL); 

            if(id >= CELL_OK)
                return GetModuleBaseAddressByHandle(id);

            // Look to see if pModuleName refers to the main executable itself.
            char8_t processPath[EA::IO::kMaxPathLength];
            if(EA::StdC::GetCurrentProcessPath(processPath, (int)EAArrayCount(processPath)))
            {
                const char8_t* pFileName = EA::IO::Path::GetFileName(processPath);
                if(EA::StdC::Stricmp(pFileName, pModuleName) == 0)
                    return 0; // The base address of the main executable on this platform is 0.

                *EA::IO::Path::GetFileExtension(pFileName) = 0;
                if(EA::StdC::Stricmp(pFileName, pModuleName) == 0)
                    return 0; // The base address of the main executable on this platform is 0.
            }

            // We walk through all the current modules and see if any matches the name or file name.
            sys_prx_id_t              idArray[48];
            sys_prx_get_module_list_t moduleList;

            moduleList.size   = sizeof(sys_prx_get_module_list_t);
            moduleList.max    = sizeof(idArray)/sizeof(idArray[0]);
            moduleList.count  = 0;
            moduleList.idlist = idArray;

            if(sys_prx_get_module_list(0, &moduleList) == 0)
            {
                char                   fileNameBuffer[256];
                sys_prx_segment_info_t segmentInfo[2];
                sys_prx_module_info_t  info;

                for(uint32_t i = 0; i < moduleList.count; i++)
                {
                    info.size          = sizeof(sys_prx_module_info_t);
                    info.filename      = fileNameBuffer;
                    info.filename_size = sizeof(fileNameBuffer)/sizeof(fileNameBuffer[0]);
                    info.segments      = segmentInfo;
                    info.segments_num  = sizeof(segmentInfo)/sizeof(segmentInfo[0]);

                    if(sys_prx_get_module_info(idArray[i], 0, &info) == 0)
                    {
                        if(EA::StdC::Stristr(info.name,     pModuleName) ||     // "cellPrxCallPrxExport"
                           EA::StdC::Stristr(info.filename, pModuleName))       // "/app_home/prx/call-prx-export.sprx"
                        {
                            if(info.segments_num) // This should always be true.
                                return info.segments[0].base;
                        }
                    }
                }
            }

            return kBaseAddressInvalid;

        #elif defined(EA_PLATFORM_KETTLE)
            // We walk through all the current modules and see if any matches the name or file name.
            SceKernelModule sceKernelModuleArray[96]; // SceKernelModule is a handle type. We declare on the stack because we don't want to allocate memory.
            size_t          arrayCount = 0;

            int result = sceKernelGetModuleList(sceKernelModuleArray, EAArrayCount(sceKernelModuleArray), &arrayCount);

            if(result == SCE_OK)
            {
                SceKernelModuleInfo moduleInfo;

                for(size_t i = 0; i < arrayCount; i++)
                {
                    moduleInfo.size = sizeof(moduleInfo);
                    result = sceKernelGetModuleInfo(sceKernelModuleArray[i], &moduleInfo);

                    if(result == SCE_OK)
                    {
                        if(EA::StdC::Stristr(moduleInfo.name, pModuleName))     // "scePrxCallPrxExport" or "/app_home/prx/call-prx-export.sprx"
                        {
                            if(moduleInfo.numSegments) // This should always be true.
                                return reinterpret_cast<uint64_t>(moduleInfo.segmentInfo[0].baseAddr); // We really want to use the first segment that has (moduleInfo.segmentInfo[i].prot & SCE_KERNEL_PROT_CPU_EXEC), but the first always has what we want.
                        }
                    }
                }
            }

            return kBaseAddressInvalid;

        #elif defined(EA_PLATFORM_CAFE)

            // Walk the list of modules and see which one matches this one, then return its base address.
            EA_UNUSED(pModuleName);
            return kBaseAddressInvalid;

        #elif defined(EA_PLATFORM_REVOLUTION)

            if(RSOModuleInfoManager::sRSOModuleInfoManager) // If the user registered a module info manager...
            {
                const RSOModuleInfo* pModule = RSOModuleInfoManager::sRSOModuleInfoManager->GetRSOModuleInfo(pModuleName, NULL, NULL);

                if(pModule)
                    return pModule->mModuleTextOffset;
            }

            return kBaseAddressInvalid;

        #elif defined(EA_PLATFORM_APPLE)
            ModuleInfoAppleArray moduleInfoAppleArray;
            size_t               count = GetModuleInfoApple(moduleInfoAppleArray);
        
            for(size_t i = 0; i < count; i++)
            {
                const EA::Callstack::ModuleInfoApple& info = moduleInfoAppleArray[(eastl_size_t)i];
            
                if(EA::StdC::Stristr(info.mPath.c_str(), pModuleName)) // This is an imperfect comparison.
                    return info.mBaseAddress;
            }
        
            // To consider: Try using dlopen(pModuleName) like done below.
            return kBaseAddressInvalid;
        
        #elif defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_BSD)

            // On Unix platforms, we need to search for the module in the process map file
            // (proc/[pid]/maps). The process map lists all modules for the current process ID in the
            // following format:
            //
            // address           perm offset   dev   inode   pathname
            // 08048000-08056000 r-xp 00000000 03:0c 64593   /lib/mylib.so

            size_t       result = kBaseAddressInvalid;
            char         mapPath[EA::IO::kMaxPathLength] = "";
            char         mapEntry[72 + EA::IO::kMaxPathLength] = ""; // This should be enough for all possible paths that use up to kMaxPathLength.
            const size_t moduleNameLen = EA::StdC::Strlen(pModuleName);

            // Open the process map file.
            EA::StdC::Snprintf(mapPath, sizeof(mapPath), "/proc/%d/maps", getpid());
            FILE* pMapFile = fopen(mapPath, "rt");

            // If the open succeeded, parse the file.
            // The fopen above may fail if /proc/ was not mounted (http://stackoverflow.com/questions/5486317/could-not-find-proc-self-maps), at least on BSD.
            if (pMapFile != NULL)
            {
                // Search all map entries for the requested module.
                while (fgets(mapEntry, sizeof(mapEntry), pMapFile) != NULL)
                {
                    size_t mapEntryLen = EA::StdC::Strlen(mapEntry);

                    // Add a null terminator to the end of the entry.
                    if ((mapEntryLen > 0) && (mapEntry[mapEntryLen-1] == '\n'))
                        mapEntry[--mapEntryLen] = '\0';

                    // If this is not the requested module entry (case-sensitive compare), skip it.
                    if ((mapEntryLen >= moduleNameLen) && (EA::StdC::Memcmp(mapEntry + mapEntryLen - moduleNameLen, pModuleName, moduleNameLen) == 0))
                    {
                        size_t start = 0;
                        size_t end = 0;
                        size_t offset = 0;
                        char   flags[4] = { 0 };

                        // Scan the module information from the map entry (e.g "08048000-08056000 r-xp 00000000 03:0c 64593   /lib/mylib.so")
                        if (EA::StdC::Sscanf(mapEntry, "%zx-%zx %c%c%c%c %zx", &start, &end, &flags[0], &flags[1], &flags[2], &flags[3], &offset) == 7)
                        {
                            // Validate the module flags. If there are multiple entries for the module, we want
                            // the one with r-x permissions.
                            if ((flags[0] == 'r') && (flags[2] == 'x'))
                            {
                                // Calculate the module base address.
                                result = start - offset;

                                // The module was found. Stop searching.
                                break;
                            }
                            // Else this entry didn't have r-x (read/execute) permissions.
                        }
                        //Else ignore the line.
                    }
                    // Else the name doesn't match.
                }

                // Close the file before returning the result.
                fclose(pMapFile);
            }

            return static_cast<uint64_t>(result);

        #elif EACALLSTACK_GLIBC_DLFCN_AVAILABLE

            // I think this isn't possible on any form of Unix, since there isn't a way 
            // to iterate modules aside from spawning a shell process. If moduleName is 
            // a file name then we can call dlopen on it.

            uint64_t result = kBaseAddressInvalid;
            void*    handle = dlopen(pModuleName, RTLD_LAZY);

            if(handle)
            {
                Dl_info info;

                // I'm not sure this is right; Need to test it.
                if(dladdr(handle, &info) != 0) // dladdr is an extension function, but all Unix-based glibc implementations seem to have it, including Mac OS X.
                    result = (uint64_t)(uintptr_t)info.dli_fbase;

                dlclose(handle);
            }

            return result;

        #else
            EA_UNUSED(pModuleName);
            return kBaseAddressInvalid;
        #endif
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleBaseAddressByHandle
//
EACALLSTACK_API uint64_t GetModuleBaseAddressByHandle(ModuleHandle moduleHandle)
{
    // Windows and XBox 360 define the __ImageBase pseudo-variable which you can use.
    // This would be a little better than using GetModuleHandle(NULL).
    // This:
    //     EXTERN_C IMAGE_DOS_HEADER __ImageBase;
    //     #define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
    // or this:
    //     MEMORY_BASIC_INFORMATION mbi;
    //     VirtualQuery(GetModuleBaseAddressByHandle, &mbi, sizeof(mbi));
    //     HMODULE hModule = (HMODULE)mbi.AllocationBase; 
    // or this:
    //     GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, GetModuleBaseAddressByHandle, &hModule);

    #if EACALLSTACK_MICROSOFT_MODULE_ENUMERATION_AVAILABLE
        HMODULE hPSAPI = LoadLibraryW(EAGMI_DLL_NAME);

        if(hPSAPI)
        {
            if(moduleHandle == kModuleHandleInvalid)
                moduleHandle = GetModuleHandle(NULL); // Get current executable module handle.

            GETMODULEINFORMATION pGetModuleInformation = (GETMODULEINFORMATION)(void*)GetProcAddress(hPSAPI, EAGMI_GMI_FUNC_NAME);

            if(pGetModuleInformation)
            {
                MODULEINFO moduleInfo;
                pGetModuleInformation(GetCurrentProcess(), (HMODULE)moduleHandle, &moduleInfo, sizeof(moduleInfo));
                return (uint64_t)moduleInfo.lpBaseOfDll; // A typical value for this under Win32 is 0x00400000.
            }
        }

        return kBaseAddressInvalid;

    #elif defined(EA_PLATFORM_XENON)

        // Is a module handle the same as a base address on this platform? 
        // If not, then how do you convert a module handle to a base address?
        if(moduleHandle)
            return (uint64_t)(uintptr_t)moduleHandle;
        return 0x82000000;

    #elif defined(EA_PLATFORM_PS3)

        // The PS3 module functions don't handle the main executable itself. So we need to add
        // special handling for the main executable. All other modules, including system modules like liblv2, are supported.
        if(moduleHandle == kModuleHandleInvalid) // If referring to the main executable...
            return kBaseAddressInvalid; // return the base address of the main executable.

        char                   fileNameBuffer[256];
        sys_prx_segment_info_t segmentInfo[1];          // It happens that the .text segment seems to always be the first segment.
        sys_prx_module_info_t  info;

        info.size          = sizeof(info);
        info.filename      = fileNameBuffer;
        info.filename_size = sizeof(fileNameBuffer)/sizeof(fileNameBuffer[0]);
        info.segments      = segmentInfo;
        info.segments_num  = sizeof(segmentInfo)/sizeof(segmentInfo[0]);

        int result = sys_prx_get_module_info(moduleHandle, 0, &info);

        if((result == 0) && (info.segments_num >= 1))
            return info.segments[0].base;

        return kBaseAddressInvalid;

    #elif defined(EA_PLATFORM_KETTLE)
        SceKernelModuleInfo moduleInfo;

        moduleInfo.size = sizeof(moduleInfo);
        int32_t result = sceKernelGetModuleInfo(moduleHandle, &moduleInfo);

        if((result == SCE_OK) && moduleInfo.numSegments)
            return reinterpret_cast<uint64_t>(moduleInfo.segmentInfo[0].baseAddr);  // We really want to use the first segment that has (moduleInfo.segmentInfo[i].prot & SCE_KERNEL_PROT_CPU_EXEC), but the first always has what we want.

        return kBaseAddressInvalid;

    #elif defined(EA_PLATFORM_REVOLUTION)

        RSOModuleInfo moduleInfo;
        RSOModuleInfoManager::ReadRSOModuleInfo(NULL, moduleHandle, moduleInfo);
        return moduleInfo.mModuleTextOffset;
    #elif defined(EA_PLATFORM_APPLE) || defined(EA_PLATFORM_UNIX)
        return (uint64_t)(uintptr_t)moduleHandle;

    #else
        (void)moduleHandle;
        return kBaseAddressInvalid;
    #endif
}


EACALLSTACK_API uint64_t GetModuleBaseAddressByAddress(const void* pCodeAddress)
{
    #if defined(EA_PLATFORM_MICROSOFT) && !defined(EA_PLATFORM_XENON) 
        MEMORY_BASIC_INFORMATION mbi;

        if(VirtualQuery(pCodeAddress, &mbi, sizeof(mbi)))
            return (uint64_t)(uintptr_t)mbi.AllocationBase;

        return kBaseAddressInvalid;

    #elif defined(EA_PLATFORM_XENON)
        // Walk through all modules, find which one has the given address.
        // 0x82000000 is the default value for a Xenon executable. Xenon DLLs will have a 
        // different value and XBDM must be enabled in order to get the base address of DLLs.
        uint64_t result = 0x82000000; 

        #if EA_XBDM_ENABLED
            PDM_WALK_MODULES pWalkMod = NULL;
            DMN_MODLOAD      ModLoad;

            while(DmWalkLoadedModules(&pWalkMod, &ModLoad) == XBDM_NOERR)
            {
                if(((uintptr_t)ModLoad.BaseAddress <=  (uintptr_t)pCodeAddress) && 
                   ((uintptr_t)pCodeAddress        <  ((uintptr_t)ModLoad.BaseAddress + ModLoad.Size)))
                {
                    result = (uint64_t)(uintptr_t)ModLoad.BaseAddress;
                    break;
                }
            }

            DmCloseLoadedModules(pWalkMod);
        #endif

        return result;


    #elif defined(EA_PLATFORM_PS3)

        // The PS3 conveniently provides a function to do this for us.
        // Otherwise we could use the other prx functions to manually find the prx associated with this code address.
        sys_prx_id_t id = sys_prx_get_module_id_by_address((void*)pCodeAddress);

        if(id >= 0) // The returned id is 0 for the main executable, which is actually not a documented return value. But it works for us.
            return GetModuleBaseAddressByHandle(id);

        return kBaseAddressInvalid;

    #elif defined(EA_PLATFORM_KETTLE)
        // We walk through all the current modules and see if any matches the address.
        SceKernelModule sceKernelModuleArray[96]; // SceKernelModule is a handle type. We declare on the stack because we don't want to allocate memory.
        size_t          arrayCount = 0;

        int result = sceKernelGetModuleList(sceKernelModuleArray, EAArrayCount(sceKernelModuleArray), &arrayCount);

        if(result == SCE_OK)
        {
            SceKernelModuleInfo moduleInfo;

            for(size_t i = 0; i < arrayCount; i++)
            {
                moduleInfo.size = sizeof(moduleInfo);
                result = sceKernelGetModuleInfo(sceKernelModuleArray[i], &moduleInfo);

                if(result == SCE_OK)
                {
                    for(uint32_t s = 0; s < moduleInfo.numSegments; s++)  // We really want to use the first segment that has (moduleInfo.segmentInfo[s].prot & SCE_KERNEL_PROT_CPU_EXEC), but the first always has what we want.
                    {
                        if(((uintptr_t)pCodeAddress >= (uintptr_t)moduleInfo.segmentInfo[s].baseAddr) && 
                           ((uintptr_t)pCodeAddress < ((uintptr_t)moduleInfo.segmentInfo[s].baseAddr + (uintptr_t)moduleInfo.segmentInfo[s].size)))
                        {
                             // This should always be true.
                            return reinterpret_cast<uint64_t>(moduleInfo.segmentInfo[s].baseAddr);
                        }
                   }
                }
            }
        }

        return kBaseAddressInvalid;

    #elif defined(EA_PLATFORM_CAFE)

        // Walk the list of modules and see which one matches this one, then return its base address.
        (void)pCodeAddress;
        return kBaseAddressInvalid;

    #elif defined(EA_PLATFORM_REVOLUTION)

        if(RSOModuleInfoManager::sRSOModuleInfoManager) // If the user registered a module info manager...
        {
            RSOModuleInfoManager* pManager = RSOModuleInfoManager::sRSOModuleInfoManager;

            const RSOModuleInfo* pModule = pManager->GetRSOModuleInfo(NULL, NULL, pCodeAddress);

            if(pModule)
                return pModule->mModuleTextOffset;
        }

        return kBaseAddressInvalid;

    #elif defined(EA_PLATFORM_APPLE)
        ModuleInfoAppleArray moduleInfoAppleArray;
        size_t               count = GetModuleInfoApple(moduleInfoAppleArray);
        uint64_t             codeAddress = (uint64_t)(uintptr_t)pCodeAddress;
        
        for(size_t i = 0; i < count; i++)
        {
            const EA::Callstack::ModuleInfoApple& info = moduleInfoAppleArray[(eastl_size_t)i];
            
            if((info.mBaseAddress < codeAddress) && (codeAddress < (info.mBaseAddress + info.mSize)))
                return info.mBaseAddress;
        }
        
        return kBaseAddressInvalid;
 
    #elif EACALLSTACK_GLIBC_DLFCN_AVAILABLE

        // typedef struct {
        //   const char* dli_fname;     /* Filename of defining object   */
        //   void*       dli_fbase;     /* Load address of that object   */
        //   const char* dli_sname;     /* Name of nearest lower symbol  */
        //   void*       dli_saddr;     /* Exact value of nearest symbol */
        // } Dl_info;
        //
        // int dladdr(void *addr, Dl_info *info);

        Dl_info info;

        if(dladdr(pCodeAddress, &info) != 0) // dladdr is an extension function, but all Unix-based glibc implementations seem to have it, including Mac OS X.
            return (uint64_t)(uintptr_t)info.dli_fbase;

        return kBaseAddressInvalid;

    #else

        (void)pCodeAddress;
        return kBaseAddressInvalid;

    #endif
}

bool operator==(const ModuleInfo& lhs, const ModuleInfo& rhs)
{
    #if EA_FILE_SYSTEM_CASE_SENSITIVE
        return (lhs.mSize == rhs.mSize && 
                lhs.mBaseAddress == rhs.mBaseAddress &&
                lhs.mName == rhs.mName &&
                lhs.mPath == rhs.mPath);
    #else
        return (lhs.mSize == rhs.mSize && 
                lhs.mBaseAddress == rhs.mBaseAddress &&
                lhs.mName.comparei(rhs.mName) == 0 && // This assumes the module name and path are
                lhs.mPath.comparei(rhs.mPath) == 0);   // using Western characters only.
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// GetModuleInfo
//
EACALLSTACK_API size_t GetModuleInfo(ModuleInfo* pModuleInfoArray, size_t moduleArrayCapacity)
{
    #if EACALLSTACK_MICROSOFT_MODULE_ENUMERATION_AVAILABLE
        size_t  moduleCount = 0;
        HANDLE  hProcess    = GetCurrentProcess();
        HMODULE hPSAPI      = LoadLibraryW(EAGMI_DLL_NAME);

        if(hPSAPI)
        {
            ENUMPROCESSMODULES   pEnumProcessModules   = (ENUMPROCESSMODULES)  (void*)GetProcAddress(hPSAPI, EAGMI_ENUM_FUNC_NAME);
            GETMODULEINFORMATION pGetModuleInformation = (GETMODULEINFORMATION)(void*)GetProcAddress(hPSAPI, EAGMI_GMI_FUNC_NAME);

            const size_t kModuleCapacity = 256;
            HMODULE      hModuleBuffer[kModuleCapacity];
            DWORD        cbNeeded = 0;
            MODULEINFO   mi;

            if(pEnumProcessModules(hProcess, hModuleBuffer, sizeof(hModuleBuffer), &cbNeeded))
            {
                moduleCount = ((cbNeeded / sizeof(HMODULE)) < kModuleCapacity) ? (cbNeeded / sizeof(HMODULE)) : kModuleCapacity;

                if(pModuleInfoArray)
                {
                    if(moduleCount > moduleArrayCapacity)
                        moduleCount = moduleArrayCapacity;

                    for(size_t i = 0; i < moduleCount; i++)
                    {
                        ModuleInfo& moduleInfo = pModuleInfoArray[i];

                        EA::StdC::Memset8(&mi, 0, sizeof(mi));
                        BOOL result = pGetModuleInformation(hProcess, hModuleBuffer[i], &mi, sizeof(mi));

                        if(result)
                        {
                            wchar_t pathW[EA::IO::kMaxPathLength]; pathW[0] = 0;
                            char8_t path8[EA::IO::kMaxPathLength];

                            moduleInfo.mModuleHandle = hModuleBuffer[i];
                            GetModuleFileNameW(hModuleBuffer[i], pathW, EAArrayCount(pathW));
                            EA::StdC::Strlcpy(path8, pathW, EAArrayCount(path8));
                            moduleInfo.mPath        = path8;
                            *EA::IO::Path::GetFileExtension(path8) = 0;
                            moduleInfo.mName        = EA::IO::Path::GetFileName(path8);
                            moduleInfo.mBaseAddress = (uint64_t)(uintptr_t)mi.lpBaseOfDll;
                            moduleInfo.mSize        = mi.SizeOfImage;
                        }
                        else
                        {
                            // This normally shouldn't occur. It might be better if we simply didn't write this entry and returned a lower moduleCount value.
                            moduleInfo.mModuleHandle = 0;
                            moduleInfo.mPath.clear();
                            moduleInfo.mName.clear();
                            moduleInfo.mBaseAddress = 0;
                            moduleInfo.mSize = 0;
                        }
                    }
                }
            }
        }

        return moduleCount;

    #elif defined(EA_PLATFORM_XENON)

        size_t moduleCount = 0;

        #if EA_XBDM_ENABLED
            PDM_WALK_MODULES pWalkMod = NULL;
            DMN_MODLOAD      ModLoad;
            size_t           totalCount = 0;

            while(DmWalkLoadedModules(&pWalkMod, &ModLoad) == XBDM_NOERR)
            {
                if(pModuleInfoArray && (moduleCount < moduleArrayCapacity))
                {
                    ModuleInfo& moduleInfo = pModuleInfoArray[moduleCount];

                    moduleInfo.mModuleHandle = ModLoad.BaseAddress;
                    moduleInfo.mPath         = ModLoad.Name;
                    *EA::IO::Path::GetFileExtension(ModLoad.Name) = 0;
                    moduleInfo.mName         = EA::IO::Path::GetFileName(ModLoad.Name);
                    moduleInfo.mBaseAddress  = (uint64_t)(uintptr_t)ModLoad.BaseAddress;
                    moduleInfo.mSize         = ModLoad.Size;

                    moduleCount++;
                }

                totalCount++;
            }

            DmCloseLoadedModules(pWalkMod);

            if(!pModuleInfoArray)
                return totalCount;
        #endif

        return moduleCount;

    #elif defined(EA_PLATFORM_PS3)

        size_t moduleCount = 0;

        sys_prx_id_t              idArray[48];
        sys_prx_get_module_list_t moduleList;

        moduleList.size   = sizeof(sys_prx_get_module_list_t);
        moduleList.max    = sizeof(idArray)/sizeof(idArray[0]);
        moduleList.count  = 0;
        moduleList.idlist = idArray;

        if(sys_prx_get_module_list(0, &moduleList) == 0)
        {
            char                   fileNameBuffer[256];
            sys_prx_segment_info_t segmentInfo[2];
            sys_prx_module_info_t  info;

            if(!pModuleInfoArray)
                return moduleList.count;

            moduleCount = moduleList.count;

            if(moduleCount < moduleArrayCapacity)
               moduleCount = moduleArrayCapacity;

            for(size_t i = 0; i < moduleCount; i++)
            {
                info.size          = sizeof(sys_prx_module_info_t);
                info.filename      = fileNameBuffer;
                info.filename_size = sizeof(fileNameBuffer)/sizeof(fileNameBuffer[0]);
                info.segments      = segmentInfo;
                info.segments_num  = sizeof(segmentInfo)/sizeof(segmentInfo[0]);

                if(sys_prx_get_module_info(idArray[i], 0, &info) == 0)
                {
                    pModuleInfoArray[i].mModuleHandle = idArray[i];
                    pModuleInfoArray[i].mPath         = info.filename;       // "/app_home/prx/call-prx-export.sprx"
                    pModuleInfoArray[i].mName         = info.name;           // "cellPrxCallPrxExport"
                    pModuleInfoArray[i].mBaseAddress  = info.segments_num ? info.segments[0].base  : 0;
                    pModuleInfoArray[i].mSize         = info.segments_num ? info.segments[0].memsz : 0;
                }
            }
        }

        return moduleCount;

    #elif defined(EA_PLATFORM_KETTLE)
        SceKernelModule sceKernelModuleArray[96]; // SceKernelModule is a handle type. We declare on the stack because we don't want to allocate memory.
        size_t          arrayCount = 0;
        size_t          copiedCount = 0;

        int result = sceKernelGetModuleList(sceKernelModuleArray, EAArrayCount(sceKernelModuleArray), &arrayCount);

        if(result == SCE_OK)
        {
            SceKernelModuleInfo moduleInfo;

            if(pModuleInfoArray)
            {
                for(size_t i = 0; (i < arrayCount) && (i < moduleArrayCapacity); i++, copiedCount++)
                {
                    moduleInfo.size = sizeof(moduleInfo);
                    result = sceKernelGetModuleInfo(sceKernelModuleArray[i], &moduleInfo);

                    if(result == SCE_OK)
                    {   // As of SDK 1.020, the moduleInfo.name is only ever a file name and not a path.
                        pModuleInfoArray[i].mModuleHandle = sceKernelModuleArray[i];
                        pModuleInfoArray[i].mPath         = moduleInfo.name;                                 // "/app_home/prx/libkernel.spx"
                        pModuleInfoArray[i].mName         = EA::IO::Path::GetFileName(moduleInfo.name);      // "libkernel.spx"
                        pModuleInfoArray[i].mBaseAddress  = moduleInfo.numSegments ? reinterpret_cast<uint64_t>(moduleInfo.segmentInfo[0].baseAddr) : kBaseAddressInvalid;
                        pModuleInfoArray[i].mSize         = moduleInfo.numSegments ? moduleInfo.segmentInfo[0].size : 0;
                    }
                }
            }
            else // If pModuleInfoArray is NULL then we return the required count.
                return arrayCount;
        }

        return copiedCount;

    #elif defined(EA_PLATFORM_CAFE)

        // Walk the list of modules.
        // This could be implemented, but this platform is dead to us.
        (void)pModuleInfoArray;
        (void)moduleArrayCapacity;
        return 0;

    #elif defined(EA_PLATFORM_REVOLUTION)

        size_t moduleCount = 0;

        if(RSOModuleInfoManager::sRSOModuleInfoManager) // If the user registered a module info manager...
        {
            RSOModuleInfoArray& mIA = RSOModuleInfoManager::sRSOModuleInfoManager->mRSOModuleInfoArray;

            if(!pModuleInfoArray)
                return mIA.size();

            moduleCount = mIA.size();

            if(moduleCount < moduleArrayCapacity)
               moduleCount = moduleArrayCapacity;

            // This could be implemented, but this platform is dead to us.
            for(size_t i = 0; i < moduleCount; i++)
            {
                mIA[i].mModuleHandle;
                mIA[i].mModuleFileName;
                mIA[i].mModuleFileName;
                mIA[i].mModuleTextOffset;
                mIA[i].mModuleTextSize;
            }
        }

        return moduleCount;


    #elif defined(EA_PLATFORM_APPLE)
        ModuleInfoAppleArray moduleInfoAppleArray;
        size_t count = GetModuleInfoApple(moduleInfoAppleArray);
        
        if(count > moduleArrayCapacity)
            count = moduleArrayCapacity;
            
        for(size_t i = 0; i < count; i++)
            pModuleInfoArray[i] = moduleInfoAppleArray[(eastl_size_t)i];
        
        return count;
        
    #else

        // Other platforms that we support (e.g. Linux) don't provide this functionality.
        (void)pModuleInfoArray;
        (void)moduleArrayCapacity;
        return 0;

    #endif
}

EACALLSTACK_API bool GetModuleInfoByAddress(const void* pCodeAddress, ModuleInfo& moduleInfo, ModuleInfo* pModuleInfoArray, size_t moduleCount)
{
    for(size_t i = 0; i < moduleCount; i++)
    {
        if((pModuleInfoArray[i].mBaseAddress <= (uint64_t)(uintptr_t)pCodeAddress) && 
           ((uint64_t)(uintptr_t)pCodeAddress < (pModuleInfoArray[i].mBaseAddress + pModuleInfoArray[i].mSize)))
        {
            moduleInfo = pModuleInfoArray[i];
            return true;
        }
    }
    return false;
}

EACALLSTACK_API bool GetModuleInfoByHandle(ModuleHandle moduleHandle, ModuleInfo& moduleInfo, ModuleInfo* pModuleInfoArray, size_t moduleCount)
{
    const uint64_t moduleBaseAddress = GetModuleBaseAddressByHandle(moduleHandle);
    if(moduleBaseAddress == kBaseAddressInvalid)
    {
        return false;
    }
    else
    {
        return GetModuleInfoByAddress((void*)(intptr_t)moduleBaseAddress, moduleInfo, pModuleInfoArray, moduleCount);
    }
}

EACALLSTACK_API bool GetModuleInfoByName(const char* pModuleName, ModuleInfo& moduleInfo, ModuleInfo* pModuleInfoArray, size_t moduleCount)
{
        for(size_t i = 0; i < moduleCount; i++)
        {
            #if EA_FILE_SYSTEM_CASE_SENSITIVE
                if(pModuleInfoArray[i].mName == pModuleName)
            #else
                if(pModuleInfoArray[i].mName.comparei(pModuleName) == 0)
            #endif
            {
                moduleInfo = pModuleInfoArray[i];
                return true;
            }
        }
        return false;
}

///////////////////////////////////////////////////////////////////////////////
// GetSymbolInfoTypeFromDatabase
//
EACALLSTACK_API SymbolInfoType GetSymbolInfoTypeFromDatabase(const wchar_t* pDatabaseFilePath, TargetOS* pTargetOS, TargetProcessor* pTargetProcessor)
{
    IO::FileStream  fileStream(pDatabaseFilePath);
    SymbolInfoType  result          = kSymbolInfoTypeNone;
    TargetOS        targetOS        = kTargetOSNone;
    TargetProcessor targetProcessor = kTargetProcessorNone;

    fileStream.AddRef();

    if(fileStream.Open(IO::kAccessFlagRead))
    {
        char buffer[4096];
        EA::StdC::Memset8(buffer, 0, sizeof(buffer));

        if(fileStream.Read(buffer, sizeof(buffer) - 1) != IO::kSizeTypeError)
        {
            const wchar_t* const pExtension = IO::Path::GetFileExtension(pDatabaseFilePath);

            // Check for a Sony .self header.
            if((buffer[0] ==  'S') && (buffer[1] ==  'C') && (buffer[2] ==  'E') && (buffer[3] ==  0))
            {
                // A SELF file contains an ELF file. We find that ELF file within the SELF file and 
                // read it into our buffer, then just fall through and let the code detect an ELF file.
                uint64_t nELFLocation = 0;
                fileStream.SetPosition(16);
                EA::IO::ReadUint64(&fileStream, nELFLocation, EA::IO::kEndianBig);
                fileStream.SetPosition((EA::IO::off_type)nELFLocation);
                fileStream.Read(buffer, sizeof(buffer) - 1);
                // Fall through to .elf checking
            }

            // Check for an .elf header or a Sony .self header.
            if((buffer[0] == 0x7f) && (buffer[1] ==  'E') && (buffer[2] ==  'L') && (buffer[3] == 'F'))
            {
                bool bFileIsGHSElf = false;

                #if EACALLSTACK_GREENHILLS_GHSFILE_ENABLED
                    // Test if the file is a Green Hills elf.
                    // The primary way to tell if this is Green Hills elf is to look for Green Hills-specific 
                    // ELF sections, such as a .ghsinfo ELF header, or any header that begins with .ghs.
                    EA::Callstack::ELFFile elfFile;

                    if(elfFile.Init(&fileStream))
                    {
                        EA::Callstack::ELF::ELF_Shdr header;
                        
                        if(elfFile.FindSectionHeaderByIndex(elfFile.mHeader.e_shstrndx, &header))
                        {
                            const size_t      kSectionBufferSize = 512;
                            char              sectionNamesBuffer[kSectionBufferSize + 1] = { };
                            EA::IO::size_type readSize = (EA::IO::size_type)((header.sh_size < kSectionBufferSize) ? header.sh_size : kSectionBufferSize);

                            if(fileStream.SetPosition((EA::IO::off_type)header.sh_offset))
                            {
                                readSize = fileStream.Read(sectionNamesBuffer, readSize);

                                if(readSize != EA::IO::kSizeTypeError)
                                {
                                    for(size_t i = 0, counter = 0; (i < readSize) && (counter < 1000); i += EA::StdC::Strlen(sectionNamesBuffer + i) + 1, counter++)
                                    {
                                        if(EA::StdC::Stristr(sectionNamesBuffer + i, ".ghs") != NULL)
                                        {
                                            bFileIsGHSElf   = true;
                                            result          = kSymbolInfoTypeGHS;
                                            targetProcessor = kTargetProcessorPowerPC32;
                                            targetOS        = kTargetOSNew;

                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                #endif

                if(!bFileIsGHSElf)
                {
                    using namespace EA::Callstack::ELF;

                    result = kSymbolInfoTypeDwarf2;
        
                  //const bool b64BitELF        = (buffer[EI_CLASS] == ELFCLASS64);
                    const EA::IO::Endian endian = (buffer[EI_DATA]  == ELFDATA2MSB) ? EA::IO::kEndianBig : EA::IO::kEndianLittle;

                    // Detect the processor first.
                    uint16_t elfProcessor;
                    EA::StdC::Memcpy(&elfProcessor, buffer + 18, sizeof(uint16_t));
                    if(endian != EA::IO::kEndianLocal)
                        elfProcessor = ARLocal::SwizzleUint16(elfProcessor);

                    if(elfProcessor == EM_X86_64)
                        targetProcessor = kTargetProcessorX64;
                    else if((elfProcessor == EM_386) || (elfProcessor == EM_486))
                        targetProcessor = kTargetProcessorX86;
                    else if(elfProcessor == EM_PPC)
                        targetProcessor = kTargetProcessorPowerPC32;
                    else if(elfProcessor == EM_PPC64)
                        targetProcessor = kTargetProcessorPowerPC64;
                    else if(elfProcessor == EM_SPU)
                        targetProcessor = kTargetProcessorSPU;
                    else if(elfProcessor == EM_ARM)
                        targetProcessor = kTargetProcessorARM;

                    // Detect the target OS.
                    if(buffer[EI_OSABI] == ELFOSABI_LINUX) // Actually, Linux usually uses ELFOSABI_NONE, but we'll catch that below.
                        targetOS = kTargetOSLinux;
                    else if((buffer[EI_OSABI] == ELFOSABI_PS3) && (buffer[17] == ET_CORE))
                        targetOS = kTargetOSPS3CoreDump;
                    else if(buffer[EI_OSABI] == ELFOSABI_PS3)
                        targetOS = kTargetOSPS3;
                    else if(targetProcessor == kTargetProcessorSPU) // Check this before ELFOSABI_NONE. 
                        targetOS = kTargetOSSPU;
                    else if((targetProcessor == kTargetProcessorPowerPC32) && (endian == EA::IO::kEndianBig)) // Check this before ELFOSABI_NONE. This test sucks, but I'm not seeing a simple alternative, given that the Wii ELF isn't identifying itself as per the ELF standard.
                        targetOS = kTargetOSWii;
                    else if(buffer[EI_OSABI] == ELFOSABI_NONE)
                    {
                        // It turns out that a number of platforms use ELFOSABI_NONE. In this case the only correct solution
                        // is to iterate the sections and look for OS-specific info. 
                        targetOS = kTargetOSLinux; // or kTargetOSOSX or kTargetOSWii.
                    }
                }
            }
            else if(EA::StdC::Stricmp(pExtension, L".dnm") == 0) // Green Hills .dnm debug info format.
            {
                // Check for a Green Hills DNM header.
                if((buffer[0] ==  0x7f) && (buffer[1] ==  'G') && (buffer[2] ==  'H') && (buffer[3] == 'S'))
                {
                    result          = kSymbolInfoTypeGHS;
                    targetProcessor = kTargetProcessorPowerPC32;
                    targetOS        = kTargetOSNew;
                }
            }
            else if((EA::StdC::Stricmp(pExtension, L".map") == 0) || 
                    (EA::StdC::Stricmp(pExtension, L".txt") == 0))
            {
                // Code Warrior RSO .map files sometimes start with lines like the following:
                if(EA::StdC::Strstr(buffer, "SYMBOL NOT FOUND") || EA::StdC::Strstr(buffer, "Link map of "))
                {
                    result = kSymbolInfoTypeMapCW;

                    // We don't current support any CodeWarrior platforms except Wii, so we 
                    // just assume we are dealing with the Wii. A Wii .map file will have 
                    // the strings "Hollywood" and/or "Revolution" in the first few lines.

                    targetOS        = kTargetOSWii;
                    targetProcessor = kTargetProcessorPowerPC32;
                }

                else if(EA::StdC::Strstr(buffer, "Timestamp is"))
                {
                    result = kSymbolInfoTypeMapVC;

                    if(EA::StdC::Strstr(buffer, "Preferred load address is 82000000") || EA::StdC::Strstr(buffer, " .XBLD"))
                    {
                        targetOS        = kTargetOSXBox360;
                        targetProcessor = kTargetProcessorPowerPC32;
                    }
                    else if(EA::StdC::Strstr(buffer, "Rva+Base               Lib:Object") || EA::StdC::Strstr(buffer, "load address is 00000"))
                    {
                        targetOS        = kTargetOSWin64;
                        targetProcessor = kTargetProcessorX64;
                    }
                    else
                    {
                        targetOS        = kTargetOSWin32;
                        targetProcessor = kTargetProcessorX86;
                    }
                }

                // CodeWarrior map files have headers that look like this:
                // .init section layout
                //   Starting        Virtual  File
                //   address  Size   address  offset
                //   ---------------------------------
                else if(EA::StdC::Strstr(buffer, "  Starting        Virtual  File"))
                {
                    result = kSymbolInfoTypeMapCW;

                    // We don't current support any CodeWarrior platforms except Wii, so we 
                    // just assume we are dealing with the Wii. A Wii .map file will have 
                    // the strings "Hollywood" and/or "Revolution" in the first few lines.

                    targetOS        = kTargetOSWii;
                    targetProcessor = kTargetProcessorPowerPC32;
                }

                // Green Hills map files have headers that look like this:
                //  Link Date:	Tue Apr 24 17:07:18 2012
                //  Host OS:	Windows 7 Service Pack 1
                //  Version:	ELXR 5.3 (c) Green Hills Software   Build: Jan 20 2012
                else if(EA::StdC::Strstr(buffer, "Link Date:\t") && 
                        EA::StdC::Strstr(buffer, "Host OS:\t"))
                {
                    result = kSymbolInfoTypeMapGHS;

                    targetOS        = kTargetOSNew;                 // To do: Make this correct. We can discern the actual OS by looking at the .map file Module Summary entries.
                    targetProcessor = kTargetProcessorPowerPC32;    // To do: Make this correct.
                }

                // SN map files have a header that look like this:
                // Address  Size     Align Out     In      File    Symbol
                // =================================================================
                else if(EA::StdC::Strstr(buffer, "Address  Size     Align Out     In      File    Symbol"))
                {
                    result = kSymbolInfoTypeMapSN;

                    if(EA::StdC::Strstr(buffer, "__PPU_GUID") || EA::StdC::Strstr(buffer, "ppu-lv2"))
                    {
                        targetOS        = kTargetOSPS3;
                        targetProcessor = kTargetProcessorPowerPC64; // The PS3 PPU is actually a 64 bit processor, but it's run in 32 bit mode.
                    }
                    else
                    {
                        targetOS        = kTargetOSSPU;        // To do: Find a more explicit way to detect this.
                        targetProcessor = kTargetProcessorSPU;
                    }
                }

                // Apple's map files have a header that look like this:
                // # Address Size        File  Name
                // =================================================================
                else if(EA::StdC::Strstr(buffer, "# Path:") && 
                        EA::StdC::Strstr(buffer, "# Arch:"))
                {
                    result = kSymbolInfoTypeMapApple;

                    if(EA::StdC::Strstr(buffer, "# Arch: x86_64"))
                    {
                        targetOS        = kTargetOSOSX;
                        targetProcessor = kTargetProcessorX64;
                    }
                    if(EA::StdC::Strstr(buffer, "# Arch: i386"))
                    {
                        targetOS        = kTargetOSOSX;         // It could instead be iOS emulation on OSX, though actually that would still indicate an OSX application.
                        targetProcessor = kTargetProcessorX86;
                    }
                    else if(EA::StdC::Strstr(buffer, "# Arch: ppc"))
                    {
                        targetOS        = kTargetOSOSX;
                        targetProcessor = kTargetProcessorPowerPC32; // Could also be 64.
                    }
                    else
                    {
                        targetOS        = kTargetOSIOS;
                        targetProcessor = kTargetProcessorARM;
                    }
                }

                // GCC map files aren't highly distinctive. It's harder to tell by looking 
                // at a given text file and programmatically tell if it's a GCC .map file.
                // So for now we just always say that it is.
                else
                {
                    result = kSymbolInfoTypeMapGCC;

                    if(EA::StdC::Stristr(buffer, "android"))
                    {
                        targetOS        = kTargetOSAndroid;
                        targetProcessor = kTargetProcessorARM;
                    }
                    else if(EA::StdC::Strstr(buffer, "/usr/lib"))
                    {
                        if(EA::StdC::Strstr(buffer, "linux"))
                            targetOS = kTargetOSLinux;
                        else if(EA::StdC::Strstr(buffer, "apple"))
                            targetOS = kTargetOSOSX;

                        // The following assumes the target processor is x86 or x64, but it doesn't work right for apple (Mac OS X), because it uses a single /lib directory for everything.
                        if(EA::StdC::Strstr(buffer, "/lib64") || EA::StdC::Strstr(buffer, "/lib/64"))
                            targetProcessor = kTargetProcessorX64;
                        else
                            targetProcessor = kTargetProcessorX86;
                    }
                    else if(EA::StdC::Strstr(buffer, "ppu/lib") || EA::StdC::Strstr(buffer, "ppu\\lib"))
                    {
                        targetOS        = kTargetOSPS3;
                        targetProcessor = kTargetProcessorPowerPC64; // The PS3 PPU is actually a 64 bit processor, but it's run in 32 bit mode.
                    }
                    else if(EA::StdC::Strstr(buffer, "spu/lib") || EA::StdC::Strstr(buffer, "spu\\lib"))
                    {
                        targetOS        = kTargetOSSPU;
                        targetProcessor = kTargetProcessorSPU; // The PS3 PPU is actually a 64 bit processor, but it's run in 32 bit mode.
                    }
                }
            }

            else if(EA::StdC::Stricmp(pExtension, L".pdb") == 0)
            {
                if(EA::StdC::Strstr(buffer, "Microsoft C/C++ MSF 7.00") == buffer)
                    result = kSymbolInfoTypePDB7;

                else if(EA::StdC::Strstr(buffer, "Microsoft C/C++ MSF 8.00") == buffer)
                    result = kSymbolInfoTypePDB8;

                // To do: Find out how to tell these correctly.
                targetOS        = kTargetOSXBox360;           // kTargetOSWin32, kTargetOSWin64
                targetProcessor = kTargetProcessorPowerPC32;  // kTargetProcessorX64
            }
        }
    }

    if(pTargetOS)
        *pTargetOS = targetOS;

    if(pTargetProcessor)
        *pTargetProcessor = targetProcessor;

    return result;
}


///////////////////////////////////////////////////////////////////////////////
// GetCurrentOS
//
TargetOS GetCurrentOS()
{
    #if defined(EA_PLATFORM_XENON)
        return kTargetOSXBox360;
    #elif defined(EA_PLATFORM_PS3)
        return kTargetOSPS3;
    #elif defined(EA_PLATFORM_PS3_SPU)
        return kTargetOSSPU;
    #elif defined(EA_PLATFORM_CAFE)
        return kTargetOSCafe;
    #elif defined(EA_PLATFORM_REVOLUTION)
        return kTargetOSWii;
    #elif defined(EA_PLATFORM_IPHONE)
        return kTargetOSIOS;
    #elif defined(EA_PLATFORM_ANDROID)
        return kTargetOSAndroid;
    #elif defined(EA_PLATFORM_PALM)
        return kTargetOSPalm;
    #elif defined(EA_PLATFORM_PLAYBOOK)
        return kTargetOSPlaybook;
    #elif defined(EA_PLATFORM_OSX)
        return kTargetOSOSX;
    #elif defined(EA_PLATFORM_LINUX)
        return kTargetOSLinux;
    #elif defined(EA_PLATFORM_WIN32)
        return kTargetOSWin32;             // It's possible that we are in fact running under Win64, though within the Win32 WoW of Win64.
    #elif defined(EA_PLATFORM_WIN64)
        return kTargetOSWin64;
    #elif defined(EA_PLATFORM_MICROSOFT) && defined(EA_PLATFORM_MOBILE)
        return kTargetOSMicrosoftMobile;
    #elif defined(EA_PLATFORM_MICROSOFT) && defined(EA_PLATFORM_CONSOLE)
        return kTargetOSMicrosoftConsole;
    #else
        return kTargetOSNone;
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// GetCurrentProcessor
//
TargetProcessor GetCurrentProcessor()
{
    #if defined(EA_PROCESSOR_POWERPC)           // We have a small problem with PowerPC: The PowerPC processor instruction set is both 32 and 64 bit at the same time. It's a unified instruction set. EABase doesn't currently define EA_PROCESSOR_POWERPC_32 / EA_PROCESSOR_POWERPC_64.
        #if defined(EA_PLATFORM_REVOLUTION) || defined(EA_PLATFORM_CAFE)
            return kTargetProcessorPowerPC32;
        #else
            return kTargetProcessorPowerPC64;
        #endif
    #elif defined(EA_PROCESSOR_SPU)
        return kTargetProcessorSPU;
    #elif defined(EA_PROCESSOR_X86)
        return kTargetProcessorX86;
    #elif defined(EA_PROCESSOR_X86_64)
        return kTargetProcessorX64;
    #elif defined(EA_PROCESSOR_ARM)
        return kTargetProcessorARM;
    #else
        return kTargetProcessorNone;
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// MakeAddressRepLookupFromDatabase
//
EACALLSTACK_API IAddressRepLookup* MakeAddressRepLookupFromDatabase(const wchar_t* pDatabaseFilePath, 
                                                                    EA::Allocator::ICoreAllocator* pCoreAllocator,
                                                                    SymbolInfoType symbolInfoType)
{
    if(!pCoreAllocator)
    {
        pCoreAllocator = EA::Callstack::GetAllocator();
        EA_ASSERT(pCoreAllocator != NULL); // If this fails, did you forget to call EA::Callstack::SetAllocator?
    }

    if(symbolInfoType == kSymbolInfoTypeNone)
        symbolInfoType = GetSymbolInfoTypeFromDatabase(pDatabaseFilePath);

    if(symbolInfoType != kSymbolInfoTypeNone)
    {
        void* pMemory;

        switch(symbolInfoType)
        {
            case kSymbolInfoTypeMapVC:
            {
                #if EACALLSTACK_MSVC_MAPFILE_ENABLED
                    pMemory = pCoreAllocator->Alloc(sizeof(MapFileMSVC), EACALLSTACK_ALLOC_PREFIX "EACallstack/MapFileMSVC", 0);

                    if(pMemory)
                    {
                        MapFileMSVC* const pMapFileMSVC = new(pMemory) MapFileMSVC(pCoreAllocator);
                        return pMapFileMSVC;
                    }
                #endif

                break;
            }

            case kSymbolInfoTypeMapGCC:
            {
                #if EACALLSTACK_GCC_MAPFILE_ENABLED
                    pMemory = pCoreAllocator->Alloc(sizeof(MapFileGCC3), EACALLSTACK_ALLOC_PREFIX "EACallstack/MapFileGCC3", 0);

                    if(pMemory)
                    {
                        MapFileGCC3* const pMapFileGCC3 = new(pMemory) MapFileGCC3(pCoreAllocator);
                        return pMapFileGCC3;
                    }
                #endif

                break;
            }

            case kSymbolInfoTypeMapSN:
            {
                #if EACALLSTACK_SN_MAPFILE_ENABLED
                    pMemory = pCoreAllocator->Alloc(sizeof(MapFileSN), EACALLSTACK_ALLOC_PREFIX "EACallstack/MapFileSN", 0);

                    if(pMemory)
                    {
                        MapFileSN* const pMapFileSN = new(pMemory) MapFileSN(pCoreAllocator);
                        return pMapFileSN;
                    }
                #endif

                break;
            }

            case kSymbolInfoTypeMapCW:
            {
                #if EACALLSTACK_CODEWARRIOR_MAPFILE_ENABLED 
                    pMemory = pCoreAllocator->Alloc(sizeof(MapFileCodeWarrior), EACALLSTACK_ALLOC_PREFIX "EACallstack/MapFileCodeWarrior", 0);

                    if(pMemory)
                    {
                        MapFileCodeWarrior* const pMapFileCW = new(pMemory) MapFileCodeWarrior(pCoreAllocator);
                        return pMapFileCW;
                    }
                #endif

                break;
            }

            case kSymbolInfoTypeMapGHS:
            {
                #if EACALLSTACK_GREENHILLS_MAPFILE_ENABLED 
                    pMemory = pCoreAllocator->Alloc(sizeof(MapFileGreenHills), EACALLSTACK_ALLOC_PREFIX "EACallstack/MapFileGreenHills", 0);

                    if(pMemory)
                    {
                        MapFileGreenHills* const pMapFileGH = new(pMemory) MapFileGreenHills(pCoreAllocator);
                        return pMapFileGH;
                    }
                #endif

                break;
            }

            case kSymbolInfoTypePDB7:
            case kSymbolInfoTypePDB8:
            {
                #if EACALLSTACK_PDBFILE_ENABLED
                    pMemory = pCoreAllocator->Alloc(sizeof(PDBFile), EACALLSTACK_ALLOC_PREFIX "EACallstack/PDBFile", 0);

                    if(pMemory)
                    {
                        PDBFile* const pPDBFile = new(pMemory) PDBFile(pCoreAllocator);
                        return pPDBFile;
                    }
                #endif

                break;
            }

            case kSymbolInfoTypeDwarf2:
            {
                #if EACALLSTACK_DWARFFILE_ENABLED
                    pMemory = pCoreAllocator->Alloc(sizeof(DWARF2File), EACALLSTACK_ALLOC_PREFIX "EACallstack/DWARF2File", 0);

                    if(pMemory)
                    {
                        DWARF2File* const pDWARF2File = new(pMemory) DWARF2File(pCoreAllocator);
                        return pDWARF2File;
                    }
                #endif

                break;
            }

            case kSymbolInfoTypeGHS:
            {
                #if EACALLSTACK_GREENHILLS_GHSFILE_ENABLED
                    pMemory = pCoreAllocator->Alloc(sizeof(GHSFile), EACALLSTACK_ALLOC_PREFIX "EACallstack/GHSFile", 0);

                    if(pMemory)
                    {
                        GHSFile* const pGHSFile = new(pMemory) GHSFile(pCoreAllocator);
                        return pGHSFile;
                    }
                #endif

                break;
            }

            case kSymbolInfoTypeMapApple:
            {
                #if EACALLSTACK_APPLE_MAPFILE_ENABLED
                    pMemory = pCoreAllocator->Alloc(sizeof(MapFileApple), EACALLSTACK_ALLOC_PREFIX "EACallstack/MapFileApple", 0);

                    if(pMemory)
                    {
                        MapFileApple* const pMapFileApple = new(pMemory) MapFileApple(pCoreAllocator);
                        return pMapFileApple;
                    }
                #endif

                break;
            }

            case kSymbolInfoTypeNone:
            case kSymbolInfoTypeStabs:
                // Not currently supported.
                break;
        }
    }

    return NULL;
}


EACALLSTACK_API void DestroyAddressRepLookup(IAddressRepLookup* pLookup, EA::Allocator::ICoreAllocator* pCoreAllocator)
{
    if(pLookup)
    {
        if(!pCoreAllocator)
        {
            pCoreAllocator = EA::Callstack::GetAllocator();
            EA_ASSERT(pCoreAllocator != NULL); // If this fails, did you forget to call EA::Callstack::SetAllocator?
        }

        pLookup->~IAddressRepLookup();
        pCoreAllocator->Free(pLookup);
    }
}


///////////////////////////////////////////////////////////////////////////////
// AddressRepLookupSet
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// AddressRepLookupSet
//
AddressRepLookupSet::AddressRepLookupSet(EA::Allocator::ICoreAllocator* pCoreAllocator)
  : mpCoreAllocator(pCoreAllocator ? pCoreAllocator : EA::Callstack::GetAllocator()),
    mbLookupsAreLocal(true),
    mbEnableAutoDatabaseFind(true),
    mbLookedForDBFiles(false),
    mAddressRepLookupList(EA::Callstack::EASTLCoreAllocator(EACALLSTACK_ALLOC_PREFIX "EACallstack/AddressRepLookupList", mpCoreAllocator)),
    mSourceCodeDirectoryList(EA::Callstack::EASTLCoreAllocator(EACALLSTACK_ALLOC_PREFIX "EACallstack/SourceCodeDirectoryList", mpCoreAllocator)),
    mnSourceCodeFailureCount(0),
    mnSourceContextLineCount(kSourceContextLineCount)
{
}


///////////////////////////////////////////////////////////////////////////////
// ~AddressRepLookupSet
//
AddressRepLookupSet::~AddressRepLookupSet()
{
    AddressRepLookupSet::Shutdown();
}


///////////////////////////////////////////////////////////////////////////////
// SetAllocator
//
void AddressRepLookupSet::SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator)
{
    mpCoreAllocator = pCoreAllocator;

    mAddressRepLookupList   .get_allocator().set_allocator(pCoreAllocator);
    mSourceCodeDirectoryList.get_allocator().set_allocator(pCoreAllocator);
}


///////////////////////////////////////////////////////////////////////////////
// Init
//
bool AddressRepLookupSet::Init()
{
    return true;
}


///////////////////////////////////////////////////////////////////////////////
// Shutdown
//
bool AddressRepLookupSet::Shutdown()
{
    for(AddressRepLookupList::iterator it = mAddressRepLookupList.begin(); it != mAddressRepLookupList.end(); ++it)
    {
        IAddressRepLookup* const pARL = *it;

        pARL->Shutdown();
        pARL->~IAddressRepLookup();
        mpCoreAllocator->Free(pARL);
    }

    mAddressRepLookupList.clear();
    mnSourceCodeFailureCount = 0;

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// EnableAutoDatabaseFind
//
void AddressRepLookupSet::EnableAutoDatabaseFind(bool bEnable)
{
    mbEnableAutoDatabaseFind = bEnable;
}


///////////////////////////////////////////////////////////////////////////////
// SetSourceCodeLineCount
//
void AddressRepLookupSet::SetSourceCodeLineCount(unsigned int lineCount)
{
    mnSourceContextLineCount = lineCount;
}


///////////////////////////////////////////////////////////////////////////////
// AddDatabaseFileEx
//
bool AddressRepLookupSet::AddDatabaseFileEx(const wchar_t* pDatabaseFilePath, uint64_t baseAddress, 
                                            bool bDelayLoad, EA::Allocator::ICoreAllocator* pCoreAllocator,
                                            bool& bDBPreviouslyPresent, bool& bBaseAddressAlreadySet)
{
    // Check to see if the DB is already registered.
    IAddressRepLookup* pARL = AddressRepLookupSet::GetAddressRepLookup(pDatabaseFilePath);

    if(pARL)
    {
        uint64_t existingDBBaseAddress = pARL->GetBaseAddress();

        if((existingDBBaseAddress == baseAddress) || (existingDBBaseAddress != kBaseAddressUnspecified))
            bBaseAddressAlreadySet = true;
        else
        {
            pARL->SetBaseAddress(baseAddress); // Overwrite the base address.
            bBaseAddressAlreadySet = false;
        }

        bDBPreviouslyPresent = true;

        return true;
    }

    bDBPreviouslyPresent  = false;
    bBaseAddressAlreadySet = false;

    if(!mpCoreAllocator)
    {
        mpCoreAllocator = EA::Callstack::GetAllocator();
        EA_ASSERT(mpCoreAllocator != NULL); // If this fails, did you forget to call EA::Callstack::SetAllocator or did you call it after creating this AddressRepLookupSet instance?
    }

    if(!pCoreAllocator)
        pCoreAllocator = mpCoreAllocator;

    // Note that we create the ARL object with our allocator and not the pCoreAllocatorArgument. 
    // The argument pCoreAllocator is used by the created ARL object. Of course, in the large majority
    // of cases these will be the same ICoreAllocator.

    pARL = MakeAddressRepLookupFromDatabase(pDatabaseFilePath, mpCoreAllocator);

    if(pARL)
    {
        pARL->SetBaseAddress(baseAddress); // baseAddress can conceivably be 0 or kBaseAddressUnspecified.
        pARL->SetAllocator(pCoreAllocator);

        #if !defined(EA_PLATFORM_DESKTOP)
            // To do: Make this an option that can be set by the user.
            pARL->SetOption(IAddressRepLookup::kOptionLowMemoryUsage, 1);
        #endif

        if(pARL->Init(pDatabaseFilePath, bDelayLoad))
        {
            AddAddressRepLookup(pARL);
            return true;
        }

        // The above Init failed, so we free the ARL.
        pARL->~IAddressRepLookup();
        mpCoreAllocator->Free(pARL);
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// AddDatabaseFile
//
bool AddressRepLookupSet::AddDatabaseFile(const wchar_t* pDatabaseFilePath, uint64_t baseAddress, 
                                            bool bDelayLoad, EA::Allocator::ICoreAllocator* pCoreAllocator)
{
    bool bDBPreviouslyPresent;
    bool bBaseAddressAlreadySet;

    return AddDatabaseFileEx(pDatabaseFilePath, baseAddress, bDelayLoad, pCoreAllocator, bDBPreviouslyPresent, bBaseAddressAlreadySet);
}


///////////////////////////////////////////////////////////////////////////////
// RemoveDatabaseFile
//
bool AddressRepLookupSet::RemoveDatabaseFile(const wchar_t* pDatabaseFilePath)
{
    for(AddressRepLookupList::iterator it = mAddressRepLookupList.begin(); it != mAddressRepLookupList.end(); ++it)
    {
        IAddressRepLookup* const pARL = *it;

        const wchar_t* pExistingDatabaseFilePath = pARL->GetDatabasePath();

        if(EA::StdC::Stricmp(pDatabaseFilePath, pExistingDatabaseFilePath) == 0)
        {
            mAddressRepLookupList.erase(it);
            return true;
        }
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressRepLookup
//
IAddressRepLookup* AddressRepLookupSet::GetAddressRepLookup(const wchar_t* pDatabaseFilePath)
{
    for(AddressRepLookupList::iterator it = mAddressRepLookupList.begin(); it != mAddressRepLookupList.end(); ++it)
    {
        IAddressRepLookup* const pARL = *it;

        const wchar_t* pExistingDatabaseFilePath = pARL->GetDatabasePath();

        if(EA::StdC::Stricmp(pDatabaseFilePath, pExistingDatabaseFilePath) == 0)
            return pARL;
    }

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// GetDatabaseFileCount
//
size_t AddressRepLookupSet::GetDatabaseFileCount() const
{
    return (size_t)mAddressRepLookupList.size();
}


///////////////////////////////////////////////////////////////////////////////
// GetDatabaseFile
//
IAddressRepLookup* AddressRepLookupSet::GetDatabaseFile(size_t index)
{
    if(index < (size_t)mAddressRepLookupList.size())
    {
        AddressRepLookupList::iterator it = mAddressRepLookupList.begin();
        eastl::advance(it, index);
        return *it;
    }

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// AddAddressRepLookup
//
void AddressRepLookupSet::AddAddressRepLookup(IAddressRepLookup* pAddressRepLookup)
{
    // It is assumed that the pAddressRepLookup instance is already initialized.
    mAddressRepLookupList.push_back(pAddressRepLookup);
}


///////////////////////////////////////////////////////////////////////////////
// AddSourceCodeDirectory
//
void AddressRepLookupSet::AddSourceCodeDirectory(const wchar_t* pSourceDirectory)
{
    mSourceCodeDirectoryList.push_back();
    mSourceCodeDirectoryList.back() = pSourceDirectory;
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressRep
//
int AddressRepLookupSet::GetAddressRep(int addressRepTypeFlags, uint64_t address, FixedString* pRepArray, int* pIntValueArray)
{
    // It turns out that we need to do a file/line lookup in order to do 
    // a source lookup. 
    if(addressRepTypeFlags & kARTFlagSource)
        addressRepTypeFlags |= kARTFlagFileLine;

    // Possibly go and auto-find symbol file sources.
    if(mAddressRepLookupList.empty() && mbEnableAutoDatabaseFind && !mbLookedForDBFiles)
        AutoDatabaseAdd();

    // Look at our set of symbol file sources.
    int returnFlags = GetAddressRepFromSet(addressRepTypeFlags, address, pRepArray, pIntValueArray);

    // Implement kARTFlagAddress.
    if(addressRepTypeFlags & kARTFlagAddress)
        returnFlags |= GetAddressRepAddress(address, pRepArray, pIntValueArray);

    // Implement kARTFlagSource.
    if((addressRepTypeFlags & kARTFlagSource) && !pRepArray[kARTFileLine].empty())
        returnFlags |= GetAddressRepSource(pRepArray, pIntValueArray);

    return returnFlags;
}


///////////////////////////////////////////////////////////////////////////////
// AutoDatabaseAdd
//
void AddressRepLookupSet::AutoDatabaseAdd()
{
    if(!mbLookedForDBFiles)
    {
        mbLookedForDBFiles = true; // Set this to true so we don't keep trying this if it fails.

        // Walk the loaded modules and see if any of them has a corresponding .map/.pdb/.elf file that we can add.
        if(mpCoreAllocator) // To consider: Support using a local array of N ModuleInfos if mpCoreAllocator is NULL or fails to allocate.
        {
            size_t moduleCount = EA::Callstack::GetModuleInfo(NULL, 0);

            if((moduleCount > 0) && (moduleCount < 512))
            {
                EA::Callstack::ModuleInfo* pModuleInfoArray = CORE_NEW_ARRAY(mpCoreAllocator, ModuleInfo, moduleCount, "ModuleInfo", EA::Allocator::MEM_TEMP);

                if(pModuleInfoArray)
                {
                    wchar_t processDirectory[IO::kMaxPathLength]; // e.g. /abc/def/
                    size_t processDirectoryLength = EA::StdC::GetCurrentProcessDirectory(processDirectory);

                    // Until some day that we fix the .elf (DWARF) parsing code, we aren't able to parse .elf/.self files directly.
                    // The problem is that at some point DWARF-generating compilers revised their code to write debug info in a way 
                    // that we can't understand. Also, we have some prototype code to read .pdb files natively but it turns out
                    // that doesn't work any more either, due to Microsoft changes.
                    #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) // We can read .pdb files directly on Windows because we can use a Microsoft API.
                        const wchar_t* kExtensionTypes[] = { L".pdb", L".map" };
                    #else
                        const wchar_t* kExtensionTypes[] = { L".map" };
                    #endif

                    moduleCount = EA::Callstack::GetModuleInfo(pModuleInfoArray, moduleCount);

                    // Walk through all the modules and see if the module appears to have a corresponding .map/.pdb, etc file we can load.
                    for(size_t i = 0; i < moduleCount; i++)
                    {
                        // Some platforms (e.g. Microsoft) provide a path, some don't (e.g. Sony).
                        wchar_t pathW[EA::IO::kMaxPathLength];

                        if(EA::IO::Path::IsRelative(pModuleInfoArray[i].mPath.c_str())) // If mPath is only a relative path...
                        {
                            // We try making a full path relative to our process directory. A lot of the time this will result
                            // in invalid paths because it will generate system library paths in our process directory. That's fine, 
                            // and we'll just let it fail for those cases.
                            EA::StdC::Strlcpy(pModuleInfoArray[i].mPath, processDirectory, processDirectoryLength); // Newer versions of EASTL can do this directly instead of using Strlcpy.
                            pModuleInfoArray[i].mPath += pModuleInfoArray[i].mName;
                            pModuleInfoArray[i].mPath += ".___";
                        }

                        // Now that we have full module path, see if there are .map analogs for it.
                        EA::StdC::Strlcpy(pathW, pModuleInfoArray[i].mPath.c_str(), EAArrayCount(pathW));
                        wchar_t* pExtension = IO::Path::GetFileExtension(pathW);

                        if((pExtension + 4) < (pathW + EAArrayCount(pathW))) // If there's enough space...
                        {
                            for(size_t j = 0; j < EAArrayCount(kExtensionTypes); j++)
                            {
                                EA::StdC::Strcpy(pExtension, kExtensionTypes[j]);
                                if(IO::File::Exists(pathW) && !GetAddressRepLookup(pathW)) // If C:\blah\file.map exists and if it's not already registered... add it.
                                    AddDatabaseFile(pathW, pModuleInfoArray[i].mBaseAddress, true, mpCoreAllocator);
                            }
                        }
                    }

                    CORE_DELETE_ARRAY(mpCoreAllocator, pModuleInfoArray);
                }
            }
        }


        // Fallback basic functionality for the case that module iteration above isn't possible. 
        // In this case we just directly look for a corresponding symbol file for the application.
        {
            wchar_t dbPath[IO::kMaxPathLength];          // e.g. /abc/def/someapp.elf
            EA::StdC::GetCurrentProcessPath(dbPath);

            // Under GCC, the application file (ELF file) also contains the debug information.
            // As of this writing, the following probably won't work in practice because our .elf (DWARF) parsing code doesn't
            // work with recent platform .elf files due to compiler/linker changes that have happened in the last few years.
            #if defined(__GNUC__) || defined(__clang__)
                if(IO::File::Exists(dbPath) && !GetAddressRepLookup(dbPath)) // If C:\blah\file.map exists and if it's not already registered... add it.
                    AddDatabaseFile(dbPath, kBaseAddressUnspecified, true, mpCoreAllocator);
            #endif

            wchar_t* pExtension = IO::Path::GetFileExtension(dbPath);

            if((pExtension + 4) < (dbPath + EAArrayCount(dbPath))) // If there's enough space...
            {
                // See if <app>.map exists.
                EA::StdC::Strcpy(pExtension, L".map");
                if(IO::File::Exists(dbPath) && !GetAddressRepLookup(dbPath)) // If C:\blah\file.map exists and if it's not already registered... add it.
                    AddDatabaseFile(dbPath, kBaseAddressUnspecified, true, mpCoreAllocator);
        
                // See if <app>.pdb exists.
                EA::StdC::Strcpy(pExtension, L".pdb");
                if(IO::File::Exists(dbPath) && !GetAddressRepLookup(dbPath)) // If C:\blah\file.pdb exists and if it's not already registered... add it.
                    AddDatabaseFile(dbPath, kBaseAddressUnspecified, true, mpCoreAllocator);
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressRepFromSet
//
int AddressRepLookupSet::GetAddressRepFromSet(int addressRepTypeFlags, uint64_t address, FixedString* pRepArray, int* pIntValueArray)
{
    int returnFlags = 0;

    // Don't search for kARTAddress and kARTSource. We search for them separately.
    addressRepTypeFlags &= ~(kARTFlagAddress | kARTFlagSource);

    // We try each AddressRepSource until all requested addressRepTypeFlags are found.
    // Of course we might try all of them without all addressRepTypeFlags being found.

    for(AddressRepLookupList::iterator it = mAddressRepLookupList.begin(); 
         addressRepTypeFlags && (it != mAddressRepLookupList.end()); ++it)
    {
        IAddressRepLookup* const pARL = *it;

        returnFlags |= pARL->GetAddressRep(addressRepTypeFlags, address, pRepArray, pIntValueArray);

        // If we find anything then don't try to look for it any more.
        addressRepTypeFlags &= ~returnFlags;
    }
    
    // To consider: Move the following processing to a more logical/sensible place. One thought is to 
    //              create a special IAdressRepLookup class soley for "local process" lookups which
    //              takes advantage of system-provided symbol information when present. 
    if(returnFlags == 0) // If nothing useful was found
    {
        if(GetLookupsAreLocal()) // If the data we are analyzing is from our own process...
        {
            #if EACALLSTACK_GLIBC_BACKTRACE_AVAILABLE
                // The symbol names may be unavailable without the use of special linker
                // options. For systems using the GNU linker, it is necessary to use the
                // -rdynamic linker option. Note that names of "static" functions are not
                // exposed and won't be available .
                // http://linux.die.net/man/3/backtrace_symbols
                //
                // backtrace_symbols yields output like so:
                //     "0   ExceptionHandlerTest                0x000e2dcc _ZN2EA5Debug16ExceptionHandler25SimulateExceptionHandlingEi + 60"
                // So we do a little post-processing of it.
                void*  pAddress = reinterpret_cast<void*>(static_cast<uintptr_t>(address));
                char** strings  = backtrace_symbols(&pAddress, 1);

                if(strings)
                {
                    FixedString strUnmangled;
                    char        moduleName[256];      moduleName[0] = 0;
                    char        functionMangled[512]; functionMangled[0] = 0;
                    int         offset;
                    
                    EA::StdC::Sscanf(strings[0], "%*s %256s %*s %512s + %d", moduleName, functionMangled, &offset);
                    UnmangleSymbol(functionMangled, strUnmangled, kCompilerTypeGCC);
                    
                    // To consider: include moduleName in the representation.
                    pRepArray[kARTFunctionOffset]      = strUnmangled.c_str();
                    pIntValueArray[kARTFunctionOffset] = offset;
                    addressRepTypeFlags |= kARTFlagFunctionOffset;

                    free(strings); // Apple's documentation for backtrace_symbols specifically states that we don't need to free the individual strings.
                }
            #endif
        }
    }

    return returnFlags;
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressRepAddress
//
int AddressRepLookupSet::GetAddressRepAddress(uint64_t address, FixedString* pRepArray, int* /*pIntValueArray*/)
{
    AddressToString(address, pRepArray[kARTAddress], true);
    return kARTFlagAddress;
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressRepSource
//
int AddressRepLookupSet::GetAddressRepSource(FixedString* pRepArray, int* pIntValueArray)
{
    int returnFlags = 0;

    if(mnSourceCodeFailureCount < 64)
    {
        // Convert char8_t path to wchar_t, as our file path functions are wchar_t.
        wchar_t pPathW[IO::kMaxPathLength];
        EA::StdC::Strlcpy(pPathW, pRepArray[kARTFileLine].c_str(), IO::kMaxPathLength);

        // If the given path exists outright, we can use it as-is and don't need to do
        // any searches through our mSourceCodeDirectoryList.
        bool bFound = IO::File::Exists(pPathW);

        if(!bFound)
        {
            const size_t pathLength = EA::StdC::Strlen(pPathW);

            // Perhaps the file path was not a full path or referred to a hard drive that
            // is on another computer, such as the host computer of a console machine.
            // What we want to do here is match the file name/path in pFilePath with 
            // the directory root(s) specified in mSourceCodeDirectoryList. This is not 
            // trivial, the first part of pFilePath may not match the first part of an
            // mSourceCodeDirectoryList entry, but the file name may nevertheless match
            // a file name that is a child of an mSourceCodeDirectoryList entry.
            // For example, pFilePath might be C:\Baseball\game\source\ai\ai.cpp, while 
            // the source code directory list may contain the directory /host/game/source/
            // of which /host/game/source/ai/ai.cpp is a file. 

            // Loop through all the source code directories that have been added via AddSourceCodeDirectory.
            for(FixedStringWList::iterator it(mSourceCodeDirectoryList.begin()); !bFound && (it != mSourceCodeDirectoryList.end()); ++it)
            {
                const FixedStringW& sSourceDirectory = *it;

                // Try joining the base source code directory with pFilePath (which came
                // from the symbol database). If the resulting file doesn't exist, remove
                // the root-most path component from pFilePath and try again.
                for(IO::Path::PathStringW::iterator itPath(pPathW), itPathEnd(pPathW + pathLength); 
                    itPath != itPathEnd; 
                    itPath = IO::Path::FindComponentFwd(itPath))
                {
                    IO::Path::PathStringW sTestPath(sSourceDirectory.c_str());
                    IO::Path::Append(sTestPath, itPath);
                    IO::Path::Canonicalize(sTestPath);

                    if(IO::File::Exists(sTestPath.c_str()))
                    {
                        bFound = true;

                        EA::StdC::Strncpy(pPathW, sTestPath.c_str(), IO::kMaxPathLength);
                        pPathW[IO::kMaxPathLength - 1] = 0;

                        break;
                    }
                }
            }

            // To consider: Take the mSourceCodeDirectoryList entry list that successfully contained the 
            // source code and if it isn't already at the front of the list, put it there so it will be
            // searched first next time around. 
        }

        if(bFound)
        {
            // Copy the source code to the user supplied output string.
            IO::FileStream fileStream(pPathW);

            if(fileStream.Open(IO::kAccessFlagRead))
            {
                const size_t fileSize  = (size_t)fileStream.GetSize();
                const char   zeroValue = 0;

                eastl::vector<char, EA::Callstack::EASTLCoreAllocator> fileData((eastl_size_t)fileSize, zeroValue, EA::Callstack::EASTLCoreAllocator(EACALLSTACK_ALLOC_PREFIX "EACallstack/AddressRepLookupSet", mpCoreAllocator));

                fileStream.Read(&fileData[0], fileSize);
                fileStream.Close();

                const char8_t* pLineNext   = fileData.data();
                const char8_t* pFileEnd    = fileData.data() + fileSize;
                const int      lineMatch   = pIntValueArray[kARTFileLine];
                const int      lineRangeLo = (int)(lineMatch - (mnSourceContextLineCount / 2));
                const int      lineRangeHi = (int)(lineMatch + (mnSourceContextLineCount / 2));

                // Scan every line of source code up to the end of the desired line range.
                // To consider: This would be faster if we simply counted \n characters 
                // instead of copying lines that we don't use.
                for(int lineCount = 1; (pLineNext < pFileEnd) && (lineCount <= lineRangeHi); ++lineCount)
                {
                    const char8_t* const pLine    = pLineNext;
                    const char8_t* const pLineEnd = EA::StdC::GetTextLine(pLine, pFileEnd, &pLineNext);

                    if((lineCount >= lineRangeLo) && (lineCount <= lineRangeHi))
                    {
                        // We have a source code line in the desired range. Store it.
                        *const_cast<char*>(pLineEnd) = 0;

                        if(lineCount == lineMatch)
                            pRepArray[kARTSource] += "> => ";
                        else
                            pRepArray[kARTSource] += ">    ";
                        pRepArray[kARTSource] += pLine;
                        pRepArray[kARTSource] += "\r\n";  // To consider: make this newline type configurable to \r\n or \n.

                        returnFlags |= kARTFlagSource;
                    }
                }
            }
        }
    } // Else we gave up trying to convert source code and we're just thrashing.

    if(!returnFlags)
        ++mnSourceCodeFailureCount;

    return returnFlags;
}





///////////////////////////////////////////////////////////////////////////////
// AddressRepCache
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// AddressRepCache
//
AddressRepCache::AddressRepCache(EA::Allocator::ICoreAllocator* pCoreAllocator)
  : mpCoreAllocator(pCoreAllocator ? pCoreAllocator : EA::Callstack::GetAllocator()),
    mAddressRepMap(EA::Callstack::EASTLCoreAllocator(EACALLSTACK_ALLOC_PREFIX "EACallstack/RepMap", mpCoreAllocator)),
    mAddressRepLookupSet(pCoreAllocator),
    mCacheSize(0),
    mMaxCacheSize(kMaxCacheSizeDefault)
{
}


///////////////////////////////////////////////////////////////////////////////
// ~AddressRepCache
//
AddressRepCache::~AddressRepCache()
{
    // Empty
}


///////////////////////////////////////////////////////////////////////////////
// SetAllocator
//
void AddressRepCache::SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator)
{
    mpCoreAllocator = pCoreAllocator;

    mAddressRepMap.get_allocator().set_allocator(pCoreAllocator);
    mAddressRepLookupSet.SetAllocator(pCoreAllocator);
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressRep
//
int AddressRepCache::GetAddressRep(int addressRepTypeFlags, uint64_t address, FixedString* pRepArray, int* pIntValueArray)
{
    const char* pStrResults[kARTCount];

    EA::StdC::Memset8(pStrResults, 0, sizeof(pStrResults));

    const int result = GetAddressRep(addressRepTypeFlags, address, pStrResults, pIntValueArray, true);

    for(int i = 0; i < kARTCount; i++)
    {
        if(pStrResults[i])
            pRepArray[i] = pStrResults[i];
    }

    return result;
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressRep
//
const char* AddressRepCache::GetAddressRep(AddressRepType addressRepType, uint64_t address, int* pIntValue, bool bLookupIfMissing)
{
    const char* pStringResults[kARTCount];
    int         pIntResults[kARTCount];

    if(AddressRepCache::GetAddressRep(1 << addressRepType, address, pStringResults, pIntResults, bLookupIfMissing))
    {
        if(pIntValue)
        {
            if(addressRepType == kARTFunctionOffset)
                *pIntValue = pIntResults[addressRepType];
            else if(addressRepType == kARTFileLine)
                *pIntValue = pIntResults[addressRepType];
        }

        return pStringResults[addressRepType];
    }

    return NULL;
}


///////////////////////////////////////////////////////////////////////////////
// GetAddressRep
//
int AddressRepCache::GetAddressRep(int addressRepTypeFlags, uint64_t address, const char** pRepArray, int* pIntValueArray, bool bLookupIfMissing)
{
    int returnFlags = 0;

    // Create an entry if one doesn't already exist. However, if the given entry
    // is going to be a new one, we call SetAllocator on it with our mpAllocator.
    const AddressRepMap::iterator it = mAddressRepMap.find(address);

    AddressRepEntry& ar = mAddressRepMap[address]; // This will create an entry if it isn't present already.

    if(it != mAddressRepMap.end())
        ar.SetAllocator(mpCoreAllocator);


    // If we haven't already looked up one or more of the requested types (regardless of the success of the lookup)...
    if(bLookupIfMissing && ((ar.mARTCheckFlags & addressRepTypeFlags) != addressRepTypeFlags))
    {
        FixedString pStringResults[kARTCount];
        int         pIntResults[kARTCount] = { }; // Initialize to all zeroes.

        // To consider: Look up values other than just what the user requests.
        // That way, if the user later looks up different types for the same address,
        // we'll already have the results.

        // Ignore the return value here, as we'll check the results directly below.
        mAddressRepLookupSet.GetAddressRep(addressRepTypeFlags, address, pStringResults, pIntResults);

        // Record the results in the cache.
        for(size_t i = 0; i < kARTCount; i++)
        {
            // Note that we call SetAddressRep to record the results in the cache even if the results
            // are empty and weren't found. This thus puts empty values into the mAddressRepMap cache.
            if((addressRepTypeFlags & (1 << i)) && !pStringResults[i].empty())
                SetAddressRep((AddressRepType)i, address, pStringResults[i].data(), pStringResults[i].length(), pIntResults[i]);
        }
    }

    // Copy the data available from the cache into the result arrays.
    const int dataAvailableFlags = ar.mARTCheckFlags & addressRepTypeFlags;

    for(size_t i = 0; i < kARTCount; i++)
    {
        if((dataAvailableFlags & (1 << i)) && !ar.mAddressRep[i].empty())
        {
            pRepArray[i] = ar.mAddressRep[i].c_str();

            if(i == kARTFunctionOffset)
                pIntValueArray[i] = ar.mFunctionOffset;
            else if(i == kARTFileLine)
                pIntValueArray[i] = ar.mFileOffset;
            else
                pIntValueArray[i] = 0;

            returnFlags |= (1 << i);
        }
        else
        {
            pRepArray[i] = NULL;
            pIntValueArray[i] = 0;
        }
    }

    return returnFlags;
}


///////////////////////////////////////////////////////////////////////////////
// SetAddressRep
//
void AddressRepCache::SetAddressRep(AddressRepType addressRepType, uint64_t address, 
                                        const char* pRep, size_t nRepLength, int intValue)
{
    // Check to see if nRepLength is a very large number (e.g. size_t(-1)), 
    // in which case we do a EA::StdC::Strlen to tell its length.
    if((ptrdiff_t)nRepLength < 0)
        nRepLength = EA::StdC::Strlen(pRep);

    AddressRepMap::iterator it = mAddressRepMap.find(address);
    size_t nMemoryUsage = CalculateMemoryUsage(it == mAddressRepMap.end(), nRepLength);

    // Disabled until the issue regarding stale pointers into the cache has been resolved.
    //if((mCacheSize + nMemoryUsage) >= mMaxCacheSize)
    //{
    //    PurgeCache((mCacheSize * 3) / 4);
    //    
    //    // The purge may remove an entry corresponding to address, and so we need to recalculate.
    //    it = mAddressRepMap.find(address);
    //    nMemoryUsage = CalculateMemoryUsage(it == mAddressRepMap.end(), nRepLength);
    //}

    // Create an entry if one doesn't already exist. However, if the given entry
    // is going to be a new one, we call SetAllocator on it with our mpAllocator.
    AddressRepEntry& ar = mAddressRepMap[address]; // This will create an entry if it isn't present already.

    if(it != mAddressRepMap.end())
        ar.SetAllocator(mpCoreAllocator);

    // Since the user is setting the value, we unilaterally flag it as having been checked already.
    ar.mARTCheckFlags |= (1 << addressRepType);

    if(addressRepType == kARTFunctionOffset)
        ar.mFunctionOffset = intValue;
    else if(addressRepType == kARTFileLine)
        ar.mFileOffset = intValue;

    ar.mAddressRep[addressRepType].assign(pRep, (eastl_size_t)nRepLength);
    mCacheSize += nMemoryUsage;
}


///////////////////////////////////////////////////////////////////////////////
// SetMaxCacheSize
//
void AddressRepCache::SetMaxCacheSize(size_t maxSize)
{
    mMaxCacheSize = maxSize;

    // Disabled until the issue regarding stale pointers into the cache has been resolved.
    //if(mCacheSize > mMaxCacheSize)
    //    PurgeCache((mMaxCacheSize * 3) / 4);
}


///////////////////////////////////////////////////////////////////////////////
// CalculateMemoryUsage
//
size_t AddressRepCache::CalculateMemoryUsage(bool bNewNode, size_t nRepLength)
{
    size_t usage = (sizeof(char) * (nRepLength + 1)); // The AddressRepEntry::mAddressRep string allocates this much memory

    if(bNewNode)
        usage += sizeof(AddressRepMap::node_type);

    return usage;
}


///////////////////////////////////////////////////////////////////////////////
// PurgeCache
//
// This purge function is unusable until we resolve a primary problem. 
// The problem is that the user might be holding pointers into our cache 
// memory and we don't know when it is safe to purge entries or what entries 
// are safe to purge. Probably we'll want some kind of marker that no users 
// are holding onto pointers.
//
void AddressRepCache::PurgeCache(size_t newCacheSize)
{
    if(newCacheSize == 0)
    {
        mAddressRepMap.clear();
        mCacheSize = 0;
    }
    else
    {
        // We currently do a dumb algorithm whereby we simply walk along the hashtable
        // nodes and remove entries semi randomly.
        AddressRepMap::iterator it = mAddressRepMap.begin();

        while(!mAddressRepMap.empty() && (mCacheSize > newCacheSize))
        {
            AddressRepMap::value_type& value = *it;
            const AddressRepEntry& are = value.second;

            // Calculate and update how much memory we have trimmed.
            size_t entrySize = sizeof(AddressRepMap::node_type);

            for(int i = 0; i < kARTCount; i++)
            {
                if(!are.mAddressRep[i].empty())
                    entrySize += (sizeof(char) * (are.mAddressRep[i].length() + 1));
            }

            mCacheSize -= entrySize;

            // Remove the entry.
            AddressRepMap::const_local_iterator itL(const_cast<AddressRepMap::iterator::node_type*>(it.get_node()));
            const bool bBucketHadMoreThanOneEntry = (++itL.mpNode == NULL);
            it = mAddressRepMap.erase(it);

            // Move to the next bucket, as opposed to the next element. This will have
            // the convenient effect of removing the oldest entries in the hash table.
            if(bBucketHadMoreThanOneEntry)
                it.increment_bucket();
            if(it == mAddressRepMap.end())
                it = mAddressRepMap.begin();
        }
    }
}




} // namespace Callstack

} // namespace EA








