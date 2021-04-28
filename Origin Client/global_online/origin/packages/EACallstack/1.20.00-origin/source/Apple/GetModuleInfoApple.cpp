
///////////////////////////////////////////////////////////////////////////////
// GetModuleInfoApple.cpp
//
// Copyright (c) 2010 Electronic Arts Inc.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/Apple/EACallstackApple.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAScanf.h>
#include <EAIO/PathString.h>
#include <EAAssert/eaassert.h>
#include <EAStdC/EAMemory.h>
#include <EAIO/EAFileBase.h> //EA::IO::kMaxPathLength
#include <mach-o/dyld_images.h> //dyld_all_image_infos
#include <mach-o/dyld.h> //segment_command(_64)

#if defined(EA_PLATFORM_IPHONE)
    //On iPhone, this gets pulled in dynamically through libproc.dylib
    extern "C" int proc_regionfilename(int pid, uint64_t address, void * buffer, uint32_t buffersize);
#else
    #include <libproc.h> //proc_regionfilename
#endif

// This function gets pulled in dynamically through libdyld.dylib
extern "C" const struct dyld_all_image_infos* _dyld_get_all_image_infos();


#if defined(__LP64__)
typedef struct mach_header_64     MachHeader;
typedef struct segment_command_64 SegmentCommand;
typedef struct section_64         Section;
#define kLCSegment                LC_SEGMENT_64
#else
typedef struct mach_header        MachHeader;
typedef struct segment_command    SegmentCommand;
typedef struct section            Section;
#define kLCSegment                LC_SEGMENT
#endif


namespace EA
{
namespace Callstack
{
// This function determines the UUID of a given module and
void GetModuleUuid(eastl::string& modulePath, eastl::string& moduleUuid)
{
    moduleUuid.clear();
    
    char command[PATH_MAX];
    sprintf(command, "otool -l %s | grep uuid", modulePath.data());
    FILE*  pFile = popen(command, "r");
    if (pFile)
    {
        char line[512];
        
        while(fgets(line, EAArrayCount(line), pFile))
        {
            char cmd[16];
            char uuid[64];
            int fieldCount = EA::StdC::Sscanf(line, "%16s %64s", cmd, uuid);
            if (fieldCount == 2 && strcmp(cmd, "uuid") == 0)
            {
                moduleUuid.assign(uuid);
            }
        }
        
        pclose(pFile);
    }
}

void GetAllModuleInfoAppleUUIDs(ModuleInfoAppleArray& moduleInfos)
{
    typedef eastl::map<eastl::string, eastl::string> UuidMap;
    UuidMap uuids;
    
    ModuleInfoAppleArray::iterator end(moduleInfos.end());
    for(ModuleInfoAppleArray::iterator i(moduleInfos.begin()); i != end; ++i)
    {
        eastl::string path(&(i->mPath[0]));
        UuidMap::iterator where = uuids.find(path);
        if (where != uuids.end())
        {
            strncpy(i->mUuid, where->second.c_str(), sizeof(i->mUuid));
        }
        else
        {
            eastl::string uuid;
            GetModuleUuid(path, uuid);
            if (!uuid.empty())
            {
                uuids[path] = uuid;
                strncpy(i->mUuid, uuid.c_str(), sizeof(i->mUuid));
            }
        }
    }
}
    
// This fills a moduleInfoApple object with the information from all of the segments listed in the given mach_header's segments, starting at the given currentSegmentPos. It also puts the pModulePath info the moduleInfoApple object, which is then push_back on the given array.
//
// moduleInfoAppleArray is an in/out param that is push_back using information derived from other parameters
// pTypeFilter is used to filter out segment types
// pModulePath is the path corresponding to the given pMachHeader. It is assumed it is NullTerminated
// currentSegmentPos is the starting segment we are iterating over
// pMachHeader is the mach_header with all the segment information
void CreateModuleInfoApple(ModuleInfoAppleArray& moduleInfoAppleArray, const char* pTypeFilter, const char* pModulePath, uintptr_t currentSegmentPos, const MachHeader* pMachHeader, intptr_t offset)
{
    for(uint32_t i = 0; i < pMachHeader->ncmds; i++) // Look at each command, paying attention to LC_SEGMENT/LC_SEGMENT_64 (segment_command) commands.
    {
        const SegmentCommand* pSegmentCommand = reinterpret_cast<const SegmentCommand*>(currentSegmentPos); // This won't actually be a segment_command unless the type is kLCSegment
        
        if(pSegmentCommand != NULL && pSegmentCommand->cmd == kLCSegment) // If this really is a segment_command... (otherwise it is some other kind of command)
        {
            const size_t segnameBufferLen = sizeof(pSegmentCommand->segname) + 1;
            
            char segnameBuffer[segnameBufferLen];
            EA::StdC::Memcpy(segnameBuffer, pSegmentCommand->segname, sizeof(pSegmentCommand->segname));
            segnameBuffer[segnameBufferLen-1] = '\0'; // Incase segname was not 0-terminated
            
            if(!pTypeFilter || EA::StdC::Strncmp(segnameBuffer, pTypeFilter, sizeof(segnameBuffer)))
            {
                ModuleInfoApple& info = moduleInfoAppleArray.push_back();
                uint64_t uOffset = (uint64_t)offset;
                info.mBaseAddress = (uint64_t)(pSegmentCommand->vmaddr + uOffset);
                info.mModuleHandle = reinterpret_cast<ModuleHandle>((uintptr_t)info.mBaseAddress);
                info.mSize = (uint64_t)pSegmentCommand->vmsize;
                info.mPath = pModulePath;
                info.mName = EA::IO::Path::GetFileName(pModulePath);
                
                info.mPermissions[0] = (pSegmentCommand->initprot & VM_PROT_READ)    ? 'r' : '-';
                info.mPermissions[1] = (pSegmentCommand->initprot & VM_PROT_WRITE)   ? 'w' : '-';
                info.mPermissions[2] = (pSegmentCommand->initprot & VM_PROT_EXECUTE) ? 'x' : '-';
                info.mPermissions[3] = '/';
                info.mPermissions[4] = (pSegmentCommand->maxprot & VM_PROT_READ)    ? 'r' : '-';
                info.mPermissions[5] = (pSegmentCommand->maxprot & VM_PROT_WRITE)   ? 'w' : '-';
                info.mPermissions[6] = (pSegmentCommand->maxprot & VM_PROT_EXECUTE) ? 'x' : '-';
                info.mPermissions[7] = '\0';
                
                EA::StdC::Strlcpy(info.mType,pSegmentCommand->segname,EAArrayCount(info.mType));
                //**********************************************************************************
                //For Debugging Purposes
                //__TEXT                 0000000100000000-0000000100001000 [    4K] r-x/rwx SM=COW  /Build/Products/Debug/TestProject.app/Contents/MacOS/TestProject
                //printf("%20s %llx-%llx %s %s\n", segnameBuffer, (unsigned long long)info.mBaseAddress, (unsigned long long)(info.mBaseAddress + pSegmentCommand->vmsize), info.mPermissions, pModulePath);
                //**********************************************************************************/
            }
            
        }
        currentSegmentPos += pSegmentCommand->cmdsize;
    }
}
    
// GetModuleInfoApple
//
// This function exists for the purpose of being a central module/VM map info collecting function,
// used by a couple functions within EACallstack.
//
// We used to use vmmap and parse the output
// https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man1/vmmap.1.html
// But starting ~osx 10.9 vmmap can not be called due to new security restrictions
//
// I tried using ::mach_vm_region, but I was unable to find the type of the segments (__TEXT, etc.)
// and system libraries addresses were given, but their name/modulePath was not.
// ::mach_vm_region_recurse did not solve this problem either.
//
// We currently call _dyld_get_all_image_infos and walk through each header and its segments
// which is fairly straightforward: https://developer.apple.com/library/mac/documentation/developertools/conceptual/machoruntime/reference/reference.html
// We also have code here similiar to deprecated functions firstsegfromheader and nextsegfromheader:
// http://fxr.watson.org/fxr/source/bsd/kern/mach_header.c?v=xnu-792.6.70;im=10
ModuleInfoAppleArray gModuleInfoAppleArrayCache;

size_t GetModuleInfoApple(ModuleInfoAppleArray& moduleInfoAppleArray, const char* pTypeFilter, bool bEnableCache)
{
    if(bEnableCache)
    {
        if(gModuleInfoAppleArrayCache.empty())
            GetModuleInfoApple(gModuleInfoAppleArrayCache, NULL, false); // Call ourselves recursively.

        for(eastl_size_t i = 0, iEnd = gModuleInfoAppleArrayCache.size(); i != iEnd; i++)
        {
            const ModuleInfoApple& mia = gModuleInfoAppleArrayCache[i];

            if(!pTypeFilter || strstr(mia.mType, pTypeFilter))
                moduleInfoAppleArray.push_back(mia);
        }
    }
    else
    {
        const struct dyld_all_image_infos* pAllImageInfos = _dyld_get_all_image_infos();
        
        for(uint32_t i = 0; i < pAllImageInfos->infoArrayCount; i++)
        {
            const char* pModulePath = pAllImageInfos->infoArray[i].imageFilePath;
            if(pModulePath != NULL && EA::StdC::Strncmp(pModulePath, "", EA::IO::kMaxPathLength) != 0)
            {
                uintptr_t         currentSegmentPos = (uintptr_t)pAllImageInfos->infoArray[i].imageLoadAddress;
                const MachHeader* pMachHeader       = reinterpret_cast<const MachHeader*>(currentSegmentPos);
                EA_ASSERT(pMachHeader != NULL);
                currentSegmentPos += sizeof(*pMachHeader);
                
                // The system library addresses we obtain are the linker address.
                // So we need to get the get the dynamic loading offset
                // The offset is also stored in pAllImageInfos->sharedCacheSlide, but there is no way
                // to know whether or not it should get used on each image. (dyld and our executable images do not slide)
                // http://lists.apple.com/archives/darwin-kernel/2012/Apr/msg00012.html
                intptr_t offset = _dyld_get_image_vmaddr_slide(i);
                CreateModuleInfoApple(moduleInfoAppleArray, pTypeFilter, pModulePath, currentSegmentPos, pMachHeader, offset);
            }
        }

        // Iterating on dyld_all_image_infos->infoArray[] does not give us entries for /usr/lib/dyld.
        // We use the mach_header to get /usr/lib/dyld
        const MachHeader* pMachHeader = (const MachHeader*)pAllImageInfos->dyldImageLoadAddress;
        uintptr_t         currentSegmentPos = (uintptr_t)pMachHeader + sizeof(*pMachHeader);
        char modulePath[EA::IO::kMaxPathLength] = "";
        pid_t  pid = getpid();
        int filenameLen = proc_regionfilename((int)pid,currentSegmentPos,modulePath,(uint32_t)sizeof(modulePath));
        EA_ASSERT(filenameLen > 0 && modulePath != NULL && EA::StdC::Strncmp(modulePath,"",sizeof(modulePath)) != 0);
        if(filenameLen > 0)
        {
            CreateModuleInfoApple(moduleInfoAppleArray, pTypeFilter, modulePath, currentSegmentPos, pMachHeader, 0); // offset is 0 because dyld is already loaded
        }
        
        // Use this to compare results
        // printf("vmmap -w %lld", (int64_t)pid);
        
        GetAllModuleInfoAppleUUIDs(moduleInfoAppleArray);
    }
    return (size_t)moduleInfoAppleArray.size();
}


}} //namespace

